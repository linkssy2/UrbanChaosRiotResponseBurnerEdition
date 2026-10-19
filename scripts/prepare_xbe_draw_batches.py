"""Prepare bounded D3D12 submissions and their hardware fixture; never launches."""
from pathlib import Path
import argparse
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from prepare_xbe_perf import once, function

MARKER = "// UC_DRAW_BATCHES_CANDIDATE"
FIELDS = '''        // Lists and arenas belong to this allocator until its existing fence wait.
        ComPtr<ID3D12GraphicsCommandList> draw_main;
        std::array<ComPtr<ID3D12GraphicsCommandList>, 16> draw_lists;
        uint32_t draw_list_cursor = 0;
        uint32_t draw_batch_draws = 0;
        uint32_t draw_batches_remaining = 16;'''
METHODS = '''// Called only after this frame's existing allocator-retirement wait.
void D3D12Backend::UseFrameDrawList(Frame& f) {
    if (!f.draw_main) {
        if (FAILED(dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, f.alloc.Get(), nullptr,
                                          IID_PPV_ARGS(&f.draw_main)))) throw std::bad_alloc();
        if (FAILED(f.draw_main->Close())) throw std::runtime_error("draw primary close failed");
    }
    cl_ = f.draw_main;
    f.draw_list_cursor = 0;
}

// Between complete draws: submit earlier work without retiring any of its storage.
void D3D12Backend::MaybeSplitDrawBatch() {
    Frame& f = CurrentFrame();
    if (f.draw_batch_draws < 256 || !f.draw_batches_remaining ||
        f.draw_list_cursor >= f.draw_lists.size()) return;
    auto& next = f.draw_lists[f.draw_list_cursor++];
    // A list slot is used only once between waited allocator resets.
    Submit(false);
    if (!next) {
        if (FAILED(dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, f.alloc.Get(), nullptr,
                                          IID_PPV_ARGS(&next)))) throw std::bad_alloc();
    } else if (FAILED(next->Reset(f.alloc.Get(), nullptr))) {
        throw std::runtime_error("draw batch reset failed");
    }
    cl_ = next;
    --f.draw_batches_remaining;
#ifndef UC_BURNER_RELEASE
    ++g_perf.draw_batches;
#endif
    Begin();
}

'''


def transform(source):
    if MARKER in source:
        raise RuntimeError("Draw batches already applied; regenerate before applying again")
    source = once(source, '#include <new>', '#include <new>\n#include <stdexcept>\n' + MARKER)
    source = once(source, '    uint64_t draws = 0;',
                  '    uint64_t draws = 0;\n    uint64_t draw_batches = 0;')
    source = once(source, '        ComPtr<ID3D12CommandAllocator> alloc;',
                  '        ComPtr<ID3D12CommandAllocator> alloc;\n' + FIELDS)
    source = once(source, '    void Begin();',
                  '    void UseFrameDrawList(Frame& f);\n    void MaybeSplitDrawBatch();\n    void Begin();')
    source = once(source,
        '    dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames_[0].alloc.Get(), nullptr, IID_PPV_ARGS(&cl_));',
        '    dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames_[0].alloc.Get(), nullptr, IID_PPV_ARGS(&cl_));\n'
        '    frames_[0].draw_main = cl_;\n'
        '#ifndef UC_BURNER_RELEASE\n'
        '    LOGR("d3d12: draw batching enabled (256 draws, maximum 16 extra submissions/frame)");\n'
        '#endif')
    source = once(source, 'void D3D12Backend::Begin() {', METHODS +
                  'void D3D12Backend::Begin() {\n    CurrentFrame().draw_batch_draws = 0;')
    submit = function(source, 'void D3D12Backend::Submit(bool wait) {')
    source = once(source, submit, once(submit, '        f.alloc->Reset();',
                  '        f.alloc->Reset();\n        UseFrameDrawList(f);'))
    reset = function(source, 'void D3D12Backend::ResetFrame() {')
    source = once(source, reset, once(reset, '    f.alloc->Reset();',
                  '    f.alloc->Reset();\n    UseFrameDrawList(f);\n    f.draw_batches_remaining = 16;'))
    signature = 'void D3D12Backend::Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) {'
    source = once(source, signature, signature + '\n    MaybeSplitDrawBatch();')
    source = once(source, '    DrawGeometry(a);',
                  '    DrawGeometry(a);\n    ++CurrentFrame().draw_batch_draws;')
    source = once(source, '        g_perf = Perf{};',
                  '        LOGR("perf: draw batches %.2f extra nonblocking submissions/frame", p.draw_batches / n);\n'
                  '        g_perf = Perf{};')
    return source


def prepare(area):
    path = Path(area) / 'src/compat/d3d12.cpp'
    changed = transform(path.read_text())
    fixture = Path(__file__).with_name('test_xbe_draw_batches.cpp').read_text()
    fixture = once(fixture, '/* FRAME_DRAW_BATCHES */', FIELDS)
    fixture = once(fixture, '/* DRAW_BATCH_METHODS */',
                   function(changed, 'void D3D12Backend::UseFrameDrawList(') + '\n\n' +
                   function(changed, 'void D3D12Backend::MaybeSplitDrawBatch('))
    path.write_text(changed)
    (path.parent / 'test_draw_batches.cpp').write_text(fixture)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--area', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.check:
        original = (args.area / 'src/compat/d3d12.cpp').read_text()
        changed = transform(original)
        assert changed.count('WaitFence(') == original.count('WaitFence(')
        assert changed.count('alloc->Reset()') == original.count('alloc->Reset()')
        assert changed.count('MaybeSplitDrawBatch();') == 2  # Declaration and draw entry.
        draw = function(changed, 'void D3D12Backend::Draw(')
        assert draw.index('MaybeSplitDrawBatch();') < draw.index('DrawEnd draw_end')
        assert draw.count('++CurrentFrame().draw_batch_draws;') == 1
        split = function(changed, 'void D3D12Backend::MaybeSplitDrawBatch(')
        assert split.index('Submit(false);') < split.index('next->Reset(')
        assert all(term not in split for term in ('WaitFence', 'alloc->Reset', '.used', '.srv', '.clear()', 'geometry_reuse_'))
        assert changed.count('UseFrameDrawList(f);') == 2
        try:
            transform(changed)
        except RuntimeError:
            pass
        else:
            raise AssertionError('Repeat transform accepted')
        print('Draw batch transform checks passed; no files changed')
    else:
        prepare(args.area)
