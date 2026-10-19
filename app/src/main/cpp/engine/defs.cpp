#include "engine/defs.h"

#include <algorithm>
#include <string>

#include "core/log.h"

namespace af {

namespace {

std::string joined(std::string_view prefix, std::string_view suffix) {
    std::string key(prefix);
    key += '.';
    key += suffix;
    return key;
}

// Background element sections are named "<prefix> <n>" -- "[TitleBG 0]",
// "[SelectBG 3]". The space matters: it is what keeps "[TitleBGdef]" from
// matching the "titlebg" prefix.
bool isBackgroundElementSection(std::string_view name, std::string_view prefix) {
    return name.size() > prefix.size() && name.compare(0, prefix.size(), prefix) == 0 &&
           name[prefix.size()] == ' ';
}

void readVec2(const IniSection& section, std::string_view key, Vec2& out) {
    const std::string_view value = IniFile::get(section, key);
    if (value.empty()) return;
    const std::vector<std::string> parts = splitCommas(value);
    if (parts.size() >= 1) tryParseFloat(parts[0], out.x);
    if (parts.size() >= 2) tryParseFloat(parts[1], out.y);
}

// Parses "left,top,right,bottom". Nullopt when the key is absent or malformed.
std::optional<Rect4> parseRect4(std::string_view value) {
    if (value.empty()) return std::nullopt;
    const std::vector<std::string> parts = splitCommas(value);
    if (parts.size() < 4) return std::nullopt;
    Rect4 rect{};
    for (size_t i = 0; i < 4; ++i) {
        if (!tryParseFloat(parts[i], rect[i])) return std::nullopt;
    }
    return rect;
}

// For fields where "unset" is a distinct state -- a display element with no
// `window` is not clipped, which is not the same as a window of all zeroes.
void readRect4(const IniSection& section, std::string_view key, std::optional<Rect4>& out) {
    const std::optional<Rect4> rect = parseRect4(IniFile::get(section, key));
    if (rect) out = rect;
}

// For fields with a meaningful default, where absent simply means "keep it".
void readRect4(const IniSection& section, std::string_view key, Rect4& out) {
    const std::optional<Rect4> rect = parseRect4(IniFile::get(section, key));
    if (rect) out = *rect;
}

void readColor(const IniSection& section, std::string_view key, Color& out) {
    const std::string_view value = IniFile::get(section, key);
    if (value.empty()) return;
    const std::vector<std::string> parts = splitCommas(value);
    if (parts.size() < 3) return;
    float rgb[3] = {0.0f, 0.0f, 0.0f};
    for (size_t i = 0; i < 3; ++i) {
        if (!tryParseFloat(parts[i], rgb[i])) return;
    }
    out = Color{rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f, 1.0f};
}

std::optional<Vec2> parseAlpha(std::string_view value) {
    if (value.empty()) return std::nullopt;
    const std::vector<std::string> parts = splitCommas(value);
    Vec2 alpha{1.0f, 1.0f};
    if (!parts.empty()) tryParseFloat(parts[0], alpha.x);
    if (parts.size() > 1) tryParseFloat(parts[1], alpha.y);
    return alpha;
}

void readAlpha(const IniSection& section, std::string_view key, std::optional<Vec2>& out) {
    if (const std::optional<Vec2> alpha = parseAlpha(IniFile::get(section, key))) out = alpha;
}

// An element's `trans` value, as the draw path wants it.
//
// The words are MUGEN's own, and only two of them are a bare blend mode:
//
//   add      -- additive at full destination, the format's `A`
//   add1     -- additive at 256,128: the docs' shorthand for `AS256D128`
//   sub      -- subtractive at full destination, the format's `S`
//   addalpha -- additive at the `alpha` pair, e.g. `alpha = 128, 64`
//   subalpha -- subtractive at the `alpha` pair
//
// So `add1` and `addalpha` differ from `add` only in the destination factor,
// which is exactly the half a bare `Blend` cannot carry. That half is not
// cosmetic: the reference feeds it to GL's constant alpha, and folding it away
// (as this parser used to) draws every `add1` screenpack layer twice as opaque
// as MUGEN does.
//
// A pair may also be written inline after the type in the newer form
// `trans = addalpha, 256, 0`; when it is, it wins over the `alpha` key. Values
// are in MUGEN's 0..256 units. The `alpha` key itself applies to every type,
// not just `addalpha`: it is the one explicit statement of source opacity in
// the section, and silently ignoring it on an `add` layer would be worse than
// letting it override the type's default pair.
//
// There is no `default`/`none` here: those are `Trans` *controller* types, and a
// screenpack element that writes no `trans` simply gets a normal, opaque draw.
// An unrecognised word is dropped the same way, which is MUGEN's own answer to
// a typo.
TransParams transFromDef(std::string_view value, const std::optional<Vec2>& alpha) {
    TransParams trans{};

    const std::vector<std::string> parts = splitCommas(value);
    const std::string type = parts.empty() ? std::string{} : toLower(trim(parts[0]));
    if (type == "add") {
        trans = TransParams{Blend::Additive};
    } else if (type == "add1") {
        trans = TransParams{Blend::Additive, 1.0f, 128.0f / 256.0f};
    } else if (type == "sub") {
        trans = TransParams{Blend::Subtractive};
    } else if (type == "addalpha" || type == "subalpha") {
        trans.blend = type == "addalpha" ? Blend::Additive : Blend::Subtractive;
        // The inline pair, in 0..256 units, wins over the separate key.
        float src = 0.0f;
        float dst = 0.0f;
        if (parts.size() >= 3 && tryParseFloat(parts[1], src) && tryParseFloat(parts[2], dst)) {
            trans.srcAlpha = std::clamp(src, 0.0f, 256.0f) / 256.0f;
            trans.dstAlpha = std::clamp(dst, 0.0f, 256.0f) / 256.0f;
            return trans;
        }
    } else {
        // Unrecognised, including empty: a normal draw, exactly as MUGEN drops
        // an invalid trans rather than guessing at what it meant.
        trans = TransParams{};
    }

    if (alpha) {
        // The separate `alpha` key, which an explicit pair above has already
        // consumed. This parser has always read it as 0..1 (`applyAlpha` clamps
        // it there), while every other alpha in the format is 0..256, so accept
        // both: a pair with no number above 1 is the normalised spelling this
        // engine already shipped with, and anything above it can only be MUGEN
        // units. For a normal draw only the source half is observable, but it
        // is exactly the tint alpha this key used to be folded into.
        const bool normalized = alpha->x <= 1.0f && alpha->y <= 1.0f;
        const float src = normalized ? alpha->x * 256.0f : alpha->x;
        const float dst = normalized ? alpha->y * 256.0f : alpha->y;
        trans.srcAlpha = std::clamp(src, 0.0f, 256.0f) / 256.0f;
        trans.dstAlpha = std::clamp(dst, 0.0f, 256.0f) / 256.0f;
    }
    return trans;
}

BarSide readBarSide(const IniSection& section, std::string_view prefix) {
    BarSide side;
    side.present = true;
    readVec2(section, joined(prefix, "pos"), side.pos);
    side.bg0 = readElement(section, joined(prefix, "bg0"));
    side.bg1 = readElement(section, joined(prefix, "bg1"));
    side.mid = readElement(section, joined(prefix, "mid"));
    side.front = readElement(section, joined(prefix, "front"));
    side.counter = readElement(section, joined(prefix, "counter"));
    readVec2(section, joined(prefix, "range.x"), side.rangeX);
    readVec2(section, joined(prefix, "range.y"), side.rangeY);
    return side;
}

FaceSide readFaceSide(const IniSection& section, std::string_view prefix) {
    FaceSide side;
    side.present = true;
    readVec2(section, joined(prefix, "pos"), side.pos);
    side.bg = readElement(section, joined(prefix, "bg"));
    side.face = readElement(section, joined(prefix, "face"));
    return side;
}

NameSide readNameSide(const IniSection& section, std::string_view prefix) {
    NameSide side;
    side.present = true;
    readVec2(section, joined(prefix, "pos"), side.pos);
    side.name = readElement(section, joined(prefix, "name"));
    return side;
}

WinIconSide readWinIconSide(const IniSection& section, std::string_view prefix) {
    WinIconSide side;
    side.present = true;
    readVec2(section, joined(prefix, "pos"), side.pos);
    readVec2(section, joined(prefix, "iconoffset"), side.iconOffset);
    side.counter = readElement(section, joined(prefix, "counter"));

    // Win methods, as spelled in fight.def: normal, special, hyper, throw,
    // "cheese", time over, suicide, teammate-KO, perfect.
    for (const char* method : {"n", "s", "h", "throw", "c", "t", "suicide", "teammate", "perfect"}) {
        DisplayElement icon = readElement(section, joined(prefix, method));
        if (icon.present) side.icons[method] = icon;
    }
    return side;
}

BgElement readBackgroundElement(const IniSection& section) {
    BgElement element;

    const std::string typeText = toLower(IniFile::get(section, "type"));
    if (typeText == "parallax") {
        element.type = BgElement::Type::Parallax;
    } else if (typeText == "anim") {
        element.type = BgElement::Type::Anim;
    } else {
        element.type = BgElement::Type::Normal;
    }

    if (const std::string_view v = IniFile::get(section, "spriteno"); !v.empty()) {
        const std::vector<std::string> parts = splitCommas(v);
        int group = 0;
        int image = 0;
        if (parts.size() >= 2 && tryParseInt(parts[0], group) && tryParseInt(parts[1], image)) {
            element.sprite = std::make_pair(group, image);
        }
    }

    if (const std::string_view v = IniFile::get(section, "actionno"); !v.empty()) {
        tryParseInt(v, element.actionNo);
    }

    readVec2(section, "start", element.start);
    readVec2(section, "delta", element.delta);
    readVec2(section, "tile", element.tile);
    readVec2(section, "tilespacing", element.tileSpacing);
    readVec2(section, "velocity", element.velocity);
    readVec2(section, "scalestart", element.scaleStart);
    readVec2(section, "scaledelta", element.scaleDelta);

    if (const std::string_view v = IniFile::get(section, "mask"); !v.empty()) {
        int mask = 0;
        if (tryParseInt(v, mask)) element.mask = mask != 0;
    }
    if (const std::string_view v = IniFile::get(section, "layerno"); !v.empty()) {
        tryParseInt(v, element.layerno);
    }
    readAlpha(section, "alpha", element.alpha);
    if (const std::string_view v = IniFile::get(section, "trans"); !v.empty()) {
        element.trans = transFromDef(v, element.alpha);
    }

    readRect4(section, "window", element.window);

    return element;
}

}  // namespace

const std::string& SystemDef::fontAt(int index) const {
    static const std::string kEmpty;
    if (index < 1 || index > 10) return kEmpty;
    return fonts[static_cast<size_t>(index)];
}

const std::string& FightDef::fontAt(int index) const {
    static const std::string kEmpty;
    if (index < 1 || index > 10) return kEmpty;
    return fonts[static_cast<size_t>(index)];
}

DisplayElement readElement(const IniSection& section, std::string_view prefix) {
    DisplayElement element;

    const auto get = [&](std::string_view suffix) { return IniFile::get(section, joined(prefix, suffix)); };

    // `anim`, `spr` and `font` are mutually exclusive; in practice a section sets
    // exactly one of them per element.
    if (const std::string_view v = get("anim"); !v.empty()) {
        int number = 0;
        if (tryParseInt(v, number)) {
            element.anim = number;
            element.present = true;
        }
    }

    if (const std::string_view v = get("spr"); !v.empty()) {
        const std::vector<std::string> parts = splitCommas(v);
        int group = 0;
        int image = 0;
        if (parts.size() >= 2 && tryParseInt(parts[0], group) && tryParseInt(parts[1], image)) {
            element.sprite = std::make_pair(group, image);
            element.present = true;
        }
    }

    if (const std::string_view v = get("font"); !v.empty()) {
        const std::vector<std::string> parts = splitCommas(v);
        FontRef ref;
        if (!parts.empty()) tryParseInt(parts[0], ref.font);
        if (parts.size() > 1) tryParseInt(parts[1], ref.bank);
        if (parts.size() > 2) tryParseInt(parts[2], ref.align);
        element.font = ref;
        element.present = true;
    }

    if (const std::string_view v = get("text"); !v.empty()) {
        element.text = unquote(v);
        element.present = true;
    }

    readVec2(section, joined(prefix, "offset"), element.offset);

    if (const std::string_view v = get("scale"); !v.empty()) {
        const std::vector<std::string> parts = splitCommas(v);
        if (!parts.empty()) tryParseFloat(parts[0], element.scale.x);
        if (parts.size() > 1) {
            tryParseFloat(parts[1], element.scale.y);
        } else {
            element.scale.y = element.scale.x;
        }
    }

    if (const std::string_view v = get("facing"); !v.empty()) tryParseInt(v, element.facing);
    if (const std::string_view v = get("layerno"); !v.empty()) tryParseInt(v, element.layerno);
    if (const std::string_view v = get("displaytime"); !v.empty()) tryParseInt(v, element.displaytime);
    // Both of these are element sub-keys like every other key above: a
    // `[LifeBar]` section writes `p1.front.alpha` / `p1.face.window`, not a
    // shared `alpha` line. Reading them unprefixed left every windowed
    // element -- the portraits are the corpus' own example -- unclipped.
    if (const std::optional<Vec2> alpha = parseAlpha(get("alpha"))) element.alpha = alpha;
    if (const std::string_view v = get("trans"); !v.empty()) {
        element.trans = transFromDef(v, element.alpha);
    }
    if (const std::optional<Rect4> rect = parseRect4(get("window"))) element.window = rect;

    return element;
}

namespace {

// readElement starts from DisplayElement's zero defaults and replaces the whole
// element, which is fine for sections that write every key but loses the
// geometry the reference documents for a screen element. Restore those defaults
// for keys the section omits; a key that is present was already parsed by
// readElement and is never overridden.
DisplayElement readElementWithDefaults(const IniSection& section, std::string_view prefix,
                                       const DisplayElement& defaults) {
    DisplayElement element = readElement(section, prefix);
    const auto absent = [&](std::string_view suffix) {
        return IniFile::get(section, joined(prefix, suffix)).empty();
    };

    if (absent("anim") && absent("spr") && absent("font")) {
        if (defaults.sprite) element.sprite = defaults.sprite;
        if (defaults.font) element.font = defaults.font;
    }
    if (absent("text") && !defaults.text.empty()) element.text = defaults.text;
    if (absent("offset")) element.offset = defaults.offset;
    if (absent("scale")) element.scale = defaults.scale;
    if (absent("facing")) element.facing = defaults.facing;
    if (absent("layerno")) element.layerno = defaults.layerno;
    if (absent("displaytime")) element.displaytime = defaults.displaytime;
    if (absent("alpha")) element.alpha = defaults.alpha;
    if (absent("window")) element.window = defaults.window;

    // A documented default can make an element drawable even when the section
    // says nothing about it (the victory portrait is the corpus case), so
    // `present` follows the post-default state rather than the file text.
    if (element.anim >= 0 || element.sprite.has_value() || element.font.has_value() ||
        !element.text.empty()) {
        element.present = true;
    }
    return element;
}

// "group,index", as written by the option screen's sound keys.
void readIntPair(const IniSection& section, std::string_view key, std::pair<int, int>& out) {
    const std::string_view value = IniFile::get(section, key);
    if (value.empty()) return;
    const std::vector<std::string> parts = splitCommas(value);
    if (!parts.empty()) tryParseInt(parts[0], out.first);
    if (parts.size() > 1) tryParseInt(parts[1], out.second);
}

// A section that exists but contains only comments has no keys to parse and
// should not produce a load-time line.
const IniSection* nonEmptySection(const IniFile& document, std::string_view name) {
    const IniSection* section = document.section(name);
    return (section != nullptr && !section->lines.empty()) ? section : nullptr;
}

MusicTrack readMusicTrack(const IniSection& section, std::string_view prefix, bool loopDefault) {
    MusicTrack track;
    track.loop = loopDefault;

    // Track keys are `title.bgm`, `title.bgm.volume`, ..., so the common part
    // is the screen name plus `.bgm`, not the screen name alone.
    const std::string bgm = joined(prefix, "bgm");
    const auto key = [&](std::string_view suffix) { return joined(bgm, suffix); };
    if (const std::string_view value = IniFile::get(section, bgm); !value.empty()) {
        track.file = unquote(value);
    }
    track.volume = IniFile::getInt(section, key("volume"), track.volume);
    track.loop = IniFile::getBool(section, key("loop"), track.loop);

    int loop = 0;
    if (const std::string_view value = IniFile::get(section, key("loopstart"));
        !value.empty() && tryParseInt(value, loop)) {
        track.loopStart = loop;
    }
    if (const std::string_view value = IniFile::get(section, key("loopend"));
        !value.empty() && tryParseInt(value, loop)) {
        track.loopEnd = loop;
    }
    return track;
}

}  // namespace

BgDef parseBackground(const IniFile& document, std::string_view bgDefSection,
                      std::string_view elementPrefix) {
    BgDef def;

    if (const IniSection* clearSection = document.section(bgDefSection); clearSection != nullptr) {
        readColor(*clearSection, "bgclearcolor", def.clearColor);
    }

    // File order is draw order within a layer; MUGEN sorts by layerno and is
    // otherwise stable, so preserving the document order is exactly right.
    for (const IniSection& section : document.sections()) {
        if (!isBackgroundElementSection(section.name, elementPrefix)) continue;
        def.elements.push_back(readBackgroundElement(section));
    }

    return def;
}

void parseFontTable(const IniSection* section, std::array<std::string, 11>& out) {
    if (section == nullptr) return;
    for (int i = 1; i <= 10; ++i) {
        const std::string key = "font" + std::to_string(i);
        const std::string_view value = IniFile::get(*section, key);
        if (!value.empty()) out[static_cast<size_t>(i)] = unquote(value);
    }
}

SystemDef parseSystemDef(const IniFile& document) {
    SystemDef def;

    if (const IniSection* info = document.section("info"); info != nullptr) {
        def.name = unquote(IniFile::get(*info, "name"));
        def.author = unquote(IniFile::get(*info, "author"));
        readVec2(*info, "localcoord", def.localCoord);
    }

    if (const IniSection* files = document.section("files"); files != nullptr) {
        // Note: system.def calls the sprite file `spr`; fight.def calls its own `sff`.
        if (const std::string_view v = IniFile::get(*files, "spr"); !v.empty()) {
            def.spriteFile = unquote(v);
        }
        if (const std::string_view v = IniFile::get(*files, "snd"); !v.empty()) {
            def.soundFile = unquote(v);
        }
        if (const std::string_view v = IniFile::get(*files, "select"); !v.empty()) {
            def.selectFile = unquote(v);
        }
        if (const std::string_view v = IniFile::get(*files, "fight"); !v.empty()) {
            def.fightFile = unquote(v);
        }
        parseFontTable(files, def.fonts);
    }

    if (const IniSection* title = document.section("title info"); title != nullptr) {
        TitleInfo& info = def.title;
        info.fadeInTime = IniFile::getInt(*title, "fadein.time", info.fadeInTime);
        info.fadeOutTime = IniFile::getInt(*title, "fadeout.time", info.fadeOutTime);
        readVec2(*title, "menu.pos", info.menuPos);
        readVec2(*title, "menu.item.spacing", info.menuItemSpacing);
        readVec2(*title, "menu.window.margins.y", info.menuWindowMargins);
        info.menuWindowVisibleItems =
            IniFile::getInt(*title, "menu.window.visibleitems", info.menuWindowVisibleItems);
        info.boxCursorVisible = IniFile::getBool(*title, "menu.boxcursor.visible", false);
        readRect4(*title, "menu.boxcursor.coords", info.boxCursorCoords);

        const DisplayElement itemFont = readElement(*title, "menu.item");
        if (itemFont.font.has_value()) info.menuItemFont = *itemFont.font;
        const DisplayElement activeFont = readElement(*title, "menu.item.active");
        if (activeFont.font.has_value()) info.menuItemActiveFont = *activeFont.font;

        constexpr std::string_view kItemPrefix = "menu.itemname.";
        for (const IniLine& line : title->lines) {
            if (line.key.size() <= kItemPrefix.size()) continue;
            if (line.key.compare(0, kItemPrefix.size(), kItemPrefix) != 0) continue;
            info.itemNames[line.key.substr(kItemPrefix.size())] = unquote(line.value);
        }
    }

    if (const IniSection* select = document.section("select info"); select != nullptr) {
        SelectInfo& info = def.select;
        info.fadeInTime = IniFile::getInt(*select, "fadein.time", info.fadeInTime);
        info.fadeOutTime = IniFile::getInt(*select, "fadeout.time", info.fadeOutTime);
        info.rows = IniFile::getInt(*select, "rows", info.rows);
        info.columns = IniFile::getInt(*select, "columns", info.columns);
        info.wrapping = IniFile::getBool(*select, "wrapping", info.wrapping);
        info.showEmptyBoxes = IniFile::getBool(*select, "showemptyboxes", info.showEmptyBoxes);
        info.moveOverEmptyBoxes =
            IniFile::getBool(*select, "moveoveremptyboxes", info.moveOverEmptyBoxes);
        info.cellSpacing = IniFile::getInt(*select, "cell.spacing", info.cellSpacing);
        info.cellRandomSwitchTime =
            IniFile::getInt(*select, "cell.random.switchtime", info.cellRandomSwitchTime);

        readVec2(*select, "pos", info.pos);
        readVec2(*select, "cell.size", info.cellSize);
        readVec2(*select, "portrait.scale", info.portraitScale);
        // startcell is written column,row -- not x,y.
        readVec2(*select, "p1.cursor.startcell", info.p1StartCell);
        readVec2(*select, "p2.cursor.startcell", info.p2StartCell);

        info.cellBg = readElement(*select, "cell.bg");
        info.cellRandom = readElement(*select, "cell.random");
        info.p1CursorActive = readElement(*select, "p1.cursor.active");
        info.p1CursorDone = readElement(*select, "p1.cursor.done");
        info.p2CursorActive = readElement(*select, "p2.cursor.active");
        info.p2CursorDone = readElement(*select, "p2.cursor.done");
        info.portrait = readElement(*select, "portrait");
        info.title = readElement(*select, "title");
        info.stage = readElement(*select, "stage.active");
        info.p1Face = readElement(*select, "p1.face");
        info.p2Face = readElement(*select, "p2.face");
        info.p1Name = readElement(*select, "p1.name");
        info.p2Name = readElement(*select, "p2.name");
        info.emptyMessage = readElement(*select, "emptymsg");
    }

    if (const IniSection* versus = document.section("vs screen"); versus != nullptr) {
        VsScreen& screen = def.versus;
        screen.time = IniFile::getInt(*versus, "time", screen.time);
        screen.fadeInTime = IniFile::getInt(*versus, "fadein.time", screen.fadeInTime);
        screen.fadeOutTime = IniFile::getInt(*versus, "fadeout.time", screen.fadeOutTime);
        screen.match = readElement(*versus, "match");
        screen.p1Sprite = readElement(*versus, "p1");
        screen.p2Sprite = readElement(*versus, "p2");
        screen.p1Name = readElement(*versus, "p1.name");
        screen.p2Name = readElement(*versus, "p2.name");
    }

    def.titleBg = parseBackground(document, "titlebgdef", "titlebg");
    def.selectBg = parseBackground(document, "selectbgdef", "selectbg");
    def.versusBg = parseBackground(document, "versusbgdef", "versusbg");
    // These two are the only post-match screens the reference names a BGdef
    // for. Win, Continue, Demo, Ending and Credits have no background section
    // in any shipped motif, so no empty record is invented for them.
    def.victoryBg = parseBackground(document, "victorybgdef", "victorybg");
    def.optionBg = parseBackground(document, "optionbgdef", "optionbg");

    // The sections below are parsed for availability only. Nothing in this
    // slice draws them, and no match/run state reads them yet.
    if (const IniSection* music = nonEmptySection(document, "music"); music != nullptr) {
        def.music.title = readMusicTrack(*music, "title", true);
        def.music.select = readMusicTrack(*music, "select", true);
        def.music.vs = readMusicTrack(*music, "vs", false);
        def.music.victory = readMusicTrack(*music, "victory", false);
        AF_LOGI("system.def: [Music] parsed (4 tracks)");
    }

    if (const IniSection* demo = nonEmptySection(document, "demo mode"); demo != nullptr) {
        DemoMode& mode = def.demo;
        mode.enabled = IniFile::getBool(*demo, "enabled", mode.enabled);
        mode.selectEnabled = IniFile::getBool(*demo, "select.enabled", mode.selectEnabled);
        mode.vsScreenEnabled = IniFile::getBool(*demo, "vsscreen.enabled", mode.vsScreenEnabled);
        mode.titleWaitTime = IniFile::getInt(*demo, "title.waittime", mode.titleWaitTime);
        mode.fightEndTime = IniFile::getInt(*demo, "fight.endtime", mode.fightEndTime);
        mode.fightPlayBgm = IniFile::getBool(*demo, "fight.playbgm", mode.fightPlayBgm);
        mode.fightStopBgm = IniFile::getBool(*demo, "fight.stopbgm", mode.fightStopBgm);
        mode.fightBarsDisplay =
            IniFile::getBool(*demo, "fight.bars.display", mode.fightBarsDisplay);
        mode.introWaitCycles =
            IniFile::getInt(*demo, "intro.waitcycles", mode.introWaitCycles);
        mode.debugInfo = IniFile::getInt(*demo, "debuginfo", mode.debugInfo);
        AF_LOGI("system.def: [Demo Mode] parsed (enabled=%d, title wait %d)",
                mode.enabled ? 1 : 0, mode.titleWaitTime);
    }

    if (const IniSection* continueSection = nonEmptySection(document, "continue screen");
        continueSection != nullptr) {
        ContinueScreen& screen = def.continueScreen;
        screen.enabled = IniFile::getBool(*continueSection, "enabled", screen.enabled);
        readVec2(*continueSection, "pos", screen.pos);
        if (const std::string_view value = IniFile::get(*continueSection, "storyboard");
            !value.empty()) {
            screen.storyboard = unquote(value);
        }

        // The `big` motif is the only one that writes these elements, and the
        // distribution's default motif is documented as "enabled but
        // text-less": the compiled defaults are therefore empty, not the big
        // motif's labels. A section that writes only `enabled` stays text-less
        // here too rather than borrowing another motif's strings.
        screen.prompt = readElement(*continueSection, "continue");
        screen.yes = readElement(*continueSection, "yes");
        screen.no = readElement(*continueSection, "no");
        screen.yesActive = readElement(*continueSection, "yes.active");
        screen.noActive = readElement(*continueSection, "no.active");

        AF_LOGI("system.def: [Continue Screen] parsed (enabled=%d)", screen.enabled ? 1 : 0);
    }

    if (const IniSection* over = nonEmptySection(document, "game over screen"); over != nullptr) {
        def.gameOver.enabled = IniFile::getBool(*over, "enabled", def.gameOver.enabled);
        if (const std::string_view value = IniFile::get(*over, "storyboard"); !value.empty()) {
            def.gameOver.storyboard = unquote(value);
        }
        AF_LOGI("system.def: [Game Over Screen] parsed (enabled=%d, storyboard %s)",
                def.gameOver.enabled ? 1 : 0,
                def.gameOver.storyboard.empty() ? "none" : def.gameOver.storyboard.c_str());
    }

    if (const IniSection* victory = nonEmptySection(document, "victory screen");
        victory != nullptr) {
        VictoryScreen& screen = def.victory;
        screen.enabled = IniFile::getBool(*victory, "enabled", screen.enabled);
        screen.time = IniFile::getInt(*victory, "time", screen.time);
        screen.fadeInTime = IniFile::getInt(*victory, "fadein.time", screen.fadeInTime);
        screen.fadeOutTime = IniFile::getInt(*victory, "fadeout.time", screen.fadeOutTime);

        DisplayElement portraitDefaults;
        portraitDefaults.sprite = std::make_pair(9000, 2);
        portraitDefaults.offset = {100.0f, 20.0f};
        portraitDefaults.window = Rect4{0.0f, 0.0f, 319.0f, 160.0f};
        screen.portrait = readElementWithDefaults(*victory, "p1", portraitDefaults);

        DisplayElement nameDefaults;
        nameDefaults.offset = {20.0f, 180.0f};
        nameDefaults.font = FontRef{3, 0, 1};
        screen.name = readElementWithDefaults(*victory, "p1.name", nameDefaults);

        DisplayElement quoteDefaults;
        quoteDefaults.text = "Winner!";
        quoteDefaults.offset = {20.0f, 192.0f};
        quoteDefaults.font = FontRef{2, 0, 1};
        quoteDefaults.window = Rect4{18.0f, 171.0f, 301.0f, 228.0f};
        screen.winQuote = readElementWithDefaults(*victory, "winquote", quoteDefaults);

        if (const std::string_view value = IniFile::get(*victory, "winquote.textwrap");
            !value.empty()) {
            screen.winQuoteTextwrap = unquote(value);
        }

        AF_LOGI("system.def: [Victory Screen] parsed (bg, %zu elements)",
                def.victoryBg.elements.size());
    }

    if (const IniSection* win = nonEmptySection(document, "win screen"); win != nullptr) {
        WinScreen& screen = def.win;
        screen.enabled = IniFile::getBool(*win, "enabled", screen.enabled);
        screen.fadeInTime = IniFile::getInt(*win, "fadein.time", screen.fadeInTime);
        screen.poseTime = IniFile::getInt(*win, "pose.time", screen.poseTime);
        screen.fadeOutTime = IniFile::getInt(*win, "fadeout.time", screen.fadeOutTime);

        DisplayElement textDefaults;
        textDefaults.text = "Congratulations!";
        textDefaults.font = FontRef{2, 0, 0};
        textDefaults.offset = {159.0f, 70.0f};
        textDefaults.displaytime = -1;
        textDefaults.layerno = 2;
        screen.winText = readElementWithDefaults(*win, "wintext", textDefaults);

        AF_LOGI("system.def: [Win Screen] parsed (pose %d ticks)", screen.poseTime);
    }

    if (const IniSection* survival = nonEmptySection(document, "survival results screen");
        survival != nullptr) {
        SurvivalResultsScreen& screen = def.survival;
        screen.enabled = IniFile::getBool(*survival, "enabled", screen.enabled);
        screen.fadeInTime = IniFile::getInt(*survival, "fadein.time", screen.fadeInTime);
        screen.showTime = IniFile::getInt(*survival, "show.time", screen.showTime);
        screen.fadeOutTime = IniFile::getInt(*survival, "fadeout.time", screen.fadeOutTime);
        screen.roundsToWin = IniFile::getInt(*survival, "roundstowin", screen.roundsToWin);

        DisplayElement textDefaults;
        textDefaults.text = "Rounds survived: %i";
        textDefaults.font = FontRef{3, 0, 0};
        textDefaults.offset = {159.0f, 70.0f};
        textDefaults.displaytime = -1;
        textDefaults.layerno = 2;
        screen.winText = readElementWithDefaults(*survival, "winstext", textDefaults);

        AF_LOGI("system.def: [Survival Results Screen] parsed (rounds to win %d)",
                screen.roundsToWin);
    }

    if (const IniSection* ending = nonEmptySection(document, "default ending");
        ending != nullptr) {
        def.defaultEnding.enabled =
            IniFile::getBool(*ending, "enabled", def.defaultEnding.enabled);
        if (const std::string_view value = IniFile::get(*ending, "storyboard"); !value.empty()) {
            def.defaultEnding.storyboard = unquote(value);
        }
        AF_LOGI("system.def: [Default Ending] parsed (enabled=%d, storyboard %s)",
                def.defaultEnding.enabled ? 1 : 0,
                def.defaultEnding.storyboard.empty() ? "none"
                                                     : def.defaultEnding.storyboard.c_str());
    }

    if (const IniSection* credits = nonEmptySection(document, "end credits"); credits != nullptr) {
        def.endCredits.enabled = IniFile::getBool(*credits, "enabled", def.endCredits.enabled);
        if (const std::string_view value = IniFile::get(*credits, "storyboard"); !value.empty()) {
            def.endCredits.storyboard = unquote(value);
        }
        AF_LOGI("system.def: [End Credits] parsed (enabled=%d, storyboard %s)",
                def.endCredits.enabled ? 1 : 0,
                def.endCredits.storyboard.empty() ? "none" : def.endCredits.storyboard.c_str());
    }

    if (const IniSection* option = nonEmptySection(document, "option info"); option != nullptr) {
        OptionInfo& info = def.option;
        readIntPair(*option, "cursor.move.snd", info.cursorMoveSound);
        readIntPair(*option, "cursor.done.snd", info.cursorDoneSound);
        readIntPair(*option, "cancel.snd", info.cancelSound);
        info.title = readElement(*option, "title");
        AF_LOGI("system.def: [Option Info] parsed (sounds, bg %zu elements)",
                def.optionBg.elements.size());
    }

    parseAnimations(document, def.animations);

    AF_LOGI("system.def: \"%s\" localcoord %.0fx%.0f, %zu animations, titlebg %zu elements",
            def.name.c_str(), def.localCoord.x, def.localCoord.y, def.animations.size(),
            def.titleBg.elements.size());
    return def;
}

FightDef parseFightDef(const IniFile& document) {
    FightDef def;

    if (const IniSection* files = document.section("files"); files != nullptr) {
        if (const std::string_view v = IniFile::get(*files, "sff"); !v.empty()) {
            def.spriteFile = unquote(v);
        }
        if (const std::string_view v = IniFile::get(*files, "snd"); !v.empty()) {
            def.soundFile = unquote(v);
        }
        if (const std::string_view v = IniFile::get(*files, "fightfx.sff"); !v.empty()) {
            def.fightFxSpriteFile = unquote(v);
        }
        if (const std::string_view v = IniFile::get(*files, "fightfx.air"); !v.empty()) {
            def.fightFxAirFile = unquote(v);
        }
        if (const std::string_view v = IniFile::get(*files, "common.snd"); !v.empty()) {
            def.commonSoundFile = unquote(v);
        }
        parseFontTable(files, def.fonts);
    }

    if (const IniSection* fx = document.section("fightfx"); fx != nullptr) {
        def.fightFxScale = IniFile::getFloat(*fx, "scale", def.fightFxScale);
    }

    if (const IniSection* lifebar = document.section("lifebar"); lifebar != nullptr) {
        def.lifebarP1 = readBarSide(*lifebar, "p1");
        def.lifebarP2 = readBarSide(*lifebar, "p2");
    }

    if (const IniSection* powerbar = document.section("powerbar"); powerbar != nullptr) {
        def.powerbarP1 = readBarSide(*powerbar, "p1");
        def.powerbarP2 = readBarSide(*powerbar, "p2");
    }

    if (const IniSection* face = document.section("face"); face != nullptr) {
        def.faceP1 = readFaceSide(*face, "p1");
        def.faceP2 = readFaceSide(*face, "p2");
    }

    if (const IniSection* name = document.section("name"); name != nullptr) {
        def.nameP1 = readNameSide(*name, "p1");
        def.nameP2 = readNameSide(*name, "p2");
    }

    if (const IniSection* time = document.section("time"); time != nullptr) {
        def.time.present = true;
        readVec2(*time, "pos", def.time.pos);
        def.time.bg = readElement(*time, "bg");
        def.time.counter = readElement(*time, "counter");
        def.time.framesPerCount = IniFile::getInt(*time, "framespercount", def.time.framesPerCount);
    }

    if (const IniSection* round = document.section("round"); round != nullptr) {
        RoundSection& rs = def.round;
        rs.matchWins = IniFile::getInt(*round, "match.wins", rs.matchWins);
        rs.startWaitTime = IniFile::getInt(*round, "start.waittime", rs.startWaitTime);
        rs.controlTime = IniFile::getInt(*round, "ctrl.time", rs.controlTime);

        rs.roundDefault = readElement(*round, "round.default");
        rs.roundTime = IniFile::getInt(*round, "round.time", 0);
        rs.roundDisplayTime =
            IniFile::getInt(*round, "round.default.displaytime", rs.roundDisplayTime);

        rs.fight = readElement(*round, "fight");
        rs.fightTime = IniFile::getInt(*round, "fight.time", 0);

        rs.ko = readElement(*round, "ko");
        rs.koTime = IniFile::getInt(*round, "ko.time", 0);

        rs.doubleKo = readElement(*round, "dko");
        rs.timeOver = readElement(*round, "to");
        rs.win = readElement(*round, "win");
        rs.win2 = readElement(*round, "win2");
        rs.draw = readElement(*round, "draw");

        rs.slowTime = IniFile::getInt(*round, "slow.time", rs.slowTime);
        rs.overWaitTime = IniFile::getInt(*round, "over.waittime", rs.overWaitTime);
        rs.overHitTime = IniFile::getInt(*round, "over.hittime", rs.overHitTime);
        rs.overWinTime = IniFile::getInt(*round, "over.wintime", rs.overWinTime);
        rs.overTime = IniFile::getInt(*round, "over.time", rs.overTime);
        rs.winTime = IniFile::getInt(*round, "win.time", rs.winTime);

        for (int n = 1; n <= 9; ++n) {
            DisplayElement banner = readElement(*round, "round" + std::to_string(n));
            if (banner.present) rs.roundBanners[n] = banner;
        }
    }

    if (const IniSection* winIcon = document.section("winicon"); winIcon != nullptr) {
        def.winIconP1 = readWinIconSide(*winIcon, "p1");
        def.winIconP2 = readWinIconSide(*winIcon, "p2");
        def.winIconUseUpTo = IniFile::getInt(*winIcon, "useiconupto", def.winIconUseUpTo);
    }

    parseAnimations(document, def.animations);

    AF_LOGI("fight.def: sff \"%s\", %zu animations, lifebar p1 %s", def.spriteFile.c_str(),
            def.animations.size(), def.lifebarP1.present ? "present" : "absent");
    return def;
}

Config parseConfig(const IniFile& document) {
    Config config;

    if (const IniSection* c = document.section("config"); c != nullptr) {
        config.gameSize.x = IniFile::getFloat(*c, "gamewidth", config.gameSize.x);
        config.gameSize.y = IniFile::getFloat(*c, "gameheight", config.gameSize.y);
        config.gameSpeed = IniFile::getInt(*c, "gamespeed", config.gameSpeed);
        // `drawshadows`, the global switch every shadow pass tests. Its
        // documented default is 1 (SHADOW-RE §0 fact 8); any non-zero value
        // enables it, as in the reference.
        config.drawShadows = IniFile::getInt(*c, "drawshadows", config.drawShadows ? 1 : 0) != 0;
    }

    if (const IniSection* options = document.section("options"); options != nullptr) {
        if (const std::string_view v = IniFile::get(*options, "motif"); !v.empty()) {
            config.motif = unquote(v);
        }
    }

    if (const IniSection* video = document.section("video"); video != nullptr) {
        // Our own extension to the format. Classic MUGEN always stretches.
        if (const std::string_view v = IniFile::get(*video, "aspect"); !v.empty()) {
            config.aspect = toLower(v) == "stretch" ? AspectMode::Stretch : AspectMode::Fit;
        }
    }

    AF_LOGI("mugen.cfg: game space %.0fx%.0f @ %d Hz, motif \"%s\"", config.gameSize.x,
            config.gameSize.y, config.gameSpeed, config.motif.c_str());
    return config;
}

SelectDef parseSelectDef(const IniFile& document) {
    SelectDef def;

    // Roster lines are read from `raw`, not from key/value: a line like
    // `kfm, music = song.mp3` contains an '=' and would otherwise be shredded
    // into a key called "kfm, music".
    if (const IniSection* characters = document.section("characters"); characters != nullptr) {
        for (const IniLine& line : characters->lines) {
            if (line.raw.empty()) continue;

            RosterEntry entry;
            if (toLower(line.raw) == "randomselect") {
                entry.isRandomCell = true;
                def.characters.push_back(std::move(entry));
                continue;
            }

            const std::vector<std::string> parts = splitCommas(line.raw);
            if (parts.empty()) continue;

            entry.defPath = parts[0];
            for (size_t i = 1; i < parts.size(); ++i) {
                const size_t equals = parts[i].find('=');
                if (equals == std::string::npos) {
                    // A stage filename, in the order they should be used.
                    entry.stages.push_back(parts[i]);
                    continue;
                }
                const std::string name =
                    toLower(trim(std::string_view(parts[i]).substr(0, equals)));
                const std::string_view value = trim(std::string_view(parts[i]).substr(equals + 1));
                if (name == "hidden") tryParseInt(value, entry.hidden);
            }
            def.characters.push_back(std::move(entry));
        }
    }

    if (const IniSection* stages = document.section("extrastages"); stages != nullptr) {
        for (const IniLine& line : stages->lines) {
            if (!line.raw.empty()) def.extraStages.push_back(line.raw);
        }
    }

    if (const IniSection* options = document.section("options"); options != nullptr) {
        for (const std::string& piece : splitCommas(IniFile::get(*options, "arcade.maxmatches"))) {
            int fights = 0;
            if (tryParseInt(piece, fights)) def.arcadeMaxMatches.push_back(fights);
        }
        // Read for the same reason: it is the same key in the same section, and
        // leaving it unread means a player editing it sees nothing happen. Team
        // modes are not implemented yet, so nothing consumes it -- but a config
        // key that is silently ignored is worse than one that is logged.
        for (const std::string& piece : splitCommas(IniFile::get(*options, "team.maxmatches"))) {
            int fights = 0;
            if (tryParseInt(piece, fights)) def.teamMaxMatches.push_back(fights);
        }
    }

    AF_LOGI("select.def: %zu roster entries (%zu selectable), %zu extra stages",
            def.characters.size(),
            static_cast<size_t>(std::count_if(def.characters.begin(), def.characters.end(),
                                             [](const RosterEntry& e) {
                                                 return !e.defPath.empty() && e.hidden == 0;
                                             })),
            def.extraStages.size());
    if (!def.arcadeMaxMatches.empty()) {
        AF_LOGI("select.def: arcade run is %d fight(s)", def.arcadeMatchCount());
    }
    if (!def.teamMaxMatches.empty()) {
        AF_LOGI("select.def: team modes are configured for %zu tier(s); not implemented",
                def.teamMaxMatches.size());
    }
    return def;
}

}  // namespace af
