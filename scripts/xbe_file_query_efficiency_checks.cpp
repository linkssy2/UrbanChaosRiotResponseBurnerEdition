// Query bodies, File, and metadata helpers are extracted from production files.cpp.
// Only guest memory/handle services and the mounted volume are fixture services.
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include "kernel/disc.h"

struct Counts { unsigned size = 0, flush = 0, time = 0, directory = 0; } calls;
bool fail_flush = false;
namespace fs {
using namespace std::filesystem;
uintmax_t file_size(const path& p, std::error_code& ec) {
    ++calls.size; return std::filesystem::file_size(p, ec);
}
file_time_type last_write_time(const path& p, std::error_code& ec) {
    ++calls.time; return std::filesystem::last_write_time(p, ec);
}
bool is_directory(const path& p, std::error_code& ec) {
    ++calls.directory; return std::filesystem::is_directory(p, ec);
}
}
int FixtureFlush(FILE* f) {
    ++calls.flush;
    return fail_flush ? EOF : std::fflush(f);
}
using Disc = kernel::Disc;  // Real SDK Entry layout; image I/O is outside this fixture.
struct Volume { uint64_t Time() const { return 132456789012345678ULL; } } volume;
const Volume* MountedDisc() { return &volume; }
enum class ObjType { File, Event };
struct Object {
    ObjType type;
    explicit Object(ObjType t) : type(t) {}
    virtual ~Object() = default;
};
using ObjectRef = std::shared_ptr<Object>;
ObjectRef object;
ObjectRef FromHandle(uint32_t h) { return h == 7 ? object : nullptr; }
constexpr uint32_t kSuccess = 0, kInvalidHandle = 0xC0000008, kInvalidParameter = 0xC000000D;
alignas(8) std::array<uint8_t, 512> memory;
void* HostPtr(uint32_t at) { return memory.data() + at; }
template <typename T> T& G(uint32_t at) { return *reinterpret_cast<T*>(memory.data() + at); }
void W32(uint32_t at, uint32_t v) { std::memcpy(HostPtr(at), &v, sizeof(v)); }
void W64(uint32_t at, uint64_t v) { std::memcpy(HostPtr(at), &v, sizeof(v)); }
uint32_t R32(uint32_t at) { uint32_t v; std::memcpy(&v, HostPtr(at), sizeof(v)); return v; }
uint64_t R64(uint32_t at) { uint64_t v; std::memcpy(&v, HostPtr(at), sizeof(v)); return v; }
#define LOG_FIRST(...) ((void)0)
#define KERNEL_EXPORT(ordinal, name, count) uint32_t name(const uint32_t* a)

namespace original {
// EXTRACTED_ORIGINAL
}
namespace fixed {
// EXTRACTED_FIXED
}

void Require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
enum class Kind {
    Buffered, Empty, ReadOnly, HostNoStream, Directory, DiscFile, DiscDir,
    MissingHost, MissingHostWithStream, FlushFailure, InvalidHandle, WrongObject,
    // These inconsistent fields are not produced by file constructors, but keep
    // the prior fp-over-entry/dir size precedence qualified too.
    StreamWithEntry, StreamWithDir
};
const char* Name(Kind kind) {
    switch (kind) {
        case Kind::Buffered: return "buffered"; case Kind::Empty: return "empty";
        case Kind::ReadOnly: return "read-only"; case Kind::HostNoStream: return "host-no-stream";
        case Kind::Directory: return "directory"; case Kind::DiscFile: return "disc-file";
        case Kind::DiscDir: return "disc-directory"; case Kind::MissingHost: return "missing-host";
        case Kind::MissingHostWithStream: return "missing-host-open-stream";
        case Kind::FlushFailure: return "flush-error"; case Kind::InvalidHandle: return "invalid-handle";
        case Kind::WrongObject: return "wrong-object"; case Kind::StreamWithEntry: return "stream-entry";
        case Kind::StreamWithDir: return "stream-directory";
    }
    return "unknown";
}
constexpr uint32_t output = 64, iosb = 256, output_length = 80;
constexpr uint64_t position = 0x12345678ABCDEF01ULL, disc_size = 0xFFFFFFF0ULL;
struct Snapshot {
    uint32_t status;
    std::array<uint8_t, 512> bytes;
    Counts work;
    uintmax_t disk_before = 0, disk_after = 0;
    bool valid, stream, dir, disc, host_missing;
};
bool IsMetadata(uint32_t cls) { return cls == 4 || cls == 5 || cls == 34; }
bool IsSupported(uint32_t cls) { return IsMetadata(cls) || cls == 14 || cls == 16 || cls == 22; }
uint32_t ResultLength(uint32_t cls) {
    switch (cls) {
        case 4: return 0x28; case 5: return 0x18; case 14: return 8;
        case 34: return 0x38; case 16: case 22: return 4;
        default: return 0;
    }
}

