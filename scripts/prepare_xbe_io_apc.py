"""Connect the XAPI overlapped-I/O completion adapter to the host APC queue."""
from pathlib import Path


def prepare(sdk, area):
    source = (sdk/'runtime/kernel/xboxkrnl.cpp').read_text()
    start = source.index('KERNEL_EXPORT(301, RtlNtStatusToDosError, 1) {')
    end = source.index('\n// exceptions, debug, shutdown', start)
    converter = source[start:end].replace(
        'KERNEL_EXPORT(301, RtlNtStatusToDosError, 1)',
        'static uint32_t NtStatusToDosError(uint32_t status)').replace('a[0]', 'status')
    adapter = '''
KERNEL_EXPORT(301, RtlNtStatusToDosError, 1) { return NtStatusToDosError(a[0]); }

// ReadFileEx passes its completion routine as APC context and OVERLAPPED as IOSB.
KERNEL_EXPORT(232, NtUserIoApcDispatcher, 3) {
    uint32_t callback = a[0], iosb = a[1];
    uint32_t status = R32(iosb);
    uint32_t error = int32_t(status) < 0 ? NtStatusToDosError(status) : 0;
    uint32_t bytes = int32_t(status) < 0 ? 0 : R32(iosb + 4);
    CallGuest(ctx, callback, {error, bytes, iosb});
    return 0;
}
'''
    (area/'src/compat/xboxkrnl.cpp').write_text(source[:start]+converter+adapter+source[end:])
    generated = (area/'generated/game/x86_recomp.61.cpp').read_text()
    begin = generated.index('X86_FUNC(__imp__sub_00251690) {')
    finish = generated.index('\nX86_WEAK_FUNC(sub_00251690', begin)
    callback = generated[begin:finish]
    dispatch = (sdk/'runtime/cpu/dispatch.cpp').read_text()
    begin = dispatch.index('uint32_t CallGuest(X86Context& ctx, uint32_t fn, const uint32_t* args, size_t count) {')
    finish = dispatch.index('\n}  // namespace kernel', begin)
    call_guest = dispatch[begin:finish]
    fixture = (Path(__file__).with_name('test_xbe_io_apc.cpp')).read_text()
    fixture = fixture.replace('// GENERATED_CALLBACK', callback)
    fixture = fixture.replace('// GENERATED_CALL_GUEST', call_guest)
    fixture = fixture.replace('// GENERATED_ADAPTER', converter+adapter)
    (area/'src/compat/test_io_apc.cpp').write_text(fixture)
