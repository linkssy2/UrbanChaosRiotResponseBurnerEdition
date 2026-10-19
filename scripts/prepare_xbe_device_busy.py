"""Keep the original device-busy query nonblocking on the threaded renderer."""
from pathlib import Path
from prepare_xbe_perf import once

METHOD = '''    bool DeviceBusy() override {
        // Queue one completion marker for new work; repeated polls must not extend it.
        if (!cur_->cmds.empty() || cur_->id - 1 > busy_probe_) {
            cur_->cmds.push_back(Cmd{Op::Flush});
            busy_probe_ = Submit();
        }
        return done_.load(std::memory_order_acquire) < busy_probe_;
    }
'''

def prepare(sdk, area):
    sdk, area = Path(sdk), Path(area)
    header = (sdk/'runtime/gpu/backend.h').read_text()
    header = once(header, '    virtual void Flush() = 0;',
        '    virtual void Flush() = 0;\n'
        '    virtual bool DeviceBusy() { Flush(); return false; }')
    (area/'src/gpu').mkdir(exist_ok=True)
    (area/'src/gpu/backend.h').write_text(header)
    path = area/'src/compat/render_thread.cpp'
    text = path.read_text()
    text = once(text, '    void SetGamma(const uint8_t* ramp) override {',
                METHOD+'\n    void SetGamma(const uint8_t* ramp) override {')
    text = once(text, '    Batch* cur_ = nullptr;',
                '    Batch* cur_ = nullptr;\n    uint64_t busy_probe_ = 0;')
    path.write_text(text)
    path = area/'src/compat/d3d8.cpp'
    text = path.read_text()
    text = once(text, '''    // shortcut: synchronous busy query; replace with nonblocking completion polling after boot parity.
    if (g_backend) g_backend->Flush();
    return 0;''', '''    return g_backend && g_backend->DeviceBusy() ? 1u : 0u;''')
    path.write_text(text)
    fixture = Path(__file__).with_name('test_xbe_device_busy.cpp').read_text()
    (area/'src/compat/test_device_busy.cpp').write_text(
        fixture.replace('/* METHOD */', METHOD.replace(' override', '')))
