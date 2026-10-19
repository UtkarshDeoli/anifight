// A character, as loaded off disk: identity, sprites, animations, the constants
// its state machine reads, and the state machine itself.
//
// A MUGEN character folder is a .def naming a .sff (sprites), an .air
// (animations) and a .cns/.cmd (states and commands). All of it is read here
// and kept as data; engine/statemachine is what runs it.
//
// Every fighter the game has comes through here: the roster is community
// characters the player supplies, not anything generated or compiled in.
#pragma once

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "audio/snd_bank.h"
#include "core/file_source.h"
#include "core/ini.h"
#include "engine/anim.h"
#include "engine/cns.h"
#include "engine/cmd.h"
#include "engine/defs.h"
#include "engine/victory_flow.h"
#include "gfx/sprite_bank.h"

namespace af {

class IniFile;

class Character {
public:
    Character() = default;
    ~Character() = default;
    Character(const Character&) = delete;
    Character& operator=(const Character&) = delete;

    // `defPath` is the path of the character's .def, relative to the game
    // root. Everything else in the folder is resolved against its directory.
    // Returns false and fills `errorOut` for a missing or malformed .def; a
    // missing .sff or .air is logged and skipped, because a character with no
    // art is still a selectable name and much better than a lost slot.
    bool load(FileSource& files, const std::string& defPath, std::string* errorOut);

    // Reads only the behaviour: the states, the commands and the constants
    // their triggers read. This is the half of a character that has no art in
    // it, which is what makes a state machine runnable without a sprite bank, a
    // display or an APK -- and therefore testable on the host, against real
    // community files, rather than only by watching a phone.
    //
    // Either text may be empty, which leaves that half of the character as it
    // was. `load()` calls this for a character that has both files.
    void loadBehaviour(std::string_view cnsText, std::string_view cmdText);

    [[nodiscard]] const std::string& folder() const { return folder_; }
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& displayName() const { return displayName_; }
    // The character's own coordinate space, from [Info] localcoord. Drawing
    // scales by the ratio of this to the screenpack's, so a 720p character
    // lands the same height as a 240p one.
    [[nodiscard]] const Vec2& localCoord() const { return localCoord_; }

    [[nodiscard]] SpriteBank& sprites() { return sprites_; }
    [[nodiscard]] AnimationSet& animations() { return animations_; }
    [[nodiscard]] const AnimationSet& animations() const { return animations_; }

    // ------------------------------------------------------------- palettes
    //
    // A character's .def names up to twelve .act files (`pal1 = red.act`...)
    // in its [Files], each 768 raw RGB bytes; they are loaded into the sprite
    // bank's palette table at load and the fighter picks between them with the
    // `palno` machinery. Index 0 of the SFF's own table stays the fallback for
    // characters that carry no .act at all.
    //
    // Which table index palno (1-based, MUGEN's numbering) selects. With .act
    // files loaded this is that numbered one's entry; without them it clamps
    // into the SFF's own table, so a `palno` trigger asking for palette 3 of a
    // palette-less character gets palette 0 rather than garbage.
    [[nodiscard]] int paletteIndexFor(int palNo) const;
    // The same lookup addressed the way `RemapPal` writes a palette:
    // `(group, item)`. Group 1 is the palno/`.act` numbering above, so the
    // legacy item-only form is exactly `group = 1`. Any other group names an
    // SFF v2 palette node by its own key; SFF v1 has no palette nodes and
    // answers -1 (the caller keeps the sprite's own table then).
    [[nodiscard]] int paletteIndexFor(int group, int item) const;
    // The palette the character starts in: [Info] pal.defaults' first entry,
    // else 1 -- MUGEN's default.
    [[nodiscard]] int defaultPalNo() const { return palDefault_; }
    // Whether the engine has a palette opinion for this character at all (an
    // empty SFF table plus no .act files means slot 0 is already the answer).
    [[nodiscard]] bool hasPalettes() const { return sprites_.paletteCount() > 0; }

    // The palettes a player may actually pick between, for a select screen that
    // wants to offer the choice. Two characters can have them from different
    // places -- a `pal1`..`pal12` set of .act files, or, for a character that
    // ships no .act at all, the palette nodes an SFF v2 carries in its own table
    // -- so both are folded into one numbering here rather than at the UI.
    //
    // `paletteChoices` is never less than 1 (a character with no alternates has
    // exactly one choice, itself), and `palNoForChoice` maps a choice back to
    // the 1-based palno that `paletteIndexFor` understands, so the caller never
    // has to know which of the two sources it is cycling. Skipping the empty
    // slots matters: `paletteIndexFor` clamps a missing palNo into the SFF's
    // table rather than failing, so a UI that counted 1..12 blindly would offer
    // the same picture four times and call it four choices.
    [[nodiscard]] int paletteChoices() const;
    [[nodiscard]] int palNoForChoice(int choice) const;