template <typename File, typename Query>
Snapshot Execute(Kind kind, uint32_t cls, bool with_iosb, const fs::path& root, Query query) {
    const auto path = root / "host.bin";
    const auto missing = root / "does-not-exist";
    std::array<char, 8192> buffer{};
    auto file = std::make_shared<File>();
    file->host = path;
    file->pos = position;
    Disc::Entry entry;
    entry.size = uint32_t(disc_size);
    file->dir = kind == Kind::Directory || kind == Kind::DiscDir || kind == Kind::StreamWithDir;
    entry.dir = kind == Kind::DiscDir;
    if (kind == Kind::DiscFile || kind == Kind::DiscDir || kind == Kind::StreamWithEntry)
        file->entry = &entry;
    const bool stream = kind == Kind::Buffered || kind == Kind::Empty || kind == Kind::ReadOnly ||
        kind == Kind::MissingHostWithStream || kind == Kind::FlushFailure ||
        kind == Kind::StreamWithEntry || kind == Kind::StreamWithDir;
    // Seed a fresh ordinary file for each query. No installed game data is touched.
    FILE* seed = std::fopen(path.string().c_str(), "wb");
    Require(seed != nullptr, "create fixture file");
    if (kind == Kind::ReadOnly || kind == Kind::HostNoStream) {
        const std::array<char, 4097> data{};
        Require(std::fwrite(data.data(), 1, data.size(), seed) == data.size(), "seed write");
    }
    Require(std::fclose(seed) == 0, "seed close");
    if (stream) {
        file->fp = std::fopen(path.string().c_str(), kind == Kind::ReadOnly ? "rb" : "r+b");
        Require(file->fp != nullptr, "open fixture stream");
        Require(std::setvbuf(file->fp, buffer.data(), _IOFBF, buffer.size()) == 0, "buffered stream");
        if (kind != Kind::ReadOnly && kind != Kind::Empty) {
            const std::array<char, 113> data{};
            Require(std::fwrite(data.data(), 1, data.size(), file->fp) == data.size(), "buffered write");
        }
    }
    if (kind == Kind::Directory) file->host = root;
    const bool host_missing = kind == Kind::MissingHost || kind == Kind::MissingHostWithStream;
    if (host_missing) file->host = missing;
    const bool valid = kind != Kind::InvalidHandle && kind != Kind::WrongObject;
    object = kind == Kind::WrongObject ? std::make_shared<Object>(ObjType::Event) : file;
    fail_flush = kind == Kind::FlushFailure;
    memory.fill(0xA5);
    const std::array<uint32_t, 5> args = {kind == Kind::InvalidHandle ? 99u : 7u,
        with_iosb ? iosb : 0u, output, output_length, cls};
    Snapshot result{};
    result.disk_before = std::filesystem::file_size(path);
    if (stream && kind != Kind::ReadOnly && kind != Kind::Empty)
        Require(result.disk_before == 0, "fixture write must still be buffered before query");
    calls = {};
    result.status = query(args.data());
    result.work = calls;
    result.bytes = memory;
    result.disk_after = std::filesystem::file_size(path);
    result.valid = valid; result.stream = stream; result.dir = file->dir;
    result.disc = file->entry != nullptr; result.host_missing = host_missing;
    Require(file->pos == position, "query changed virtual position");
    if (stream) {
        const long expected = (kind == Kind::Empty || kind == Kind::ReadOnly) ? 0 : 113;
        Require(std::ftell(file->fp) == expected, "query changed host stream position");
    }
    object.reset();
    file.reset();  // Close while the user-supplied stdio buffer is still alive.
    fail_flush = false;
    return result;
}

