// Host test for the palette *choice* mapping on engine/character:
//
//   Character::paletteChoices()       how many palettes a player may pick between
//   Character::palNoForChoice(int)    a 0-based choice -> the 1-based palNo that
//                                     paletteIndexFor() understands
//   Character::defaultPaletteChoice() which choice is the character's own default
//
// A select screen offers a fixed number of colours and has to translate the one
// the player stopped on back into a `palno`. The two things that make that
// non-trivial, and that this test exists to pin, are:
//
//   * The choices do not come from one place. A character with `pal1`..`pal12`
//     .act files gets them from the populated slots; a character shipping no
//     .act at all gets them from the SFF's own palette table. Both fold into one
//     0-based numbering, and the caller must never have to know which.
//
//   * `paletteIndexFor` *clamps* a palNo that names no .act into the SFF's table
//     rather than failing. So a UI that counted 1..12 blindly would offer the
//     same picture several times and call it several choices -- a character with
//     only pal1, pal3 and pal7 would present twelve "different" colours, nine of
//     them the same fallback. Skipping the empty slots is what prevents that,
//     and it is the property most likely to regress.
//
// Like test_palette_flip this is a synthetic fixture -- a .def and some 768-byte
// .act files written into a temp dir at run time -- so it always runs, on every
// clone, with no local_chars and no art. It links shim/sprite_bank_palette_stub
// rather than shim/sprite_bank_stub: the latter returns -1 from every addPalette,
// which leaves actPalettes_ empty and makes the mapping untestable. See that
// file for why the two shims differ.
//
// Build and run:
//   clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//       -o test_palette_choice tools/hosttest/test_palette_choice.cpp \
//       tools/hosttest/shim/sprite_bank_palette_stub.cpp \
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

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const std::string& what) {
    ++checks;
    if (condition) return;
    ++failures;
    std::printf("FAIL: %s\n", what.c_str());
}

// Reads a character's files straight off the filesystem, rooted at the temp dir
// holding every scenario. The engine's own FileSource is backed by the APK asset
// manager, which is not available here; the interface is three lines, so the
// host test brings its own. (Mirrors test_palette_flip's.)
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

// One 768-byte .act. The contents do not matter here -- this test is about which
// slots exist, not what colour they hold -- so any 768 bytes will do.
std::vector<uint8_t> makeAct() {
    std::vector<uint8_t> bytes(768);
    for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<uint8_t>(i & 0xff);
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

// A .def naming exactly the palN slots given, each with its own .act beside it,
// plus an optional pal.defaults. `pals` is the list of populated pal numbers; the
// gaps between them are the whole point of the gaps scenario.
bool writeCharacter(const std::filesystem::path& dir, const std::vector<int>& pals,
                    const std::string& palDefaults = {}) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;

    std::string def = "[Info]\nname = \"Palette Choice Probe\"\n";
    if (!palDefaults.empty()) def += "pal.defaults = " + palDefaults + "\n";
    def += "\n[Files]\n";
    for (const int n : pals) {
        const std::string name = "pal" + std::to_string(n) + ".act";
        def += "pal" + std::to_string(n) + " = " + name + "\n";
        if (!writeFile(dir / name, makeAct())) return false;
    }
    return writeFile(dir / "probe.def", def);
}

// Loads a character whose files live under `sub` of the temp root. Fills
// `errorOut` on failure and returns nullptr so a caller can bail cleanly.
std::unique_ptr<af::Character> loadCharacter(af::FileSource& files, const std::string& sub) {
    auto character = std::make_unique<af::Character>();
    std::string error;
    if (!character->load(files, sub + "/probe.def", &error)) {
        check(false, "the fixture " + sub + " should load: " + error);
        return nullptr;
    }
    return character;
}