    // Where the character's own `pal.defaults` sits among `paletteChoices`, so a
    // picker can open on the colour the author meant it to be drawn in. A
    // character whose defaults name pal 6 should appear in palette 6 until
    // somebody asks for another one, and the choice index a UI holds is not a
    // palNo -- the two only line up when every slot is filled.
    //
    // 0 when the default is not one of the choices at all, which is the normal
    // case for a character shipping no .act files: there its default is a plain
    // slot number into the sprite bank's own table rather than one of the .act
    // entries this counts.
    [[nodiscard]] int defaultPaletteChoice() const;

    // The character's own sounds, from the `sound` key in its [Files]. Empty for
    // the many characters packaged without one -- the sound file is the first
    // thing an archive drops to save space -- which is why the mixer also takes
    // the screenpack's bank as a fallback.
    [[nodiscard]] const SndBank& sounds() const { return sounds_; }

    // The character's states, merged from its .cns and .cmd. Empty for a
    // character with neither, which the runtime treats as "no behaviour of its
    // own" rather than as an error.
    [[nodiscard]] const StateLibrary& states() const { return states_; }
    [[nodiscard]] const CommandList& commands() const { return commands_; }

    // The `[Quotes]` win quotes, indexed by MUGEN's `victory0`..`victory99`
    // spelling. They are character data, not screen data: the Victory screen
    // reads them through the winner, and `VictoryQuote` only records which
    // index the character asked for. The set is sparse -- a character may write
    // only victory1, victory3 and victory7 -- and an unwritten index stays an
    // empty string. See victory_flow.h for the parser and selection helpers.
    [[nodiscard]] const VictoryQuoteSet& victoryQuotes() const { return victoryQuotes_; }

    // Every `const(...)` value the .cns declares, keyed the way `const()`
    // spells it: "data.life", "size.ground.back", "velocity.walk.fwd",
    // "movement.yaccel". Kept as one map rather than a field per constant
    // because characters read a long tail of these and the tail differs from
    // character to character.
    //
    // Returns whether the constant is declared at all, writing its value
    // through `out` when it is: a character needs to tell a constant that is
    // declared as zero from one that was never written, and `const()` on a
    // missing key is zero either way but the log is not.
    [[nodiscard]] bool constant(std::string_view key, float* out) const {
        const auto it = consts_.find(std::string(key));
        if (it == consts_.end()) return false;
        if (out != nullptr) *out = it->second;
        return true;
    }
    [[nodiscard]] float constant(std::string_view key, float fallback) const {
        float value = fallback;
        return constant(key, &value) ? value : fallback;
    }
    [[nodiscard]] const std::unordered_map<std::string, float>& constants() const {
        return consts_;
    }

    // ----------------------------------------------------------- constants
    //
    // `consts_` starts empty and only ever holds what a file wrote, so
    // `Const(x)` on a name the character never wrote reads 0 -- a character
    // that leaves out `data.life` fights with life 0, and one that leaves out
    // `velocity.jump.y` cannot jump through a trigger at all. MUGEN's engine
    // holds a default for the whole documented set (the 74 dotted names under
    // `Const(*)` in trigger.html, the list running from data.life to
    // movement.down.friction.threshold), so the names a file leaves out are
    // seeded from that set here rather than answering 0.
    //
    // The values are Elecbyte's shipped kfm/kfm720 template -- the same numbers
    // docs/tutorial3 reproduces -- so a name the engine "knows" agrees with the
    // character authors tune against.
    //
    // SCALE. A default is written in the 240p coordinate space (a 320-wide
    // character), but every constant in a character's files is in the
    // character's *own* space: motion is deliberately left unconverted (see the
    // note in fighter.cpp's setScale). So a default has to be converted into
    // the character's space too, and the conversion is by WIDTH, not height.
    // coordspace.html says it twice: "The translation scaling factor of vectors
    // between coordinate spaces is the ratio of the target coordinate space's
    // width and the source coordinate space's width", and `Const240p`'s
    // "conversion ratio between coordinate spaces is the ratio of their
    // widths". Elecbyte's own pair is the proof rather than the prose: every
    // [Size]/[Velocity]/[Movement] number in kfm720.cns is exactly four times
    // the same number in kfm.cns, y-components included (head.pos y -90 -> -360,
    // yaccel .44 -> 1.76), and kfm720 declares 1280x720 against kfm's 320x240.
    // 1280/320 = 4; the height ratio 720/240 = 3 is not a factor that appears
    // anywhere in the pair. So the factor is `localCoord.x / 320`, and only
    // values written in coordinate-space units (pixels, pixels/tick,
    // pixels/tick^2) take it.

