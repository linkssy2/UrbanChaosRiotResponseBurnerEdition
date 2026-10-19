"""Index immutable Xbox format metadata without changing returned object identity."""
from pathlib import Path
from prepare_xbe_perf import once, function

LOOKUP='''
// Index existing entries; first duplicate ID retains the original scan's result.
constexpr auto kFormatLookup = [] {
    std::array<const FormatInfo*, 256> lookup{};
    for (const auto& e : kFormats)
        if (e.id < lookup.size() && !lookup[e.id]) lookup[e.id] = &e.info;
    return lookup;
}();
'''
OLD='''const FormatInfo& Format(uint32_t f) {
    for (auto& e : kFormats)
        if (e.id == f) return e.info;
    return kUnknown;
}'''
NEW='''const FormatInfo& Format(uint32_t f) {
    if (f < kFormatLookup.size()) {
        const FormatInfo* info = kFormatLookup[f];
        return info ? *info : kUnknown;
    }
    for (auto& e : kFormats)
        if (e.id == f) return e.info;
    return kUnknown;
}'''

def transform(source):
    if LOOKUP in source:
        assert source.count(LOOKUP)==source.count(NEW)==1
        assert source.count('constexpr Entry kFormats[]')==source.count('#include <array>')==1
        return source
    source=once(source,'#include <algorithm>','#include <algorithm>\n#include <array>')
    source=once(source,'const Entry kFormats[]','constexpr Entry kFormats[]')
    source=once(source,'\nuint32_t Log2(',LOOKUP+'\nuint32_t Log2(')
    return once(source,OLD,NEW)

def undo(source):
    source=once(source,LOOKUP,'')
    source=once(source,NEW,OLD)
    source=once(source,'constexpr Entry kFormats[]','const Entry kFormats[]')
    return once(source,'#include <algorithm>\n#include <array>','#include <algorithm>')

def fixture(before,after):
    extra='''
    {0x06, {"duplicate_existing", 8, false, false, false, Host::None}},
    {0x08, {"first_gap", 8, false, false, false, Host::BGRA8}},
    {0x08, {"duplicate_gap", 32, true, false, true, Host::R32F}},
    {0x123, {"first_high", 16, true, false, false, Host::BGRA8}},
    {0x123, {"duplicate_high", 32, false, false, false, Host::None}},
    {0xFFFFFFFF, {"maximum_id", 8, true, false, false, Host::BGRA8}},'''
    end=before.index('\n};',before.index('const Entry kFormats[]'))
    synthetic=before[:end]+extra+before[end:]
    text=Path(__file__).with_name('test_xbe_texture_formats.cpp').read_text()
    for label,source in [('BEFORE',before),('AFTER',after),('SYNTHETIC',transform(synthetic))]:
        table=source[source.index('const FormatInfo kUnknown'):source.index('\nuint32_t Log2(')]
        text=once(text,'/* '+label+' */',table+'\n'+function(source,'const FormatInfo& Format('))
    return text

def prepare(sdk,area):
    before=(Path(sdk)/'runtime/gpu/texture.cpp').read_text()
    after=transform(before)
    assert transform(after)==after and undo(after)==before
    output=Path(area)/'src/compat'
    (output/'texture.cpp').write_text(after)
    (output/'test_xbe_texture_formats.cpp').write_text(fixture(before,after))