uint64_t Read64(const Snapshot& s, uint32_t at) {
    uint64_t value; std::memcpy(&value, s.bytes.data() + at, sizeof(value)); return value;
}
uint32_t Read32(const Snapshot& s, uint32_t at) {
    uint32_t value; std::memcpy(&value, s.bytes.data() + at, sizeof(value)); return value;
}
void CheckResult(const Snapshot& s, Kind kind, uint32_t cls, bool with_iosb, bool optimized) {
    const std::string label = std::string(Name(kind)) + "/" + std::to_string(cls);
    Require(s.status == (!s.valid ? kInvalidHandle : IsSupported(cls) ? kSuccess : kInvalidParameter), label + " status");
    if (!s.valid) {
        for (auto b : s.bytes) Require(b == 0xA5, label + " invalid handle wrote memory");
        Require(s.work.size + s.work.flush + s.work.time + s.work.directory == 0, label + " invalid handle host I/O");
        return;
    }
    for (uint32_t i = 0; i < s.bytes.size(); ++i) {
        const bool output_region = i >= output && i < output + output_length;
        const bool status_region = with_iosb && IsSupported(cls) && i >= iosb && i < iosb + 8;
        if (!output_region && !status_region) Require(s.bytes[i] == 0xA5, label + " memory guard");
    }
    if (with_iosb && IsSupported(cls)) {
        Require(Read32(s, iosb) == kSuccess && Read32(s, iosb + 4) == ResultLength(cls), label + " IOSB/return length");
    }
    for (uint32_t i = ResultLength(cls); i < output_length; ++i)
        Require(s.bytes[output + i] == 0, label + " zeroed output tail");
    if (cls == 14) Require(Read64(s, output) == position, label + " 64-bit file position");
    if (cls == 16 || cls == 22) Require(Read32(s, output) == 0, label + " unchanged zero mode result");
    if (cls == 4 || cls == 34) {
        for (int i = 1; i < 4; ++i) Require(Read64(s, output + i * 8) == Read64(s, output), label + " timestamps differ");
        if (s.disc) Require(Read64(s, output) == volume.Time(), label + " disc timestamp");
        const bool returned_dir = cls == 34 ? kind == Kind::Directory || kind == Kind::DiscDir : s.dir;
        Require(Read32(s, output + (cls == 4 ? 0x20 : 0x30)) == (returned_dir ? 0x10u : 0x80u), label + " attributes");
    }
    if (cls == 5 || cls == 34) {
        uint64_t expected;
        const bool disc_size_used = s.disc && (cls == 34 || !s.stream);
        if (disc_size_used) expected = kind == Kind::DiscDir ? 0 : disc_size;
        else if (kind == Kind::Directory) expected = 0;
        else if (s.host_missing) expected = std::numeric_limits<uint64_t>::max();
        else expected = s.disk_after;
        const uint32_t at = output + (cls == 5 ? 0 : 0x20);
        Require(Read64(s, at + 8) == expected, label + " EOF size (including metadata error)");
        Require(Read64(s, at) == ((expected + 0xFFF) & ~0xFFFull), label + " allocation size");
        if (cls == 5) {
            Require(Read32(s, output + 0x10) == 1, label + " links");
            Require(s.bytes[output + 0x14] == 0 && s.bytes[output + 0x15] == s.dir, label + " deletion/directory fields");
        }
    }
    Require(s.work.flush == (s.stream && (!optimized || IsMetadata(cls)) ? 1u : 0u), label + " flush count");
    unsigned expected_size = 0;
    if (!optimized) expected_size = (!s.dir && !s.disc ? 1u : 0u) + (s.stream ? 1u : 0u);
    else if (cls == 5) expected_size = s.stream || (!s.dir && !s.disc) ? 1u : 0u;
    if (cls == 34 && !s.disc && kind != Kind::Directory) ++expected_size;
    Require(s.work.size == expected_size, label + " size count");
    Require(s.work.time == ((cls == 4 || cls == 34) && !s.disc ? 1u : 0u), label + " time count");
    Require(s.work.directory == (cls == 34 && !s.disc ? 1u : 0u), label + " directory count");
    if (s.stream && kind != Kind::ReadOnly && kind != Kind::Empty) {
        const bool should_flush = kind != Kind::FlushFailure && (!optimized || IsMetadata(cls));
        Require(s.disk_after == (should_flush ? 113u : 0u), label + " buffered-write visibility");
    }
}