    // One documented `Const` name and the default MUGEN gives it. `value` is a
    // pair for the names that take one ("-5, -90"); `scaled` marks a value in
    // coordinate-space units, which converts with the character's localcoord.
    struct ConstantDefault {
        const char* name;
        const char* value;
        bool scaled;
    };

    // Names are lower-case because that is the spelling the ini reader folds
    // keys to and the expression tokenizer folds identifiers to, so they key
    // the same map `const()` reads ("data.ko.echo", not "data.KO.echo").
    //
    // `movement.jump.changeanim.threshold` is deliberately absent: trigger.html
    // documents it, but no file in the corpus defines it and the engine default
    // is not knowable from the files Elecbyte shipped, so a value here would be
    // invented. It stays absent and reads 0 until the reference gives a number.
    static constexpr ConstantDefault kConstantDefaults[] = {
        // [Data]: combat numbers and tick counts. None of them is a
        // coordinate-space length or speed, so none converts.
        {"data.life", "1000", false},
        {"data.attack", "100", false},
        {"data.defence", "100", false},
        {"data.airjuggle", "15", false},
        {"data.power", "3000", false},
        {"data.liedown.time", "60", false},
        {"data.sparkno", "2", false},
        {"data.guard.sparkno", "40", false},
        {"data.ko.echo", "0", false},
        {"data.volume", "0", false},
        {"data.intpersistindex", "60", false},
        {"data.floatpersistindex", "40", false},
        // Input to the derived data.fall.defence_mul, below.
        {"data.fall.defence_up", "50", false},

        // [Size]: lengths, except the two art scales and the proj.doscale flag.
        {"size.xscale", "1", false},
        {"size.yscale", "1", false},
        {"size.ground.back", "15", true},
        {"size.ground.front", "16", true},
        {"size.air.back", "12", true},
        {"size.air.front", "12", true},
        {"size.height", "60", true},
        {"size.attack.dist", "160", true},
        {"size.proj.attack.dist", "90", true},
        {"size.proj.doscale", "0", false},
        {"size.head.pos", "-5, -90", true},
        {"size.mid.pos", "-5, -60", true},
        {"size.shadowoffset", "0", true},
        {"size.draw.offset", "0, 0", true},

        // [Velocity]: speeds. The air-recover .mul pair is a scale applied to a
        // velocity rather than a velocity, so it does not convert. The
        // jump.neu/airjump.neu pairs are seeded whole even though trigger.html
        // names only their `.x`: their y half is where velocity.jump.y and
        // velocity.airjump.y are copied from below, and a character with no
        // [Velocity] at all still has to be able to jump.
        {"velocity.walk.fwd", "2.4", true},
        {"velocity.walk.back", "-2.2", true},
        {"velocity.run.fwd", "4.6, 0", true},
        {"velocity.run.back", "-4.5, -3.8", true},
        {"velocity.jump.neu", "0, -8.4", true},
        {"velocity.jump.back", "-2.55", true},
        {"velocity.jump.fwd", "2.5", true},
        {"velocity.runjump.back", "-2.55", true},
        {"velocity.runjump.fwd", "4", true},
        {"velocity.airjump.neu", "0, -8.1", true},
        {"velocity.airjump.back", "-2.55", true},
        {"velocity.airjump.fwd", "2.5", true},
        {"velocity.air.gethit.groundrecover", "-0.15, -3.5", true},
        {"velocity.air.gethit.airrecover.mul", "0.5, 0.2", false},
        {"velocity.air.gethit.airrecover.add", "0, -4.5", true},
        {"velocity.air.gethit.airrecover.back", "-1", true},
        {"velocity.air.gethit.airrecover.fwd", "0", true},
        {"velocity.air.gethit.airrecover.up", "-2", true},
        {"velocity.air.gethit.airrecover.down", "1.5", true},

        // [Movement]: accelerations, and the heights and thresholds they act
        // over, are lengths; the friction coefficients and airjump.num are not.
        {"movement.yaccel", "0.44", true},
        {"movement.stand.friction", "0.85", false},
        {"movement.crouch.friction", "0.82", false},
        {"movement.stand.friction.threshold", "2", true},
        {"movement.crouch.friction.threshold", "0.05", true},
        {"movement.airjump.num", "1", false},
        {"movement.airjump.height", "35", true},
        {"movement.air.gethit.groundlevel", "25", true},
        {"movement.air.gethit.groundrecover.ground.threshold", "-20", true},
        {"movement.air.gethit.groundrecover.groundlevel", "10", true},
        {"movement.air.gethit.airrecover.threshold", "-1", true},
        {"movement.air.gethit.airrecover.yaccel", "0.35", true},
        {"movement.air.gethit.trip.groundlevel", "15", true},
        {"movement.down.bounce.offset", "0, 20", true},
        {"movement.down.bounce.yaccel", "0.4", true},
        {"movement.down.bounce.groundlevel", "12", true},
        {"movement.down.friction.threshold", "0.05", true},
    };

