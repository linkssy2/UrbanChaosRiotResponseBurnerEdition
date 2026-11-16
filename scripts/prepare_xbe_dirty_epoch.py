"""Reuse exact dirty epochs between writes, keeping the original map and mutex."""
from pathlib import Path
import re

INCLUDE='#include "xbe_dirty_epoch.h"\n'
WRITE='    g_dirty[data] = ++g_epoch;'
READ='''    std::lock_guard<std::mutex> lock(g_dirty_lock);
    auto it = g_dirty.find(data);
    return it == g_dirty.end() ? 0 : it->second;'''
NEW_WRITE='''    uc_dirty_epoch::Write publish;
    g_dirty[data] = ++g_epoch;'''
NEW_READ='''    uint64_t value;
    if(uc_dirty_epoch::TryRead(data,value))return value;
    std::lock_guard<std::mutex> lock(g_dirty_lock);
    auto it = g_dirty.find(data);
    value=it == g_dirty.end() ? 0 : it->second;
    uc_dirty_epoch::StoreLocked(data,value);
    return value;'''


def once(source,old,new):
    assert source.count(old)==1,(old,source.count(old))
    return source.replace(old,new,1)


def body(source,signature):
    start=source.index(signature)
    return source[start:source.index('\n}',start)+2]


def original(source):
    if INCLUDE not in source:return source
    source=once(source,INCLUDE,'')
    source=once(source,NEW_WRITE,WRITE)
    return once(source,NEW_READ,READ)


def audit(source):
    # The complete named-owner census excludes clear/reset/erase/swap and
    # escaped map references. New mutations require a fresh ownership review.
    uses=[line.strip() for line in source.splitlines() if re.search(r'\bg_dirty\b',line)]
    assert uses==[
        'std::unordered_map<uint32_t, uint64_t> g_dirty;',WRITE.strip(),
        'auto it = g_dirty.find(data);','return it == g_dirty.end() ? 0 : it->second;'],uses
    epochs=[line.strip() for line in source.splitlines() if re.search(r'\bg_epoch\b',line)]
    assert epochs==['uint64_t g_epoch = 1;',WRITE.strip()],epochs
    mark=body(source,'void MarkDirty(uint32_t res) {')
    read=body(source,'uint64_t DirtyEpoch(uint32_t data) {')
    assert mark=='''void MarkDirty(uint32_t res) {
    if (!res) return;
    uint32_t data = mem::VirtualFromPhysical(kernel::R32(res + 4));
    std::lock_guard<std::mutex> lock(g_dirty_lock);
    g_dirty[data] = ++g_epoch;
}'''
    assert read=='uint64_t DirtyEpoch(uint32_t data) {\n'+READ+'\n}'


def transform(source):
    clean=original(source)
    audit(clean)
    result=once(clean,WRITE,NEW_WRITE)
    result=once(result,READ,NEW_READ)
    result=INCLUDE+result
    assert original(result)==clean
    return result


def prepare(area):
    area,local=Path(area),Path(__file__).parent
    path=area/'src/compat/texture.cpp'
    clean=original(path.read_text())
    changed=transform(clean)
    assert transform(changed)==changed
    header=(local/'xbe_dirty_epoch.h').read_text()
    test_header=once(header,'    // Verify revision again after matching the numeric TLS entry.',
                     '    ::test::Recheck(); // deterministic fixture scheduling boundary')
    test=(local/'test_xbe_dirty_epoch.cpp').read_text()
    names=('void MarkDirty(uint32_t res) {','uint64_t DirtyEpoch(uint32_t data) {')
    test=once(test,'// BEFORE_FUNCTIONS','\n'.join(body(clean,name) for name in names))
    test=once(test,'// AFTER_FUNCTIONS','\n'.join(body(changed,name) for name in names))
    (area/'src/common/xbe_dirty_epoch.h').write_text(header)
    (area/'src/compat/test_dirty_epoch_cache.h').write_text(test_header)
    (area/'src/compat/test_dirty_epoch.cpp').write_text(test)
    path.write_text(changed)
