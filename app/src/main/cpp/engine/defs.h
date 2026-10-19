// The screenpack model: mugen.cfg, system.def, fight.def and select.def.
//
// `system.def` and `fight.def` are both "documents full of drawable elements",
// so rather than a bespoke struct per section, everything drawable is a
// DisplayElement and every parser fills one in with the same helper. MUGEN's own
// rules make that safe: `anim`, `spr` and `font` are mutually exclusive on a
// given element, and the same modifiers (offset, scale, facing, layerno, ...)
// apply whichever of the three is used.
#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/ini.h"
#include "engine/anim.h"
#include "gfx/renderer.h"

namespace af {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

using Rect4 = std::array<float, 4>;  // x1, y1, x2, y2

struct FontRef {
    int font = -1;   // 1..10, indexes the file's own font table
    int bank = 0;    // colour bank within the font
    int align = 0;   // 1 left, 0 centre, -1 right
};

// One drawable thing. Only one of anim/sprite/font is ever set.
struct DisplayElement {
    bool present = false;

    int anim = -1;                              // an action number in this same file
    std::optional<std::pair<int, int>> sprite;  // group, image
    std::optional<FontRef> font;
    std::string text;  // font elements only; may contain %i / %s

    Vec2 offset;
    Vec2 scale{1.0f, 1.0f};
    int facing = 1;
    int layerno = 0;
    int displaytime = 1;
    std::optional<Vec2> alpha;
    std::optional<Rect4> window;
    // A screenpack element's `trans` is the same six-type family as a state
    // controller's, but two of its forms carry their pair differently; see
    // `transFromDef` in defs.cpp. Kept whole rather than folded to a blend mode
    // because `add1` and `addalpha` mean nothing without their destination half.
    TransParams trans{};

    [[nodiscard]] bool drawsSomething() const {
        return present && (anim >= 0 || sprite.has_value() || font.has_value());
    }
};

// A background layer, as used by [TitleBG n], [SelectBG n] and stage [BG n].
// The three share a syntax; only the section prefix differs.
struct BgElement {
    enum class Type { Normal, Parallax, Anim };

    Type type = Type::Normal;
    std::pair<int, int> sprite{-1, -1};
    int actionNo = -1;
    Vec2 start;
    Vec2 delta{1.0f, 1.0f};
    Vec2 tile{0.0f, 0.0f};
    Vec2 tileSpacing{1.0f, 1.0f};
    Vec2 velocity;
    TransParams trans{};
    std::optional<Vec2> alpha;
    bool mask = false;
    int layerno = 0;
    std::optional<Rect4> window;
    Vec2 scaleStart{1.0f, 1.0f};
    Vec2 scaleDelta{0.0f, 0.0f};
};

struct BgDef {
    Color clearColor{0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<BgElement> elements;
};

// -------------------------------------------------------------------- stages
//
// How a stage draws the shadows under the fighters. MUGEN puts these in the
// stage rather than in the character, which is what lets one stage put its
// fighters in flat noon light and another in something warmer, without either
// fighter knowing anything about it.
//
// The defaults are the reference's own (`SHADOW-RE.md` §1.2): intensity 128 on
// MUGEN's 0..256 scale where 256 is darkest, colour black, and a 0.4 vertical
// squash. `fadeTop`/`fadeBottom` are the stage's `fade.range = top_y, bot_y`;
// the pair is inactive unless top < bottom, which is also how the reference
// gates the ramp (`SHADOW-RE.md` §3.3). `reflectionIntensity` is
// `[Reflection] intensity`, default 0 = no reflection.
struct ShadowStyle {
    Color color{0.0f, 0.0f, 0.0f, 1.0f};
    // MUGEN writes this 0..256; zero turns the shadows off entirely, which is
    // how a stage says "there is no floor here to cast one on". The reference
    // does not clamp it at parse time, but the draw treats >= 256 as full.
    float intensity = 128.0f;
    // Vertical squash. A shadow on a floor receding away from the camera is
    // flatter than the fighter is tall, and this is the number that says how
    // much. A negative value puts the shadow on the other side (SHADOW-RE
    // §0 fact 5); this engine mirrors it, see `Fighter::draw`.
    float yScale = 0.4f;
    // `[Shadow] fade.range`. Both zero means the key was omitted, which is the
    // reference's "no fade": the shadow is always fully visible.
    float fadeTop = 0.0f;
    float fadeBottom = 0.0f;
    // `[Reflection] intensity`, parsed rather than the stale `reflect` flag.
    // Fighters and helpers draw a mirrored copy of their current sprite at this
    // alpha when it is positive; zero is off (`engine/fighter.cpp`).
    float reflectionIntensity = 0.0f;