    // Reads one .cns document's [Data]/[Size]/[Velocity]/[Movement] lines into
    // `out`. A key written as a pair (`jump.neu = 0, -8.4`) is stored three
    // ways -- `<key>` and `<key>.x` hold the first half, `<key>.y` the second --
    // which is MUGEN's spelling, so `size.head.pos` and `size.head.pos.y` both
    // resolve and a file that writes a single number still answers the bare
    // name. The values are kept in the character's own units: they are what its
    // author tuned and are not converted.
    //
    // Static and defined here, rather than in character.cpp, so the host tests
    // reach it through their pure-logic build, which links no art or audio. It
    // depends on nothing but the ini reader and arithmetic.
    static void readConstants(const IniFile& document, std::unordered_map<std::string, float>& out) {
        struct Section {
            const char* name;
            const char* prefix;
        };
        static constexpr Section kSections[] = {
            {"data", "data."},         {"size", "size."},
            {"velocity", "velocity."}, {"movement", "movement."},
        };
        for (const Section& entry : kSections) {
            const IniSection* section = document.section(entry.name);
            if (section == nullptr) continue;
            for (const IniLine& line : section->lines) {
                if (line.key.empty()) continue;
                storeConstant(out, std::string(entry.prefix) + line.key, line.value, 1.0f);
            }
        }
    }

    // Fills every documented name `out` does not already hold with MUGEN's
    // default, converted into the character's space, then derives the three
    // values MUGEN computes rather than reads.
    //
    // Called once the character's files are in, not per file, so a name a later
    // file writes is never shadowed by a default seeded from an earlier one. It
    // is idempotent: an existing key is left alone, and the derived values are
    // recomputed from whatever is there.
    static void seedConstantDefaults(const Vec2& localCoord,
                                     std::unordered_map<std::string, float>& out) {
        const float ratio = localCoord.x > 0.0f ? localCoord.x / 320.0f : 1.0f;
        for (const ConstantDefault& entry : kConstantDefaults) {
            const std::string key = entry.name;
            if (out.find(key) != out.end()) continue;  // the file wrote it
            storeConstant(out, key, entry.value, entry.scaled ? ratio : 1.0f);
        }

        // The two jump aliases. trigger.html documents `velocity.jump.y` as
        // "the y-component of the jump.neu parameter" and takes the trouble to
        // add "this is NOT velocity.jump.neu.y": the name a character reads is
        // not the name the file writes, and nothing in the loader ever created
        // it, so every character read 0. It is written from the pair's y half
        // whenever there is one, so it follows the author's number; when a file
        // has no jump pair at all it stays absent, which is what leaves
        // statemachine.cpp's own fallback to jumpUp() in charge.
        if (const auto neu = out.find("velocity.jump.neu.y"); neu != out.end()) {
            out["velocity.jump.y"] = neu->second;
        }
        if (const auto neu = out.find("velocity.airjump.neu.y"); neu != out.end()) {
            out["velocity.airjump.y"] = neu->second;
        }

        // data.fall.defence_mul is trigger.html's own arithmetic -- "calculated
        // as 100/(f+100), where f is the fall.defence_up parameter" -- so it is
        // computed, never read, and a character that writes fall.defence_up
        // gets a multiplier of its own.
        const auto up = out.find("data.fall.defence_up");
        out["data.fall.defence_mul"] =
            100.0f / ((up == out.end() ? 50.0f : up->second) + 100.0f);
    }

