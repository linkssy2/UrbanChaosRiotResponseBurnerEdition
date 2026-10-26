"""Backport XboxXBERecomp c9cda4a without changing render-target sampling."""
from pathlib import Path


HELPERS = '''namespace {
// XboxXBERecomp c9cda4ac39e1e8823a1517f950966a7ee1bd3905 (MIT).
// args in PixelKey::ff order: arg1, arg2, arg0.
bool ReadsTexture(uint32_t op, const uint32_t* args, bool alpha) {
    if (op == 14 || op == 16) return true; // texture-alpha blends
    bool alpha_arg1_only = alpha && op >= 18 && op <= 21;
    bool uses_arg1 = op != 3;
    bool uses_arg2 = op != 2 && !alpha_arg1_only;
    bool uses_arg0 = op == 23 || op == 24;
    auto texture = [](uint32_t a) { return (a & 0xF) == 2; };
    return (uses_arg1 && texture(args[0])) || (uses_arg2 && texture(args[1])) || (uses_arg0 && texture(args[2]));
}

// A fixed-function op reading an absent texture passes the incoming colour.
void PassMissingTexture(uint32_t* f) {
    if (f[0] > 1 && ReadsTexture(f[0], f + 1, false)) {
        f[0] = 2;
        f[1] = 1;
    }
    if (f[4] > 1 && ReadsTexture(f[4], f + 5, true)) {
        f[4] = 2;
        f[5] = 1;
    }
}
} // namespace

'''
CALL = '        if (!tex[i].kind) PassMissingTexture(k.ff[i]);\n'


def transform(source):
    signature = 'PixelKey MakePixelKey(const State& s, const TexInfo tex[4]) {'
    anchor = '        std::memcpy(k.ff[i], v, sizeof(v));\n'
    assert source.count(signature) == source.count(anchor) == 1
    if HELPERS in source:
        assert source.count(HELPERS) == source.count(CALL) == 1
        return source
    assert 'PassMissingTexture' not in source
    return source.replace(signature, HELPERS + signature, 1).replace(anchor, anchor + CALL, 1)


def prepare(area):
    compat = Path(area) / 'src/compat'
    path = compat / 'shaders.cpp'
    source = path.read_text()
    changed = transform(source)
    assert transform(changed) == changed
    original = changed.replace(HELPERS, '').replace(CALL, '')
    assert transform(original) == changed
    start = original.index('PixelKey MakePixelKey(')
    function = original[start:original.index('\n}', start) + 2]
    fixture = Path(__file__).with_name('test_xbe_missing_texture.cpp').read_text()
    assert fixture.count('// ORIGINAL_KEY') == 1
    fixture = fixture.replace('// ORIGINAL_KEY', 'namespace gpu {\n' + function.replace('MakePixelKey(', 'BeforeKey(', 1) + '\n}')
    path.write_text(changed)
    (compat / 'test_missing_texture.cpp').write_text(fixture)
