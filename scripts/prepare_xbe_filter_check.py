"""Exercise production sampler descriptors; no GPU or live Apply claim."""
from pathlib import Path


def prepare(area):
    area=Path(area)
    source=(area/'src/compat/d3d12.cpp').read_text()
    def body(start,end):
        at=source.index(start)
        return source[at:source.index(end,at)]
    production='\n'.join([
        body('float Bits(', '// D3DBLEND,'),
        body('D3D12_COMPARISON_FUNC Cmp(', '// D3DSTENCILOP,'),
        body('D3D12_TEXTURE_ADDRESS_MODE Address(', 'DXGI_FORMAT HostFormat('),
        body('struct DrawTextures {', '\n};')+'\n};',
        body('// D3DTEXF: 1 point, 2 linear, 3 anisotropic', 'uint32_t D3D12Backend::EnsureStageSamplers('),
        body('bool UnitTexcoords(', '// a stage without a texture gets a null view'),
    ])
    shaders=(area/'src/compat/shaders.cpp').read_text()
    at=shaders.index('uint64_t Hash(')
    cache=shaders[at:shaders.index('\n}',at)+2]+'\n'+body(
        'uint32_t D3D12Backend::EnsureStageSamplers(', 'uint32_t SimpleState(')
    fixture=Path(__file__).with_name('test_xbe_filter.cpp').read_text()
    assert fixture.count('// PRODUCTION_SAMPLER')==1
    assert fixture.count('// PRODUCTION_CACHE')==1
    (area/'src/compat/test_filter.cpp').write_text(fixture.replace('// PRODUCTION_SAMPLER',production).replace('// PRODUCTION_CACHE',cache))

