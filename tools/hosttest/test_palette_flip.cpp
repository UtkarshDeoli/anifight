// Host test for the .act palette flip in engine/character.
//
// An .act stores its 256 RGB entries in the opposite index order from the
// palettes the sprite bank itself carries: for every character here that has
// both, bank_palette[i] == act[255 - i], all 256 entries (tools/palcheck.py
// re-runs that comparison against local_chars). Sprite pixels, though, are
// indexed in the *bank's* order. So an .act has to be reversed before it joins
// the bank's table, and a character's sprite that falls through to its
// character palette draws blue/green confetti instead of a fighter if it is
// not. That was the bug; this test pins the fix.
//
// It is a synthetic fixture on purpose. A real character would drag an .sff,
// an .air and a whole .cns into a test whose only subject is 768 bytes of
// colour, and it would SKIP on every clone that has no local_chars -- which is
// every CI clone. Instead the test writes a two-line .def and two .act files
// into a temporary directory at run time, so it always runs, and asserts on the
// exact bytes `Character::load` handed the bank:
//
//   * the 768-byte .act is reversed, entry for entry;
//   * the 772-byte variant (a 4-byte header in front) is reversed the same way,
//     and its header is ignored, because the payload is the last 768 bytes.
//
// The shim under tools/hosttest/shim records those bytes; see
// shim/palette_recorder.h for why the recording exists and why it is opt-in.
//
// Build and run:
//   clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//       -o test_palette_flip tools/hosttest/test_palette_flip.cpp \
//       tools/hosttest/shim/sprite_bank_stub.cpp \
//       app/src/main/cpp/engine/character.cpp app/src/main/cpp/engine/anim.cpp \
//       app/src/main/cpp/engine/cns.cpp app/src/main/cpp/engine/cmd.cpp \
//       app/src/main/cpp/engine/expr.cpp \
//       app/src/main/cpp/audio/snd_bank.cpp app/src/main/cpp/core/ini.cpp

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "engine/character.h"

#include "palette_recorder.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const std::string& what) {
    ++checks;
    if (condition) return;
    ++failures;
    std::printf("FAIL: %s\n", what.c_str());
}

// Reads a character's files straight off the filesystem, rooted at the folder
// holding its .def. The engine's own FileSource is backed by the APK's asset
// manager, which is not available here; the interface is three lines, so the
// host test brings its own. (Mirrors test_statemachine's.)
class DirectorySource : public af::FileSource {
public:
    explicit DirectorySource(std::string root) : root_(std::move(root)) {}

    std::unique_ptr<af::FileData> read(const std::string& path) override {
        std::ifstream file(root_ + "/" + path, std::ios::binary);
        if (!file) return nullptr;
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        return std::make_unique<af::FileData>(path, std::move(data));
    }

    bool exists(const std::string& path) override {
        struct stat info {};
        return stat((root_ + "/" + path).c_str(), &info) == 0;
    }

private:
    std::string root_;
};

// One .act entry. Deliberately asymmetric -- entry 0 is nothing like entry 255
// -- so that a pass-through (unflipped) engine cannot accidentally satisfy the
// reversal check. If the pattern were palindromic the test would pass with the
// fix removed, which is exactly the failure mode it exists to prevent.
std::array<uint8_t, 3> actEntry(int index) {
    return {static_cast<uint8_t>(index), static_cast<uint8_t>(255 - index),
            static_cast<uint8_t>((index * 7) & 0xff)};
}

std::vector<uint8_t> makeAct(int entries) {
    std::vector<uint8_t> bytes;
    bytes.reserve(static_cast<size_t>(entries) * 3);
    for (int i = 0; i < entries; ++i) {
        const std::array<uint8_t, 3> rgb = actEntry(i);
        bytes.insert(bytes.end(), rgb.begin(), rgb.end());
    }
    return bytes;
}

bool writeFile(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return file.good();
}

bool writeFile(const std::filesystem::path& path, const std::string& text) {
    return writeFile(path, std::vector<uint8_t>(text.begin(), text.end()));
}

// The whole payload of the .act as the engine must have received it: entry i
// of the bank's table is entry 255 - i of the file. `raw` is the trailing 768
// bytes of the file, header already stripped.
bool isReversed(const std::array<uint8_t, 768>& got, const std::vector<uint8_t>& raw) {
    for (size_t i = 0; i < 256; ++i) {
        for (size_t channel = 0; channel < 3; ++channel) {
            if (got[i * 3 + channel] != raw[(255 - i) * 3 + channel]) return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    using std::filesystem::path;

    const path dir = std::filesystem::temp_directory_path() / "anifight_palette_flip";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::printf("FAIL: cannot create %s: %s\n", dir.string().c_str(), ec.message().c_str());
        return 1;
    }

    // pal1 is a plain 768-byte .act; pal2 carries the 772-byte variant's 4-byte
    // header, which the engine must skip by reading the last 768 bytes.
    const std::vector<uint8_t> pal1 = makeAct(256);
    std::vector<uint8_t> pal2 = {0x00, 0x00, 0x00, 0x00};
    const std::vector<uint8_t> pal2Payload = makeAct(256);
    pal2.insert(pal2.end(), pal2Payload.begin(), pal2Payload.end());

    const std::string def =
        "[Info]\n"
        "name = \"Palette Flip Probe\"\n"
        "\n"
        "[Files]\n"
        "pal1 = pal1.act\n"
        "pal2 = pal2.act\n";

    if (!writeFile(dir / "probe.def", def) || !writeFile(dir / "pal1.act", pal1) ||
        !writeFile(dir / "pal2.act", pal2)) {
        std::printf("FAIL: cannot write fixture under %s\n", dir.string().c_str());
        return 1;
    }

    af::palette_stub::clear();
    af::palette_stub::record(true);

    DirectorySource files(dir.string());
    af::Character character;
    std::string error;
    const bool loaded = character.load(files, "probe.def", &error);
    check(loaded, "the fixture .def should load: " + error);
    if (!loaded) {
        af::palette_stub::record(false);
        return 1;
    }

    const std::vector<std::array<uint8_t, 768>>& recorded = af::palette_stub::recorded();
    // Both palN keys name a readable .act, so both reach the bank, in the order
    // the .def lists them. A count short of two means a palette never loaded and
    // the reversal checks below would be checking nothing.
    check(recorded.size() == 2, "both .act files should be offered to the sprite bank");

    if (recorded.size() >= 1) {
        // The heart of it: the fixture is asymmetric, so this fails both when
        // the reversal is removed (the bytes arrive in file order) and when it
        // is applied twice (they arrive doubly reversed).
        check(isReversed(recorded[0], pal1),
              "the 768-byte .act should reach the bank reversed, entry for entry");
        // Guard the guard: prove the fixture could tell reversed from un-reversed
        // at all. Without this a palindromic fixture would make the check above
        // vacuous and nobody would notice.
        const bool entryZeroDiffers =
            recorded[0][0] != pal1[0] || recorded[0][1] != pal1[1] || recorded[0][2] != pal1[2];
        check(entryZeroDiffers,
              "the fixture's first entry must differ from its last, or the reversal check proves nothing");
    }
    if (recorded.size() >= 2) {
        check(isReversed(recorded[1], pal2Payload),
              "the 772-byte .act should be reversed the same way, its 4-byte header ignored");
    }

    af::palette_stub::record(false);

    std::printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
