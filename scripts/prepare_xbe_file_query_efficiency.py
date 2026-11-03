"""Avoid host metadata work for Xbox file queries that do not return metadata.

Run after prepare_xbe_language, which regenerates src/compat/files.cpp. This is
the bounded query-class optimization used by the TGP XboxXBERecomp-based
Darkwatch runtime, adapted to Urban Chaos's existing query contract. It changes
no file completion, locking, mode-information, or guest return conventions.
"""
from pathlib import Path
import argparse


BEGIN = "KERNEL_EXPORT(211, NtQueryInformationFile, 5) {"
END = "\nKERNEL_EXPORT(226, NtSetInformationFile, 5) {"
OLD_METADATA = """    std::error_code ec;
    uint64_t size = f->dir ? 0 : f->entry ? f->entry->size : fs::file_size(f->host, ec);
    if (f->fp) {
        std::fflush(f->fp);
        size = fs::file_size(f->host, ec);
    }
"""
NEW_METADATA = """    std::error_code ec;
    // Position/mode queries use only the handle. Metadata still observes buffered writes.
    if (f->fp && (cls == 4 || cls == 5 || cls == 34)) std::fflush(f->fp);
"""
OLD_STANDARD = """        case 5:  // FileStandardInformation
            W64(out, (size + 0xFFF) & ~0xFFFull);"""
NEW_STANDARD = """        case 5: {  // FileStandardInformation
            // An open host stream takes precedence, as in the original query path.
            uint64_t size = f->fp ? fs::file_size(f->host, ec) :
                f->dir ? 0 : f->entry ? f->entry->size : fs::file_size(f->host, ec);
            W64(out, (size + 0xFFF) & ~0xFFFull);"""
OLD_STANDARD_END = """            IoStatus(iosb, kSuccess, 0x18);
            return kSuccess;
        case 14:"""
NEW_STANDARD_END = """            IoStatus(iosb, kSuccess, 0x18);
            return kSuccess;
        }
        case 14:"""


def query_body(source):
    if source.count(BEGIN) != 1 or source.count(END) != 1:
        raise ValueError("Expected one NtQueryInformationFile and following NtSetInformationFile")
    start = source.index(BEGIN)
    return source[start:source.index(END, start)]


def patch_source(source):
    body = query_body(source)
    result = body
    for old, new in ((OLD_METADATA, NEW_METADATA), (OLD_STANDARD, NEW_STANDARD),
                     (OLD_STANDARD_END, NEW_STANDARD_END)):
        if result.count(old) != 1 or new in result:
            raise ValueError("Unqualified NtQueryInformationFile source: " + old.splitlines()[0])
        result = result.replace(old, new, 1)
    return source.replace(body, result, 1)


def prepare(area):
    path = Path(area) / "src/compat/files.cpp"
    source = path.read_text(encoding="utf-8")
    path.write_text(patch_source(source), encoding="utf-8")
    return path


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--area", type=Path, required=True)
    print(prepare(parser.parse_args().area))
