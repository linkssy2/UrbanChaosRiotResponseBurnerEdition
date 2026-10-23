"""Opt-in frame evidence for the walkway report; never part of release builds."""
from pathlib import Path


def once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def transform(main, renderer):
    # Handle the diagnostic key before the game's input handler consumes it.
    seam = '    if (intptr_t result = 0; kGame.message'
    main = once(main, seam, '''    if (msg == WM_KEYUP && wp == VK_F9) {
        static unsigned captures = 0;
        if (captures < 4 && !gpu::g_capture.exchange(true)) ++captures;
        return 0;
    }
''' + seam)
    renderer = once(renderer, '    CaptureTextures(s, stages);', '''    fprintf(cap_, " front_ccw %u depth_clip %u samples %u viewport %.9g %.9g %.9g %.9g",
        k.ccw, k.depth_clip, k.samples, c.surf[0], c.surf[1], c.surf[2], c.surf[3]);
    CaptureTextures(s, stages);''')
    renderer = once(renderer, '    if (count <= kQuadVertices) CaptureInputs(s, a, c);',
        '    if (count <= kQuadVertices || s.rs[kRsStencilEnable]) CaptureInputs(s, a, c);')
    renderer = once(renderer, '        EndDrawCapture(s, a, count, c);', '''        if (count > kQuadVertices && !s.rs[kRsStencilEnable] &&
            (stages.info[0].shadow || stages.info[1].shadow || stages.info[2].shadow || stages.info[3].shadow))
            CaptureInputs(s, a, c);
        EndDrawCapture(s, a, count, c);''')
    renderer = once(renderer, '        if (res && !tg && stages.info[i].kind == kTex2D && cap_seen_.insert(d.data).second) {', '''        if (res && tg && stages.info[i].shadow) {
            const auto& ts = s.tss[size_t(i)];
            fprintf(cap_, " shadow_func %X filters %u/%u address %u/%u border %08X",
                s.rs[kRsShadowFunc], ts[kTssMinFilter], ts[kTssMagFilter],
                ts[kTssAddressU], ts[kTssAddressV], ts[kTssBorderColor]);
            cap_shadow_.insert(SizedTargetKey(d.data, true, d.width, d.height));
        }
        if (res && !tg && stages.info[i].kind == kTex2D && cap_seen_.insert(d.data).second) {''')
    renderer = once(renderer, '    std::unordered_set<uint32_t> cap_seen_;',
        '    std::unordered_set<uint32_t> cap_seen_;\n    std::unordered_set<uint64_t> cap_shadow_, cap_phases_;')
    renderer = once(renderer, '    cap_seen_.clear();',
        '    cap_seen_.clear();\n    cap_shadow_.clear();\n    cap_phases_.clear();')
    renderer = once(renderer, '    if (cap_) CapStep(s.rt);', '''    if (cap_) {
        CapStep(s.rt);
        const uint32_t ps = s.ps ? s.ps->handle : 0;
        const uint64_t phase = (uint64_t(ps) << 32) | s.textures[1];
        // Capture before any draw setup; CapSave invalidates targets and submits commands.
        if (cap_phases_.size() < 24 && cap_phases_.insert(phase).second) {
            if (Target* target = FindTarget(s.rt, false)) {
                char name[64];
                snprintf(name, sizeof(name), "%04u_before_ps_%08X", cap_i_, ps);
                CapSave(*target, name);
            }
        }
    }''')
    renderer = once(renderer, '    fclose(cap_);', '''    // Readback resets draw bindings: only run after all guest draws have finished.
    for (auto key : cap_shadow_) {
        auto it = targets_.find(key);
        if (it == targets_.end()) continue;
        char name[48];
        snprintf(name, sizeof(name), "shadow_end_%08X", it->second.data);
        CapSave(it->second, name);
    }
    fclose(cap_);''')
    renderer = once(renderer, '    FILE* cap_ = nullptr;', '''    FILE* cap_ = nullptr;
    std::unique_ptr<FILE, decltype(&fclose)> wall_trace_{nullptr, &fclose};
    unsigned wall_frames_ = 0, wall_draws_ = 0;
    uint64_t wall_bytes_ = 0;
    uint64_t wall_saved_bytes_ = 0;
    std::unordered_set<uint64_t> wall_saved_;''')
    renderer = once(renderer, '    cap_tex_.clear();', '''    cap_tex_.clear();
    // Follow the isolated capture with consecutive CPU-side state, without GPU readbacks.
    wall_trace_.reset(_wfopen((cap_dir_ / "lighting-sequence.txt").c_str(), L"w"));
    wall_frames_ = wall_trace_ ? 180 : 0;
    wall_draws_ = 0;
    wall_bytes_ = 0;
    wall_saved_bytes_ = 0;
    wall_saved_.clear();
    if (wall_trace_) fprintf(wall_trace_.get(), "begin frame=%llu tick=%llu\\n",
        (unsigned long long)frame_, (unsigned long long)GetTickCount64());''')
    renderer = once(renderer, '    UploadConstants(s, sh, k, stages, size, c, analytic_fog);', '''    UploadConstants(s, sh, k, stages, size, c, analytic_fog);
    if (wall_trace_ && wall_draws_++ < 4096) {
        TextureDesc td{};
        if (s.textures[1]) Describe(s.textures[1], td);
        fprintf(wall_trace_.get(), "draw %u rt=%08X ds=%08X vs=%08X ps=%08X count=%u t1=%08X pso=%016llX constants=%016llX projection=%016llX color=%.9g,%.9g,%.9g,%.9g\\n",
            wall_draws_, s.rt.data, s.zs.data, s.vs_handle, s.ps ? s.ps->handle : 0,
            count, td.data, (unsigned long long)Hash(&k, sizeof(k)),
            (unsigned long long)Hash(&c, sizeof(c)),
            (unsigned long long)Hash(&s.vsc[144], 4 * sizeof(s.vsc[0])),
            c.pc[0][0], c.pc[0][1], c.pc[0][2], c.pc[0][3]);
        if (s.vs_handle == 0xB01012E1u || s.vs_handle == 0xB01013A1u) {
            fprintf(wall_trace_.get(), "receiver draw=%u position=%016llX viewport=%016llX light01=%016llX light4=%016llX vertices=%u base=%d decode=%016llX\\n",
                wall_draws_, (unsigned long long)Hash(&s.vsc[104], 4 * sizeof(s.vsc[0])),
                (unsigned long long)Hash(&s.vsc[58], 2 * sizeof(s.vsc[0])),
                (unsigned long long)Hash(&c.pc[0], 2 * sizeof(c.pc[0])),
                (unsigned long long)Hash(&c.pc[4], sizeof(c.pc[0])),
                a.vertex_count, a.base_vertex, (unsigned long long)a.decode);
            // Bound CPU hashing; omission is explicit rather than a false unchanged fingerprint.
            const auto record = [&](const char* kind, unsigned slot, const void* data, uint64_t bytes, unsigned stride) {
                const bool read = data && bytes <= (4u << 20) - wall_bytes_;
                uint64_t hash = 0;
                if (read) {
                    hash = Hash(data, size_t(bytes)); wall_bytes_ += bytes;
                    // Retain actual inputs for offline comparison of differently batched surfaces.
                    if (!wall_saved_.count(hash) && bytes <= (64u << 20) - wall_saved_bytes_ && wall_saved_.size() < 4096) {
                        char name[64];
                        snprintf(name, sizeof(name), "input_%016llX.bin", (unsigned long long)hash);
                        FILE* file = _wfopen((cap_dir_ / name).c_str(), L"wb");
                        if (file) {
                            const bool written = fwrite(data, 1, size_t(bytes), file) == bytes;
                            const bool closed = fclose(file) == 0;
                            wall_saved_bytes_ += bytes;
                            if (written && closed) wall_saved_.insert(hash);
                        }
                    }
                    fprintf(wall_trace_.get(), "snapshot hash=%016llX saved=%u\\n",
                        (unsigned long long)hash, unsigned(wall_saved_.count(hash) != 0));
                }
                fprintf(wall_trace_.get(), "input %s slot=%u bytes=%llu stride=%u read=%u hash=%016llX\\n",
                    kind, slot, (unsigned long long)bytes, stride, unsigned(read), (unsigned long long)hash);
            };
            for (unsigned i = 0; i < a.buffer_count; ++i)
                record("vertex", i, a.buffers[i].src, a.buffers[i].bytes, a.buffers[i].stride);
            if (a.index_count) record("index", 0, a.index_src, uint64_t(a.index_count) * (a.index16 ? 2 : 4), a.index16 ? 2 : 4);
            for (unsigned i = 0; i < a.elements.size(); ++i) if (a.regs & (1u << i))
                fprintf(wall_trace_.get(), "element reg=%u format=%u buffer=%u offset=%u\\n",
                    i, unsigned(a.elements[i].format), unsigned(a.elements[i].buffer), unsigned(a.elements[i].offset));
            fprintf(wall_trace_.get(), "raster topology=%u depth=%u/%u/%u bias=%d slope=%.9g cull=%u ccw=%u clip=%u samples=%u\\n",
                unsigned(a.topology), k.zen, k.zfunc, k.zwrite, k.zbias, k.zslope, k.cull, k.ccw, k.depth_clip, k.samples);
            for (unsigned row : {58u, 59u, 104u, 105u, 106u, 107u})
                fprintf(wall_trace_.get(), "matrix row=%u %.9g %.9g %.9g %.9g\\n", row,
                    s.vsc[row][0], s.vsc[row][1], s.vsc[row][2], s.vsc[row][3]);
        }
    }''')
    renderer = once(renderer, '    EndAllSmoothFog(); // a missing/disabled tone pass cannot leak the temporary format', '''    if (wall_trace_) {
        fprintf(wall_trace_.get(), "end frame=%llu tick=%llu draws=%u truncated=%u\\n",
            (unsigned long long)frame_, (unsigned long long)GetTickCount64(), wall_draws_, unsigned(wall_draws_ > 4096));
        wall_draws_ = 0;
        wall_bytes_ = 0;
        if (--wall_frames_ == 0) wall_trace_.reset();
    }
    EndAllSmoothFog(); // a missing/disabled tone pass cannot leak the temporary format''')
    capture_textures = renderer.split('void D3D12Backend::CaptureTextures(', 1)[1].split('\n}\n', 1)[0]
    assert 'CapSave(' not in capture_textures
    assert 'cap_shadow_.insert(' in capture_textures
    draw = renderer.split('void D3D12Backend::Draw(const State&', 1)[1].split('\n}\n', 1)[0]
    assert draw.index('CapSave(*target, name)') < draw.index('BindTargets(s)')
    trace = draw.split('if (wall_trace_ &&', 1)[1].split('\n    }', 1)[0]
    assert 'wall_draws_++ < 4096' in trace
    assert not any(call in trace for call in ('CapSave(', 'Submit(', 'ReadTarget('))
    assert 'wall_trace_ ? 180 : 0' in renderer
    assert 'if (--wall_frames_ == 0) wall_trace_.reset();' in renderer
    assert 'Hash(&s.vsc[104], 4 * sizeof(s.vsc[0]))' in draw
    assert 'Hash(&s.vsc[58], 2 * sizeof(s.vsc[0]))' in draw
    assert 'bytes <= (4u << 20) - wall_bytes_' in draw
    assert 'bytes <= (64u << 20) - wall_saved_bytes_' in draw
    assert 'wall_saved_.size() < 4096' in draw
    assert 'if (written && closed) wall_saved_.insert(hash);' in draw
    assert renderer.count('wall_bytes_ = 0;') == 3
    return main, renderer


def prepare(area):
    area = Path(area)
    main = area/'src/compat/main.cpp'
    renderer = area/'src/compat/d3d12.cpp'
    m, r = transform(main.read_text(), renderer.read_text())
    main.write_text(m)
    renderer.write_text(r)


if __name__ == '__main__':
    # Exercise the exact current production seams without modifying staged files.
    root = Path(__file__).resolve().parents[1]
    area = root/'build/xbe-full-integration-20261014/src/compat'
    m, r = transform((area/'main.cpp').read_text(), (area/'d3d12.cpp').read_text())
    assert m.index('wp == VK_F9') < m.index('kGame.message &&')
    assert 'captures < 4' in m and 'WM_KEYUP && wp == VK_F9' in m
    assert '|| s.rs[kRsStencilEnable]' in r
    try:
        transform(m, r)
    except AssertionError:
        pass
    else:
        raise AssertionError('Repeated diagnostic preparation must fail')
    print('Shadow diagnostic source seams, key routing, capture bound and repeat guard passed')