// A 768-byte palette to hand the bank directly, standing in for the palette
// nodes an SFF v2 character carries. A real bank fills its table during
// sprites_.load; with no art to load, this seeds the same post-load state so the
// "no .act files" branch can be exercised.
void seedBankPalettes(af::Character& character, int count) {
    const std::vector<uint8_t> bytes = makeAct();
    for (int i = 0; i < count; ++i) character.sprites().addPalette(bytes.data());
}

}  // namespace

int main() {
    using std::filesystem::path;

    const path root = std::filesystem::temp_directory_path() / "anifight_palette_choice";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        std::printf("FAIL: cannot create %s: %s\n", root.string().c_str(), ec.message().c_str());
        return 1;
    }

    // Build every fixture up front; a write failure here means the checks below
    // would be checking nothing, so bail rather than report misleading passes.
    const bool built = writeCharacter(root / "gaps", {1, 3, 7}) &&
                       writeCharacter(root / "default", {1, 3}, "3") &&
                       writeCharacter(root / "default_missing", {1, 2}, "3") &&
                       writeCharacter(root / "banktable", {}) &&
                       writeCharacter(root / "empty", {}) &&
                       writeCharacter(root / "mixed", {1, 3});
    if (!built) {
        std::printf("FAIL: cannot write fixtures under %s\n", root.string().c_str());
        return 1;
    }

    DirectorySource files(root.string());

    // ---------------------------------------------------------------- 1. gaps
    //
    // pal1, pal3, pal7. The empty slots (2, 4..6, 8..12) must not each become a
    // choice: paletteIndexFor would clamp every one of them into the SFF table,
    // so a UI counting 1..12 would show the same fallback picture twelve times.
    if (auto character = loadCharacter(files, "gaps")) {
        check(character->paletteChoices() == 3,
              "a character with pal1/pal3/pal7 has three choices, not twelve");
        check(character->palNoForChoice(0) == 1, "choice 0 of {1,3,7} is palNo 1");
        check(character->palNoForChoice(1) == 3, "choice 1 of {1,3,7} is palNo 3, skipping the empty pal2");
        check(character->palNoForChoice(2) == 7, "choice 2 of {1,3,7} is palNo 7, skipping pal4..6");

        // Round trip over every choice: strictly increasing (distinct choices
        // give distinct palNos) and each one lands on a populated slot -- i.e.
        // paletteIndexFor() answers with one of the .act entries, not a clamp
        // into the SFF table. The seeded bank has exactly three entries (0,1,2),
        // so a choice that had fallen through to the table would repeat an index.
        const int choices = character->paletteChoices();
        int previousPalNo = 0;
        int previousIndex = -1;
        bool increasing = true;
        bool indicesIncreasing = true;
        for (int c = 0; c < choices; ++c) {
            const int palNo = character->palNoForChoice(c);
            if (palNo <= previousPalNo) increasing = false;
            previousPalNo = palNo;
            const int index = character->paletteIndexFor(palNo);
            if (index <= previousIndex) indicesIncreasing = false;
            previousIndex = index;
        }
        check(increasing, "distinct choices map to strictly increasing palNos");
        check(indicesIncreasing,
              "distinct choices map to distinct bank entries, so each is a different picture");

        // Clamping: a choice outside [0, choices) must land on the first or the
        // last real palette, never on an index the caller could not draw.
        check(character->palNoForChoice(-1) == 1, "a negative choice clamps to the first palette");
        check(character->palNoForChoice(99) == 7, "a too-large choice clamps to the last palette");

        // The `(group, item)` spelling RemapPal uses. Group 1 is exactly the
        // palno/`.act` numbering above -- the corpus's `source = 1,3` must keep
        // meaning the same palette as `pal3` -- while a group this bank has no
        // palette-node keys for answers -1 rather than inventing an entry.
        check(character->paletteIndexFor(1, 3) == character->paletteIndexFor(3),
              "group 1 item 3 is the same entry as palNo 3");
        check(character->paletteIndexFor(1, 7) == character->paletteIndexFor(7),
              "and the same holds for every populated slot");
        check(character->paletteIndexFor(2, 3) == -1,
              "a non-1 group has no palette node in an .act-only bank");

        // The default (pal.defaults absent => 1) is the first choice here.
        check(character->defaultPaletteChoice() == 0,
              "default palNo 1 is choice 0 when pal1 is present");
    }

    // ------------------------------------------------------------ 2. default
    //
    // pal.defaults names pal3, and pal3 sits at choice index 1 (pal1 is choice
    // 0). A picker that opened on the raw index 3 would be out of range; the
    // point of defaultPaletteChoice is that it hands back the *choice*, not the
    // palNo, and the two only coincide when every slot is filled.
    if (auto character = loadCharacter(files, "default")) {
        check(character->paletteChoices() == 2, "pal1/pal3 gives two choices");
        check(character->palNoForChoice(1) == 3, "choice 1 is the palNo the default names");
        check(character->defaultPaletteChoice() == 1,
              "pal.defaults = 3 must map to choice 1, not 0 or the raw palNo 3");
    }

    // The default names a palette that is not among the choices at all, which is
    // the normal case when the default points at an SFF slot rather than an .act.
    // The answer has to be a valid index, so it falls back to 0.
    if (auto character = loadCharacter(files, "default_missing")) {
        check(character->paletteChoices() == 2, "pal1/pal2 gives two choices");
        check(character->defaultPaletteChoice() == 0,
              "a default (palNo 3) not among the choices falls back to choice 0");
    }

    // ------------------------------------------------ 3. no .act, bank table
    //
    // No palN keys of any kind. The choices are the sprite bank's own table,
    // 1-based, and palNoForChoice is a plain offset. The three palettes are
    // seeded onto the character's own bank to stand in for the nodes an SFF v2
    // character carries; a bank that has none of those and no .act reports a
    // single choice -- itself.
    if (auto character = loadCharacter(files, "banktable")) {
        check(character->paletteChoices() == 1,
              "a bank with no palettes still reports one choice, never zero");
        check(character->palNoForChoice(0) == 1, "the single choice is palNo 1");

        seedBankPalettes(*character, 3);
        check(character->paletteChoices() == 3, "three bank palettes are three choices");
        bool oneToOne = true;
        for (int c = 0; c < 3; ++c) {
            if (character->palNoForChoice(c) != c + 1) oneToOne = false;
        }
        check(oneToOne, "with no .act files, choice c maps to palNo c + 1");
        check(character->palNoForChoice(-5) == 1, "a negative choice clamps to palNo 1");
        check(character->palNoForChoice(99) == 3, "a too-large choice clamps to the last bank palette");
    }

    // ------------------------------------- 4. neither .act nor bank palettes
    //
    // The floor a UI relies on: choices is never 0, so a picker is never empty
    // and a caller dividing by it never divides by zero.
    if (auto character = loadCharacter(files, "empty")) {
        check(character->paletteChoices() >= 1, "a character with no palette at all still has one choice");
        check(character->palNoForChoice(0) == 1, "its one choice is palNo 1");
        check(character->palNoForChoice(99) == 1, "clamping a too-large choice still yields a drawable palNo");
    }

    // ------------------------------------------------- 5. .act set beats table
    //
    // A character can have both. The .act set is the author explicitly naming
    // the alternates, so it is the whole answer; folding the SFF's own table in
    // would only add duplicates of pictures the character already offers.
    if (auto character = loadCharacter(files, "mixed")) {
        check(character->paletteChoices() == 2, "pal1/pal3 is two choices even with bank palettes present");
        seedBankPalettes(*character, 5);
        check(character->paletteChoices() == 2,
              "adding bank palettes does not enlarge an .act-defined choice set");
        check(character->palNoForChoice(0) == 1 && character->palNoForChoice(1) == 3,
              "the .act palNos are still the ones offered");
    }

    std::printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