    [[nodiscard]] bool fadeActive() const { return fadeTop < fadeBottom; }
};

// The raw colour triple of an object's `shadow = r,g,b` / `projshadow = r,g,b`.
// Kept as integers rather than a `Color` because -1 is a sentinel: for an
// Explod `r == -1` selects the stage colour, for a Projectile *any* component
// does (SHADOW-RE §4.4, §4.5), and 0,0,0 -- the default -- means no shadow at
// all. Resolving it before the draw would lose the distinction.
struct ShadowRgb {
    int r = 0;
    int g = 0;
    int b = 0;

    [[nodiscard]] bool none() const { return r == 0 && g == 0 && b == 0; }
};

// ---------------------------------------------------------------- system.def

struct TitleInfo {
    int fadeInTime = 10;
    int fadeOutTime = 10;

    Vec2 menuPos;
    FontRef menuItemFont;
    FontRef menuItemActiveFont;
    Vec2 menuItemSpacing{0.0f, 13.0f};
    Vec2 menuWindowMargins{12.0f, 8.0f};
    int menuWindowVisibleItems = 5;
    bool boxCursorVisible = false;
    Rect4 boxCursorCoords{};

    // Item keys are the stable names ("arcade", "versus", "options", "exit");
    // an empty string disables the item.
    std::map<std::string, std::string> itemNames;
};

struct SelectInfo {
    int fadeInTime = 10;
    int fadeOutTime = 10;

    int rows = 1;
    int columns = 1;
    bool wrapping = false;
    Vec2 pos;
    bool showEmptyBoxes = false;
    bool moveOverEmptyBoxes = false;
    Vec2 cellSize{27.0f, 27.0f};
    // MUGEN takes a single value here and applies it to both axes.
    int cellSpacing = 2;

    DisplayElement cellBg;
    DisplayElement cellRandom;
    int cellRandomSwitchTime = 4;

    DisplayElement p1CursorActive;
    DisplayElement p1CursorDone;
    DisplayElement p2CursorActive;
    DisplayElement p2CursorDone;
    Vec2 p1StartCell;
    Vec2 p2StartCell;

    // The small portrait drawn inside a cell, taken from the character's own
    // sprite file at group 9000.
    DisplayElement portrait;
    Vec2 portraitScale{1.0f, 1.0f};

    DisplayElement title;   // e.g. "Arcade Mode"
    DisplayElement stage;   // stage name, when a stage is highlighted

    DisplayElement p1Face;
    DisplayElement p2Face;
    DisplayElement p1Name;
    DisplayElement p2Name;