    // [Data] -- the combat numbers. MUGEN's defaults, so a .cns missing any
    // of them still produces a sane fighter.
    [[nodiscard]] int life() const { return life_; }
    [[nodiscard]] int attack() const { return attack_; }
    [[nodiscard]] int defence() const { return defence_; }
    // [Data] airjuggle: how much air-combo allowance the fighter has. Every hit
    // that lands on an airborne victim spends its HitDef's `air.juggle` against
    // this, and a hit the allowance cannot pay for simply does not land -- which
    // is the whole of what stops an air combo going on forever.
    [[nodiscard]] int airJuggle() const { return airJuggle_; }

    // [Data] liedown.time: how many ticks a knocked-down fighter lies there
    // before the engine stands it up. It is not the length of the lying-down
    // animation and is not meant to be -- the reference's own common1.cns
    // defines state 5110 with no hand-over at all and leaves the timing to the
    // engine, which is why a character with no art for the pose still lies down
    // for the right amount of time instead of getting up on the frame it hit
    // the floor. What a character reads back as `GetHitVar(recovertime)` is this
    // countdown, not this number: see the field of the same name in
    // StateMachine, which re-arms from here.
    [[nodiscard]] int liedownTime() const { return liedownTime_; }

    // [Velocity] and [Movement], in MUGEN's units: pixels per tick, and
    // pixels of acceleration per tick per tick.
    [[nodiscard]] float walkFwd() const { return walkFwd_; }
    [[nodiscard]] float walkBack() const { return walkBack_; }
    [[nodiscard]] float jumpUp() const { return jumpUp_; }
    [[nodiscard]] float gravity() const { return gravity_; }

    // [Size] height: how tall the character declares itself to be, in its own
    // units. Every character authored to the format uses roughly MUGEN's 60 for
    // it, which is what makes two characters of wildly different art scales
    // still agree about how big a fighter is.
    [[nodiscard]] float sizeHeight() const { return sizeHeight_; }

    // How tall the character's standing art actually draws, in its own units.
    // Measured from the sprite rather than declared, because the declaration is
    // routinely ignored: characters in the wild draw at 1.5 times the height
    // they claim, and it is the art -- not the claim -- that decides whether two
    // fighters look like they belong in the same match.
    [[nodiscard]] float standingHeight() const { return standingHeight_; }

    // Jump animation length: how long to hold the jump art while airborne.
    [[nodiscard]] const Animation* animation(int number) const {
        return animations_.find(number);
    }

private:
    // The one place a constant goes into the map: bare key, `.x` and `.y`, each
    // half multiplied by `scale` (1 for a file's own value, the localcoord
    // ratio for a seeded default). A value whose first half will not parse is
    // dropped, which is how a file's stray comment or unit suffix is ignored.
    static void storeConstant(std::unordered_map<std::string, float>& out, const std::string& key,
                              std::string_view value, float scale) {
        const std::vector<std::string> parts = splitCommas(value);
        float first = 0.0f;
        if (parts.empty() || !tryParseFloat(parts[0], first)) return;
        out[key] = first * scale;
        out[key + ".x"] = first * scale;
        if (parts.size() >= 2) {
            float second = 0.0f;
            if (tryParseFloat(parts[1], second)) out[key + ".y"] = second * scale;
        }
    }

    void loadCns(const IniFile& document, std::string_view raw);
    // Reads the standing sprite and reports its pixel height, or 0 when the
    // character has no art to measure.
    [[nodiscard]] float measureStandingHeight() const;

    std::string folder_;  // trailing slash, e.g. "chars/akari/"
    std::string name_;
    std::string displayName_;
    Vec2 localCoord_{320.0f, 240.0f};

    SpriteBank sprites_;
    AnimationSet animations_;
    StateLibrary states_;
    CommandList commands_;
    SndBank sounds_;
    std::unordered_map<std::string, float> consts_;
    // `[Quotes]`, read from the constants document(s). Kept on the character
    // rather than copied into each fighter: two fighters of the same character
    // share the same quote list, and the screen needs the winner's copy.
    VictoryQuoteSet victoryQuotes_{};

    // pal1..pal12's sprite-bank table indices, -1 where the .def names no .act.
    // A sparse array rather than a counter because palno indexes by number:
    // pal3 with pal1 missing is still palette 3, not the first one found.
    std::array<int, 12> actPalettes_;
    int palDefault_ = 1;

    int life_ = 1000;
    int attack_ = 100;
    int defence_ = 100;
    int airJuggle_ = 15;
    int liedownTime_ = 60;
    float sizeHeight_ = 60.0f;
    float standingHeight_ = 0.0f;  // 0 means "not measured"
    float walkFwd_ = 3.0f;
    float walkBack_ = 2.5f;
    float jumpUp_ = 7.8f;
    float gravity_ = 0.44f;
};

}  // namespace af