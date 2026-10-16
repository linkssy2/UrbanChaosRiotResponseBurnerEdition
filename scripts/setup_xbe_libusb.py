"""Pinned upstream Windows USB dependency; no system driver installation."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / '.tools/libusb-1.0.29'
RELEASE = 'https://github.com/libusb/libusb/releases/download/v1.0.29/'
FILES = {
    'include/libusb.h': '8c77e192d53960966c42d5f6ef204e3be88c9ff3ca431acdd5c6039689b3fe0b',
    'VS2022/MS64/dll/libusb-1.0.lib': '821f07f42be6e00e3377b86e40d9eae1e68edc126064016be95bcfaa6f53c29f',
    'VS2022/MS64/dll/libusb-1.0.dll': '5072054cb3002ae071f382ad5c2c2b0092d9451c537d0c13444c2b6f968f7251',
    'COPYING': '5df07007198989c622f5d41de8d703e7bef3d0e79d62e24332ee739a452af62a',
    'libusb-1.0.29.tar.bz2': '5977fc950f8d1395ccea9bd48c06b3f808fd3c2c961b44b0c2e6e29fc3a70a85',
}

def verify():
    for name, digest in FILES.items():
        path = DEST / name
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError('Missing/changed libusb dependency: ' + str(path) + '; run scripts/setup_xbe_libusb.py')

def prepare(area):
    verify()
    output = Path(area) / 'out-msvc'
    output.mkdir(parents=True, exist_ok=True)
    shutil.copy2(DEST / 'VS2022/MS64/dll/libusb-1.0.dll', output)
    # OFF means link the import library, not search PATH via LoadLibrary.
    return ['-DSDL_HIDAPI_LIBUSB=ON', '-DSDL_HIDAPI_LIBUSB_SHARED=OFF',
            '-DLibUSB_INCLUDE_PATH=' + str(DEST / 'include'),
            '-DLibUSB_LIBRARY=' + str(DEST / 'VS2022/MS64/dll/libusb-1.0.lib')]

if __name__ == '__main__':
    DEST.mkdir(parents=True, exist_ok=True)
    archive = DEST / 'libusb-1.0.29.7z'
    urllib.request.urlretrieve(RELEASE + archive.name, archive)
    assert hashlib.sha256(archive.read_bytes()).hexdigest() == '964a38152ca9a104cd00ec8d2f0617b89cd814f9b635e29763c68563d951521d'
    subprocess.run(['7z', 'x', '-y', str(archive), '-o' + str(DEST),
                    'include/libusb.h', 'VS2022/MS64/dll/libusb-1.0.dll',
                    'VS2022/MS64/dll/libusb-1.0.lib'], check=True)
    urllib.request.urlretrieve(RELEASE + 'libusb-1.0.29.tar.bz2', DEST / 'libusb-1.0.29.tar.bz2')
    urllib.request.urlretrieve('https://raw.githubusercontent.com/libusb/libusb/v1.0.29/COPYING', DEST / 'COPYING')
    verify()
    print('PASS pinned libusb DLL, import library, header, license and source archive')