    // --- Anifight extension ---
    // Shown when select.def lists no characters. MUGEN's own behaviour is to
    // refuse to start, which makes an empty roster impossible to look at; since
    // an empty roster is the whole of v1, this screen says so instead.
    DisplayElement emptyMessage;
};

struct VsScreen {
    int time = 150;
    int fadeInTime = 15;
    int fadeOutTime = 15;
    DisplayElement match;   // text like "Match %i"
    DisplayElement p1Sprite;
    DisplayElement p2Sprite;
    DisplayElement p1Name;
    DisplayElement p2Name;
};

// ------------------------------------------------------- post-match screens
//
// Everything below is parsed but not consumed. The slice that added it exists
// to prove a corpus screenpack's post-match sections parse; the screens
// themselves land later, so these records deliberately have no draw code and no
// flow state behind them.
//
// Defaults are the ones SCREEN-FLOW-RE.md tabulates: the loader's compiled
// default where it was identified (Victory `time` 150, Continue `pos` 159,40)
// and otherwise the value the four shipped motifs agree on. A section may be
// missing entirely, in which case the reference keeps its compiled defaults --
// which are not always known, hence the notes.

struct VictoryScreen {
    // Gate tested before the screen is shown. The compiled default was not
    // identified; the distribution motif writes 1.
    bool enabled = true;
    // The loader pre-stores 150 and the motif overwrites it with 300.
    int time = 150;
    int fadeInTime = 8;
    int fadeOutTime = 15;
    // Winner's portrait, always from the character's own sprite file at
    // group 9000, image 2, rather than from system.sff.
    DisplayElement portrait;  // p1.*
    DisplayElement name;      // p1.name.*
    // Fallback text for a character that set no VictoryQuote.
    DisplayElement winQuote;  // winquote.*
    // `winquote.textwrap`; the corpus only ever writes `w` (word wrap).
    std::string winQuoteTextwrap = "w";
};

struct WinScreen {
    // Shown once per cleared run, not per match.
    bool enabled = true;
    DisplayElement winText;  // wintext.*
    int fadeInTime = 32;
    int poseTime = 300;
    int fadeOutTime = 64;
};

struct ContinueScreen {
    // The compiled default was not tabulated; the default motif enables it.
    bool enabled = true;
    // Binary default `0x48cc33`; the `big` motif rewrites it to 640,240.
    Vec2 pos{159.0f, 40.0f};
    DisplayElement prompt;  // continue.*
    DisplayElement yes;     // yes.*
    DisplayElement yesActive;
    DisplayElement no;  // no.*
    DisplayElement noActive;
    // Present in the loader (a 0x200-byte path) but absent from the shipped
    // docs and every corpus motif, and no consumer was located. Kept because it
    // is a real key; nothing here interprets it.
    std::string storyboard;
};

struct SurvivalResultsScreen {
    bool enabled = true;
    DisplayElement winText;  // winstext.*; %i is the rounds-survived counter
    int fadeInTime = 32;
    int showTime = 300;
    int fadeOutTime = 64;
    int roundsToWin = 5;
};

// [Game Over Screen], [Default Ending] and [End Credits] are the same shape:
// an enable switch plus a storyboard path, with no geometry or timer of their
// own. They stay separate SystemDef fields because each has its own place in
// the flow. All three are disabled in the distribution motif; KFM enables Game
// Over and End Credits.
struct StoryboardSwitch {
    bool enabled = false;
    std::string storyboard;
};

struct OptionInfo {
    std::pair<int, int> cursorMoveSound{100, 0};
    std::pair<int, int> cursorDoneSound{100, 1};
    std::pair<int, int> cancelSound{100, 2};
    // Only the `big` motif writes a title element; the loader reserves a
    // string for it in the record.
    DisplayElement title;
};

// Attract mode. Every shipped motif writes the same values, so these double as
// the defaults; no compiled default was tabulated.
struct DemoMode {
    bool enabled = true;
    bool selectEnabled = false;
    bool vsScreenEnabled = false;
    int titleWaitTime = 600;
    int fightEndTime = 1500;
    bool fightPlayBgm = false;
    bool fightStopBgm = true;
    bool fightBarsDisplay = false;
    int introWaitCycles = 1;
    int debugInfo = 0;
};

struct MusicTrack {
    std::string file;
    // Percent, 100 = unattenuated. The corpus comment states 100 is the
    // default; the loader's own initializer was not traced.
    int volume = 100;
    bool loop = false;
    // Empty in every corpus motif and no default was resolved, so "unset" is
    // kept distinct from a real zero.
    std::optional<int> loopStart;
    std::optional<int> loopEnd;
};

struct MusicDef {
    MusicTrack title{.loop = true};
    MusicTrack select{.loop = true};
    MusicTrack vs;
    MusicTrack victory;
};

struct SystemDef {
    std::string name;
    std::string author;
    Vec2 localCoord{320.0f, 240.0f};

    // [Files]
    std::string spriteFile = "system.sff";
    std::string soundFile = "system.snd";
    std::string selectFile = "select.def";
    std::string fightFile = "fight.def";
    std::array<std::string, 11> fonts;  // indices 1..10
    [[nodiscard]] const std::string& fontAt(int index) const;

    TitleInfo title;
    SelectInfo select;
    VsScreen versus;

    // Inert: parsed, never drawn or read by the flow.
    MusicDef music;
    DemoMode demo;
    VictoryScreen victory;
    WinScreen win;
    ContinueScreen continueScreen;
    StoryboardSwitch gameOver;
    SurvivalResultsScreen survival;
    StoryboardSwitch defaultEnding;
    StoryboardSwitch endCredits;
    OptionInfo option;

    BgDef titleBg;
    BgDef selectBg;
    BgDef versusBg;
    BgDef victoryBg;
    BgDef optionBg;

    AnimationSet animations;
};

// ----------------------------------------------------------------- fight.def

// A player's side of a bar: lifebar, powerbar. Layers draw bottom to top as
// bg0, bg1, mid, front.
struct BarSide {
    Vec2 pos;
    DisplayElement bg0;
    DisplayElement bg1;
    DisplayElement mid;
    DisplayElement front;
    // Where the fill starts and ends, relative to `pos`. P1 uses a negative
    // end x so the bar drains leftward.
    Vec2 rangeX;
    Vec2 rangeY;
    DisplayElement counter;
    bool present = false;
};

struct FaceSide {
    Vec2 pos;
    DisplayElement bg;
    DisplayElement face;
    bool present = false;
};

struct NameSide {
    Vec2 pos;
    DisplayElement name;
    bool present = false;
};

struct TimeSection {
    Vec2 pos;
    DisplayElement bg;
    DisplayElement counter;
    // Ticks per displayed second; 60 is one real second.
    int framesPerCount = 60;
    bool present = false;
};

struct RoundSection {
    int matchWins = 2;
    int startWaitTime = 30;
    int controlTime = 30;