void CheckParity(Snapshot old, Snapshot fresh, uint32_t cls) {
    Require(old.status == fresh.status, "query status parity");
    if (old.valid && (cls == 4 || cls == 34) && !old.disc) {
        // Separate real files/flushes have separate wall-clock timestamps. Check
        // each four-field group above, then allow at most one second between
        // independently executed old/fixed host queries. All other bytes match.
        const uint64_t a = Read64(old, output), b = Read64(fresh, output);
        Require((a > b ? a - b : b - a) < 10000000ULL, "host timestamp parity");
        for (uint32_t i = output; i < output + 32; ++i) old.bytes[i] = fresh.bytes[i] = 0;
    }
    Require(old.bytes == fresh.bytes, "guest output/IOSB byte parity");
}

int main(int argc, char** argv) {
    try {
        const fs::path root = std::filesystem::current_path() / "query-fixture-data";
        std::filesystem::create_directories(root);
        if (argc > 1) {
            const bool optimized = std::string(argv[1]) == "--zero-work-fixed";
            auto s = optimized ? Execute<fixed::File>(Kind::Buffered, 14, true, root, fixed::NtQueryInformationFile) :
                Execute<original::File>(Kind::Buffered, 14, true, root, original::NtQueryInformationFile);
            std::cout << "{\"size_calls\":" << s.work.size << ",\"flush_calls\":" << s.work.flush << "}\n";
            return s.work.size == 0 && s.work.flush == 0 ? 0 : 77;
        }
        constexpr Kind kinds[] = {Kind::Buffered, Kind::Empty, Kind::ReadOnly, Kind::HostNoStream,
            Kind::Directory, Kind::DiscFile, Kind::DiscDir, Kind::MissingHost, Kind::MissingHostWithStream,
            Kind::FlushFailure, Kind::InvalidHandle, Kind::WrongObject, Kind::StreamWithEntry, Kind::StreamWithDir};
        constexpr uint32_t classes[] = {4, 5, 14, 16, 22, 34, 0, 99, 0xFFFFFFFF};
        unsigned cases = 0;
        uint64_t old_size = 0, new_size = 0, old_flush = 0, new_flush = 0;
        for (auto kind : kinds) for (auto cls : classes) for (bool with_iosb : {false, true}) {
            auto before = Execute<original::File>(kind, cls, with_iosb, root, original::NtQueryInformationFile);
            auto after = Execute<fixed::File>(kind, cls, with_iosb, root, fixed::NtQueryInformationFile);
            CheckResult(before, kind, cls, with_iosb, false);
            CheckResult(after, kind, cls, with_iosb, true);
            CheckParity(before, after, cls);
            ++cases; old_size += before.work.size; new_size += after.work.size;
            old_flush += before.work.flush; new_flush += after.work.flush;
        }
        std::cout << "{\"parity_cases\":" << cases << ",\"old_size_calls\":" << old_size
            << ",\"fixed_size_calls\":" << new_size << ",\"old_flush_calls\":" << old_flush
            << ",\"fixed_flush_calls\":" << new_flush << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
