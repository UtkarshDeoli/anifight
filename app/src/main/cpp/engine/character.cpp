#include "engine/character.h"

#include <array>
#include <cstring>
#include <memory>
#include <optional>
#include <utility>

#include "core/ini.h"
#include "core/log.h"

namespace af {

namespace {

// Reads a file whole. Every file a character names is small and read once.
std::unique_ptr<FileData> readWhole(FileSource& files, const std::string& path) {
    return files.read(path);
}

std::string directoryOf(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return {};
    return path.substr(0, slash + 1);
}

}  // namespace


bool Character::load(FileSource& files, const std::string& defPath, std::string* errorOut) {
    actPalettes_.fill(-1);
    palDefault_ = 1;

    auto defData = readWhole(files, defPath);
    if (!defData) {
        *errorOut = "def not found: " + defPath;
        return false;
    }
    IniFile def = IniFile::parse(
        std::string_view(reinterpret_cast<const char*>(defData->data()), defData->size()));

    bool ok = false;
    const IniSection* info = def.section("info");
    if (info != nullptr) {
        // Both values are quoted in a real character .def, so unquote them;
        // the raw string would otherwise draw its own quotation marks.
        name_ = std::string(unquote(IniFile::get(*info, "name")));
        displayName_ = std::string(unquote(IniFile::get(*info, "displayname")));
        ok = !name_.empty();
        // localcoord is MUGEN's "both numbers or nothing" pair.
        const std::vector<std::string> coord =
            splitCommas(std::string(IniFile::get(*info, "localcoord")));
        if (coord.size() == 2) {
            tryParseFloat(coord[0], localCoord_.x);
            tryParseFloat(coord[1], localCoord_.y);
        }
        // pal.defaults is MUGEN 1.0's palette preference list; only the first
        // entry is the default the fighter starts in.
        const std::vector<std::string> defaults =
            splitCommas(std::string(IniFile::get(*info, "pal.defaults")));
        float first = 0.0f;
        if (!defaults.empty() && tryParseFloat(defaults[0], first) && first >= 1.0f &&
            first <= 12.0f) {
            palDefault_ = static_cast<int>(first);
        }
    }
    if (!ok) {
        *errorOut = "def has no [Info] name: " + defPath;
        return false;
    }
    if (displayName_.empty()) displayName_ = name_;

    folder_ = directoryOf(defPath);
    const IniSection* filesSection = def.section("files");
    if (filesSection == nullptr) {
        *errorOut = "def has no [Files]: " + defPath;
        return false;
    }

    // ------------------------------------------------------------- sprites
    //
    // The sprite file has three spellings across MUGEN's eras: `sprite` in
    // the classic .def files, `sff` in 1.0+ ones. `anim`/`air` differs
    // likewise for animations. Read every spelling, newest first.
    const std::string sff = normaliseFileRef(IniFile::get(*filesSection, "sff",
                                                 IniFile::get(*filesSection, "sprite")));
    if (!sff.empty()) {
        const std::string path = folder_ + sff;
        std::string error;
        if (sprites_.load(files, path, &error)) {
            AF_LOGI("char %s: %zu sprites from %s (SFF v%d)", name_.c_str(), sprites_.size(),
                    path.c_str(), sprites_.version());
        } else {
            AF_LOGE("char %s: sprite file %s failed: %s", name_.c_str(), path.c_str(),
                    error.c_str());
        }
    } else {
        AF_LOGW("char %s: no sprite file in [Files]", name_.c_str());
    }

    // --------------------------------------------------------- animations
    const std::string air = normaliseFileRef(IniFile::get(*filesSection, "air",
                                                 IniFile::get(*filesSection, "anim")));
    if (!air.empty()) {
        const std::string path = folder_ + air;
        if (auto data = readWhole(files, path)) {
            parseAirText(
                std::string_view(reinterpret_cast<const char*>(data->data()), data->size()),
                animations_);
            AF_LOGI("char %s: %zu animations from %s", name_.c_str(), animations_.size(),
                    path.c_str());
        } else {
            AF_LOGW("char %s: animation file not found: %s", name_.c_str(), path.c_str());
        }
    }

    // ------------------------------------------------------------- palettes
    //
    // pal1..pal12 name .act files: 768 bytes of RGB triples, nothing else
    // (there is a 768+4-byte variant with a leading count word, tolerated by
    // reading the last 768 bytes). They join the sprite bank's palette table
    // after the SFF's own entries, so palette index 0 keeps meaning "what the
    // SFF carries" and the acts live beyond it.
    int acts = 0;
    for (int n = 1; n <= 12; ++n) {
        const std::string key = "pal" + std::to_string(n);
        const std::string act = normaliseFileRef(IniFile::get(*filesSection, key));
        if (act.empty()) continue;
        const std::string path = folder_ + act;
        auto data = readWhole(files, path);
        if (!data || data->size() < 768) {
            AF_LOGW("char %s: palette %s not found or too small (%s)", name_.c_str(), key.c_str(),
                    path.c_str());
            continue;
        }
        // The 772-byte variant carries a 4-byte signature/count in front.
        const uint8_t* rgb = data->data() + (data->size() - 768);

        // An .act's entries run the opposite way round from the palettes a
        // sprite bank carries: for every character here that has both, entry i
        // of the bank's palette equals entry 255-i of the .act, exactly, all 256
        // of them (the counts are printed by tools/palcheck.py). That holds for
        // both formats -- against an SFF v1 sprite's own palette and against an
        // SFF v2 palette node alike -- so this is a property of .act files, not
        // of either sprite layout.
        //
        // Sprite indices are authored against the bank's palette: that is what
        // makes a character render correctly before any .act is applied, and the
        // bank is read unflipped because that is the reading that produces the
        // right picture. So an .act has to be flipped to be indexed by them.
        // Without this a sprite that falls through to its character palette
        // draws in whatever colours the reversed indices happen to land on,
        // which reads as blue/green confetti rather than a fighter.
        std::array<uint8_t, 768> flipped{};
        for (size_t i = 0; i < 256; ++i) {
            std::memcpy(&flipped[i * 3], rgb + (255 - i) * 3, 3);
        }
        rgb = flipped.data();
        actPalettes_[static_cast<size_t>(n - 1)] = sprites_.addPalette(rgb);
        ++acts;
    }
    if (acts > 0) {
        AF_LOGI("char %s: %d palettes from .act files", name_.c_str(), acts);
    }

    // ------------------------------------------------- states and commands
    //
    // A character's behaviour is spread over more files than its name suggests,
    // and the keys are MUGEN's:
    //
    //   cns       the constants, and usually the character's own states
    //   st        the states, when they are kept apart from the constants
    //   st1..st9  the states the author split off: normals, specials, supers,
    //             transformations. Some characters keep *all* of their moves in
    //             these and none in `st`.
    //   stcommon  the common states MUGEN shares between characters. The file
    //             is very often not in the character's folder at all -- MUGEN
    //             looks for it in its own `data/` folder next, and Kung Fu Man
    //             is one of the characters that relies on that -- so a missing
    //             one is normal and is not an error here. The engine's built-in
    //             movement states cover it.
    //   cmd       the named commands, and by convention states -1, -2 and -3
    //
    // They are read in that order because later documents win: a character's own
    // state 0 replaces the common one, and its .cmd's state -1 is the one that
    // reads the commands.
    static const char* const kStateKeys[] = {
        "stcommon", "cns", "st", "st1", "st2", "st3", "st4", "st5", "st6", "st7", "st8", "st9",
    };
    for (const char* key : kStateKeys) {
        // `st` and `cns` are frequently the same file; reading it twice would
        // duplicate every controller in it.
        const std::string value = normaliseFileRef(IniFile::get(*filesSection, key));
        if (value.empty()) continue;
        const std::string path = folder_ + value;
        if (auto data = readWhole(files, path)) {
            const std::string_view raw(reinterpret_cast<const char*>(data->data()), data->size());
            // Constants live in the .cns; a file named as a state file that
            // happens to carry [Data] is read for those too, since some
            // characters keep everything in one place.
            if (std::string_view(key) == "cns" || raw.find("[Data]") != std::string_view::npos ||
                raw.find("[data]") != std::string_view::npos) {
                loadCns(IniFile::parse(raw), raw);
            }
            states_.parse(raw);
        } else {
            // A missing common state file is how most of the characters in the
            // wild are packaged; a missing state file the character *needs* is
            // worth a warning.
            if (std::string_view(key) == "stcommon") {
                AF_LOGI("char %s: no %s in the folder (%s); the engine's built-in "
                        "common states will be used",
                        name_.c_str(), key, value.c_str());
            } else {
                AF_LOGW("char %s: %s not found: %s", name_.c_str(), key, path.c_str());
            }
        }
    }

    // ------------------------------------------------------------- sounds
    //
    // Read here rather than with the art, because a missing sound file is the
    // normal case rather than a failure: most archives drop it, and the sounds
    // a character actually needs for a hit -- groups 5 and 6 -- come from the
    // screenpack's own file.
    const std::string sound = normaliseFileRef(IniFile::get(*filesSection, "sound"));
    if (!sound.empty()) {
        const std::string path = folder_ + sound;
        if (auto data = readWhole(files, path)) {
            std::string error;
            if (sounds_.load(*data, &error)) {
                AF_LOGI("char %s: %zu sounds from %s", name_.c_str(), sounds_.size(),
                        path.c_str());
            } else {
                AF_LOGW("char %s: sound file %s failed: %s", name_.c_str(), path.c_str(),
                        error.c_str());
            }
        }
    }

    std::string cmdText;
    const std::string cmd = normaliseFileRef(IniFile::get(*filesSection, "cmd"));
    if (!cmd.empty()) {
        const std::string path = folder_ + cmd;
        if (auto data = readWhole(files, path)) {
            cmdText.assign(reinterpret_cast<const char*>(data->data()), data->size());
        } else {
            AF_LOGW("char %s: cmd not found: %s", name_.c_str(), path.c_str());
        }
    }

    // The .cns has already been read above; passing it again here would parse
    // every one of its states twice. Only the .cmd is left for this call, and it
    // is read here rather than in the loop because it contributes *commands* as
    // well as states.
    loadBehaviour({}, cmdText);

    // Measured last, because it reads the sprites and the animations rather
    // than the behaviour.
    standingHeight_ = measureStandingHeight();

    AF_LOGI("char %s: life %d, attack %d, defence %d, walk %.1f/%.1f, jump %.1f, %zu states, %zu controllers, %zu commands, size %.0f/art %.0f",
            name_.c_str(), life_, attack_, defence_, walkFwd_, walkBack_, jumpUp_, states_.size(),
            states_.controllerCount(), commands_.size(), sizeHeight_, standingHeight_);
    return true;
}

void Character::loadBehaviour(std::string_view cnsText, std::string_view cmdText) {
    if (!cnsText.empty()) {
        loadCns(IniFile::parse(cnsText), cnsText);
        states_.parse(cnsText);
    }
    if (!cmdText.empty()) {
        commands_.parse(cmdText);
        // The .cmd's own states -- conventionally -1, -2, -3, and any states the
        // author keeps beside their commands -- are part of the same machine, so
        // they are merged into the same library. A state the .cns already
        // defined keeps its header; this only adds.
        states_.parse(cmdText);
    }
    // After every file, once: the constants a character's files left out take
    // MUGEN's defaults and the derived aliases are computed from whatever the
    // files (or the defaults) put in. Seeding per file instead would let an
    // earlier file's default shadow a later file's own number.
    seedConstantDefaults(localCoord_, consts_);
}

void Character::loadCns(const IniFile& document, std::string_view /*raw*/) {
    // `[Quotes]` travels with the constants: MUGEN documents the quotes as
    // living "in the [Quotes] group of the player's constants file"
    // (sctrls.html), which is the .cns this is called for. A state file that
    // also carries `[Data]` is read here too, so a character that keeps
    // everything in one document still works. Entries are merged rather than
    // replaced, so a later document may add quotes without erasing the .cns's.
    readVictoryQuotes(document, victoryQuotes_);

    // The named constants. Every key of every constant section is kept under its
    // `const()` spelling, because which ones a character reads is up to the
    // character: one walks with `const(velocity.walk.fwd)` and the next
    // hard-codes 2.4. Names the file left out are filled in later, by
    // seedConstantDefaults.
    readConstants(document, consts_);

    if (const IniSection* data = document.section("data")) {
        life_ = std::max(1, IniFile::getInt(*data, "life", life_));
        attack_ = std::max(1, IniFile::getInt(*data, "attack", attack_));
        defence_ = std::max(1, IniFile::getInt(*data, "defence", defence_));
        // Not clamped to a minimum of 1: a character is allowed to declare that it
        // cannot be juggled at all, and 0 is how it says so.
        airJuggle_ = std::max(0, IniFile::getInt(*data, "airjuggle", airJuggle_));
        // How long a knocked-down fighter lies there. Not clamped to a minimum
        // of 1 for the same reason airjuggle is not: a file may write 0 and mean
        // "get straight up", and the reference's countdown honours that.
        liedownTime_ = std::max(0, IniFile::getInt(*data, "liedown.time", liedownTime_));
    }
    if (const IniSection* velocity = document.section("velocity")) {
        walkFwd_ = std::max(0.0f, IniFile::getFloat(*velocity, "walk.fwd", walkFwd_));
        // [Velocity] walk.back is written NEGATIVE in classic .cns files (it
        // reads as an offset), MUGEN 1.0 uses magnitude. A speed is taken
        // from whichever sign arrives.
        const float back = IniFile::getFloat(*velocity, "walk.back", -walkBack_);
        walkBack_ = std::fabs(back) > 1e-4f ? std::fabs(back) : walkBack_;
        // Jump velocities are written negative (up). Store a positive upward
        // speed and let the runtime apply the sign; a non-negative file value
        // is nonsense and is ignored. Two spellings exist: `jump.neu = x, y`
        // (a velocity pair, the classic form) and `jump.neu.up = y` (1.0's).
        float jumpUpValue = 0.0f;
        const std::vector<std::string> neu = splitCommas(
            std::string(IniFile::get(*velocity, "jump.neu")));
        if (neu.size() == 2 && tryParseFloat(neu[1], jumpUpValue) == false) {
            jumpUpValue = 0.0f;
        }
        if (neu.size() != 2) {
            jumpUpValue = IniFile::getFloat(*velocity, "jump.neu.up", 0.0f);
        }
        if (jumpUpValue < -1e-4f) jumpUp_ = -jumpUpValue;
    }
    if (const IniSection* movement = document.section("movement")) {
        gravity_ = std::max(0.01f, IniFile::getFloat(*movement, "yaccel", gravity_));
    }
    if (const IniSection* size = document.section("size")) {
        sizeHeight_ = std::max(1.0f, IniFile::getFloat(*size, "height", sizeHeight_));
    }
}

// How tall the character's standing art draws, measured off the sprite.
//
// Taken from the first frame of animation 0 that has art, falling back to
// sprite (0,0) itself and then to the declared height. Characters do not
// reliably put their standing pose at animation 0 frame 0, and a wrong
// measurement here would scale the whole fighter, so all three are tried.
float Character::measureStandingHeight() const {
    const Animation* stand = animations_.find(0);
    if (stand != nullptr) {
        for (const AnimFrame& frame : stand->frames) {
            if (frame.blank()) continue;
            const Sprite* sprite = sprites_.find(frame.group, frame.image);
            if (sprite != nullptr && sprite->valid()) return static_cast<float>(sprite->height);
        }
    }
    if (const Sprite* sprite = sprites_.find(0, 0); sprite != nullptr && sprite->valid()) {
        return static_cast<float>(sprite->height);
    }
    return 0.0f;
}

int Character::paletteIndexFor(int palNo) const {
    if (palNo >= 1 && palNo <= 12 && actPalettes_[static_cast<size_t>(palNo - 1)] >= 0) {
        return actPalettes_[static_cast<size_t>(palNo - 1)];
    }
    const int count = sprites_.paletteCount();
    if (count <= 0) return 0;
    return std::clamp(palNo - 1, 0, count - 1);
}

int Character::paletteIndexFor(int group, int item) const {
    // Group 1 is what `palno`, `pal1..pal12` and the corpus's RemapPal lines
    // all use: the 1-based palette number, with the .act files' entries taking
    // precedence over the SFF table. The existing palno lookup is reused
    // verbatim, so an item the character does not carry clamps into the table
    // exactly as `palno` does. Anything else is an SFF v2 palette node
    // addressed by the key it carries, which is what makes the group half of a
    // RemapPal pair mean something.
    if (group == 1) return paletteIndexFor(item);
    return sprites_.paletteNodeIndex(group, item);
}

int Character::paletteChoices() const {
    int acts = 0;
    for (const int index : actPalettes_) {
        if (index >= 0) ++acts;
    }
    // An .act set is the character explicitly saying "these are my alternates",
    // so when there is one it is the whole answer -- the SFF's own table is the
    // same picture under a different name and would only add duplicates.
    if (acts > 0) return acts;
    return std::max(1, sprites_.paletteCount());
}

int Character::palNoForChoice(int choice) const {
    const int choices = paletteChoices();
    const int wanted = std::clamp(choice, 0, choices - 1);
    int seen = 0;
    for (int palNo = 1; palNo <= 12; ++palNo) {
        if (actPalettes_[static_cast<size_t>(palNo - 1)] < 0) continue;
        if (seen == wanted) return palNo;
        ++seen;
    }
    // No .act files, so the choices are the sprite bank's table and `palno`
    // reaches it directly, 1-based.
    return wanted + 1;
}

int Character::defaultPaletteChoice() const {
    const int choices = paletteChoices();
    for (int choice = 0; choice < choices; ++choice) {
        if (palNoForChoice(choice) == palDefault_) return choice;
    }
    return 0;
}

}  // namespace af