    DisplayElement roundDefault;
    int roundTime = 0;
    int roundDisplayTime = 60;

    DisplayElement fight;
    int fightTime = 0;

    DisplayElement ko;
    int koTime = 0;

    DisplayElement doubleKo;   // DKO
    DisplayElement timeOver;   // TO

    DisplayElement win;
    DisplayElement win2;
    DisplayElement draw;

    // Per-round banners, keyed by round number (round1 .. round9).
    std::map<int, DisplayElement> roundBanners;

    int slowTime = 60;
    int overWaitTime = 45;
    int overHitTime = 10;
    int overWinTime = 45;
    int overTime = 210;
    int winTime = 60;
};

struct WinIconSide {
    Vec2 pos;
    Vec2 iconOffset;
    DisplayElement counter;
    // Keyed by win method: "n", "s", "h", "throw", "c", "t", "suicide",
    // "teammate", "perfect".
    std::map<std::string, DisplayElement> icons;
    bool present = false;
};

struct FightDef {
    std::string spriteFile = "fight.sff";
    std::string soundFile = "fight.snd";
    // The hit sparks: their own sprite file and their own animations, which is where a
    // HitDef's plain `sparkno` looks -- the `S` prefix is what means "this character's
    // own". This screenpack declared both files and the generator wrote neither, so
    // every hit landed without one.
    std::string fightFxSpriteFile = "fightfx.sff";
    std::string fightFxAirFile = "fightfx.air";
    std::string commonSoundFile = "common.snd";
    std::array<std::string, 11> fonts;  // indices 1..10
    [[nodiscard]] const std::string& fontAt(int index) const;

    float fightFxScale = 1.0f;

    BarSide lifebarP1;
    BarSide lifebarP2;
    BarSide powerbarP1;
    BarSide powerbarP2;
    FaceSide faceP1;
    FaceSide faceP2;
    NameSide nameP1;
    NameSide nameP2;
    TimeSection time;
    RoundSection round;
    WinIconSide winIconP1;
    WinIconSide winIconP2;
    int winIconUseUpTo = 4;

    AnimationSet animations;
};

// ----------------------------------------------------------------- mugen.cfg

struct Config {
    Vec2 gameSize{320.0f, 240.0f};  // [Config] GameWidth / GameHeight
    int gameSpeed = 60;
    std::string motif = "data/system.def";  // [Options] motif
    // Our own extension: classic MUGEN stretches to fill, we letterbox.
    AspectMode aspect = AspectMode::Fit;
    // `[Config] drawshadows`, the global kill switch for every shadow pass.
    // The reference defaults it to 1 (SHADOW-RE §0 fact 8).
    bool drawShadows = true;
};

// ---------------------------------------------------------------- select.def

struct RosterEntry {
    std::string defPath;   // e.g. "kfm/kfm.def", relative to chars/
    std::vector<std::string> stages;
    bool isRandomCell = false;
    int hidden = 0;
};

struct SelectDef {
    std::vector<RosterEntry> characters;
    std::vector<std::string> extraStages;

    // [Options] arcade.maxmatches: one value per difficulty tier, saying how
    // many fights that tier contributes to an arcade run. The tiers themselves
    // are a CPU-difficulty feature this engine does not have yet, so only the
    // total is used -- but the shape is kept, because collapsing it to a single
    // number at parse time would throw away the one thing the key is for.
    std::vector<int> arcadeMaxMatches;
    // The same shape for team modes. Parsed and logged, but nothing reads it
    // yet: team battles are not implemented.
    std::vector<int> teamMaxMatches;

    // How long an arcade run is. Zero when select.def does not say, which the
    // caller reads as "use the whole roster".
    [[nodiscard]] int arcadeMatchCount() const {
        int total = 0;
        for (const int fights : arcadeMaxMatches) {
            if (fights > 0) total += fights;
        }
        return total;
    }
};

// ------------------------------------------------------------------ parsing

// Reads the DisplayElement whose keys are prefixed by `prefix` (e.g. "p1.front"
// reads p1.front.spr, p1.front.offset, ...). Returns an element with
// present=false when the section says nothing about it.
DisplayElement readElement(const IniSection& section, std::string_view prefix);

// Parses every `[<prefix> n]` section of a document into one BgDef. `bgDefSection`
// supplies bgclearcolor.
BgDef parseBackground(const IniFile& document, std::string_view bgDefSection,
                      std::string_view elementPrefix);

SystemDef parseSystemDef(const IniFile& document);
FightDef parseFightDef(const IniFile& document);
Config parseConfig(const IniFile& document);
SelectDef parseSelectDef(const IniFile& document);

// Font tables are `font1 = jg.fnt` .. `font10 = ...`.
void parseFontTable(const IniSection* section, std::array<std::string, 11>& out);

}  // namespace af
