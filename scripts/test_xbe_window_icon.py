"""Read the built EXE's icon resources without executing it or opening a window."""
from pathlib import Path
import argparse
import ctypes as c
from ctypes import wintypes as w
import os
import struct


def verify(executable, icon):
    assert os.name == 'nt', 'Run this resource check through Windows APIs.'
    source = Path(icon).read_bytes()
    reserved, kind, count = struct.unpack_from('<3H', source)
    assert reserved == 0 and kind == 1 and count > 0
    assert len(source) >= 6 + count * 16

    kernel = c.WinDLL('kernel32', use_last_error=True)
    user = c.WinDLL('user32', use_last_error=True)
    kernel.LoadLibraryExW.argtypes = (w.LPCWSTR, w.HANDLE, w.DWORD)
    kernel.LoadLibraryExW.restype = w.HMODULE
    kernel.FreeLibrary.argtypes = (w.HMODULE,)
    kernel.FreeLibrary.restype = w.BOOL
    kernel.FindResourceW.argtypes = (w.HMODULE, c.c_void_p, c.c_void_p)
    kernel.FindResourceW.restype = c.c_void_p
    kernel.SizeofResource.argtypes = (w.HMODULE, c.c_void_p)
    kernel.SizeofResource.restype = w.DWORD
    kernel.LoadResource.argtypes = (w.HMODULE, c.c_void_p)
    kernel.LoadResource.restype = c.c_void_p
    kernel.LockResource.argtypes = (c.c_void_p,)
    kernel.LockResource.restype = c.c_void_p
    user.LoadIconW.argtypes = (w.HINSTANCE, c.c_void_p)
    user.LoadIconW.restype = w.HICON
    user.LoadImageW.argtypes = (w.HINSTANCE, c.c_void_p, w.UINT, c.c_int, c.c_int, w.UINT)
    user.LoadImageW.restype = w.HANDLE
    user.DestroyIcon.argtypes = (w.HICON,)
    user.DestroyIcon.restype = w.BOOL
    # Map resources only: never load the executable's code or run its entrypoint.
    module = kernel.LoadLibraryExW(str(Path(executable).resolve()), None, 0x22)
    assert module, c.WinError(c.get_last_error())
    try:
        def resource(kind, name):
            found = kernel.FindResourceW(module, name, kind)
            assert found, f'Missing resource type {kind}, ID {name}'
            size = kernel.SizeofResource(module, found)
            loaded = kernel.LoadResource(module, found)
            pointer = kernel.LockResource(loaded) if loaded else None
            assert size and pointer, c.WinError(c.get_last_error())
            return c.string_at(pointer, size)

        group = resource(14, 1)
        assert struct.unpack_from('<3H', group) == (0, 1, count)
        assert len(group) == 6 + count * 14
        sizes = []
        for index in range(count):
            entry = source[6 + index * 16:6 + (index + 1) * 16]
            member = group[6 + index * 14:6 + (index + 1) * 14]
            assert member[:12] == entry[:12], f'Changed icon descriptor {index}'
            length, offset = struct.unpack_from('<II', entry, 8)
            assert offset >= 6 + count * 16 and offset + length <= len(source)
            name, = struct.unpack_from('<H', member, 12)
            assert resource(3, name) == source[offset:offset + length], f'Changed icon pixels {index}'
            sizes.append(entry[0] or 256)
        # This is the same resource lookup used by both production window classes.
        assert user.LoadIconW(module, 1), 'Window class LoadIconW failed'
        for size in (16, 32, 48):
            loaded = user.LoadImageW(module, 1, 1, size, size, 0)
            assert loaded, f'Unable to load {size}-pixel icon'
            assert user.DestroyIcon(loaded)
        print(f'PASS window icon: resource 1, {sizes}, exact existing artwork; Win32 icon loading passed')
    finally:
        assert kernel.FreeLibrary(module)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('icon', type=Path)
    args = parser.parse_args()
    verify(args.executable, args.icon)
