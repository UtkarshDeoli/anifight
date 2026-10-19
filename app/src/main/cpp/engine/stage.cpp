#include "engine/stage.h"

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>

#include "core/log.h"

namespace af {

namespace {

void readFloat(const IniSection& section, std::string_view key, float& out) {
    const std::string_view value = IniFile::get(section, key);
    if (!value.empty()) tryParseFloat(value, out);
}

// A stage writes most of its vectors as two comma-separated numbers, but a few
// -- p1startx / p1starty -- as separate keys. This covers the pair form.
void readVec2(const IniSection& section, std::string_view key, Vec2& out) {
    const std::string_view value = IniFile::get(section, key);
    if (value.empty()) return;
    const std::vector<std::string> parts = splitCommas(value);
    if (parts.size() >= 1) tryParseFloat(parts[0], out.x);
    if (parts.size() >= 2) tryParseFloat(parts[1], out.y);
}

// MUGEN writes a colour as three 0..255 values, which is not the 0..1 the
// renderer wants.
void readColor(const IniSection& section, std::string_view key, Color& out) {
    const std::string_view value = IniFile::get(section, key);
    if (value.empty()) return;
    const std::vector<std::string> parts = splitCommas(value);
    int r = 0, g = 0, b = 0;
    if (parts.size() >= 1) tryParseInt(parts[0], r);
    if (parts.size() >= 2) tryParseInt(parts[1], g);
    if (parts.size() >= 3) tryParseInt(parts[2], b);
    out.r = static_cast<float>(std::clamp(r, 0, 255)) / 255.0f;
    out.g = static_cast<float>(std::clamp(g, 0, 255)) / 255.0f;
    out.b = static_cast<float>(std::clamp(b, 0, 255)) / 255.0f;
}

}  // namespace

std::string resolveStageFile(const std::string& folder, const std::string& file) {
    if (file.empty()) return {};
    if (file[0] == '/') return file;
    // The fully-qualified spelling is what the stage author meant, and adding
    // the folder to it would double it. The comparison includes the folder's
    // trailing slash, so `stages/` does not swallow `stages2/x.sff`.
    if (!folder.empty() && file.compare(0, folder.size(), folder) == 0) return file;
    return folder + file;
}

StageTranslation stageTranslation(const StageDef& def, float viewportHeight) {
    const float sourceHeight = def.localCoord.y > 0.0f ? def.localCoord.y : 240.0f;
    const float base = viewportHeight > 0.0f ? viewportHeight / sourceHeight : 1.0f;
    StageTranslation translation;
    translation.x = base * (def.scaleX > 0.0f ? def.scaleX : 1.0f);
    translation.y = base * (def.scaleY > 0.0f ? def.scaleY : 1.0f);
    return translation;
}

std::string stageDefPath(const std::string& entry) {
    const std::string path(trim(entry));
    if (path.empty()) return {};

    // Resolved from the asset root, the same way game.cpp resolves a roster
    // entry: select.def's entries are written as `stages/x.def` or as just `x`,
    // and MUGEN accepts both. A stage is a sibling of chars/, not a screenpack
    // file, so it does not live under data/.
    if (path.rfind("stages/", 0) == 0) return path;
    if (path.find(".def") != std::string::npos) return "stages/" + path;
    return "stages/" + path + "/" + path + ".def";
}

StageDef parseStageDef(const IniFile& document) {
    StageDef def;

    if (const IniSection* info = document.section("info"); info != nullptr) {
        def.name = unquote(IniFile::get(*info, "name"));
        def.displayName = unquote(IniFile::get(*info, "displayname"));
        def.author = unquote(IniFile::get(*info, "author"));
    }
    if (def.displayName.empty()) def.displayName = def.name;

    // The camera's travel limits are what stop a fight from walking off the end
    // of the art, so they are read from [Camera] where MUGEN puts them. Some
    // stages repeat them under [Bound] as the distance from the screen edge;
    // boundleft/boundright are the ones this engine uses.
    if (const IniSection* camera = document.section("camera"); camera != nullptr) {
        readFloat(*camera, "boundleft", def.boundLeft);
        readFloat(*camera, "boundright", def.boundRight);
        readFloat(*camera, "startx", def.cameraStart.x);
        readFloat(*camera, "starty", def.cameraStart.y);
    }

    if (const IniSection* players = document.section("playerinfo"); players != nullptr) {
        readFloat(*players, "p1startx", def.p1Start.x);
        readFloat(*players, "p1starty", def.p1Start.y);
        readFloat(*players, "p2startx", def.p2Start.x);
        readFloat(*players, "p2starty", def.p2Start.y);
        readFloat(*players, "leftbound", def.playerLeftBound);
        readFloat(*players, "rightbound", def.playerRightBound);
    }

    // `[Bound]` is the distance a player keeps from the screen's edge once the
    // camera has run out of travel, so it is geometry in the stage's own units
    // like every other number here.
    if (const IniSection* bound = document.section("bound"); bound != nullptr) {
        readFloat(*bound, "screenleft", def.screenLeft);
        readFloat(*bound, "screenright", def.screenRight);
    }

    if (const IniSection* stage = document.section("stageinfo"); stage != nullptr) {
        readFloat(*stage, "zoffset", def.zOffset);
        readVec2(*stage, "localcoord", def.localCoord);
        readFloat(*stage, "xscale", def.scaleX);
        readFloat(*stage, "yscale", def.scaleY);
        // A stage that names a zero or negative localcoord has not described a
        // usable coordinate space; leaving the default in place keeps it
        // playable rather than making the translation infinite.
        if (!(def.localCoord.x > 0.0f)) def.localCoord.x = 320.0f;
        if (!(def.localCoord.y > 0.0f)) def.localCoord.y = 240.0f;
    }

    // A stage that says nothing about shadows gets MUGEN's own defaults, which
    // are a soft black one -- so the section is optional in the way every other
    // optional section is.
    if (const IniSection* shadow = document.section("shadow"); shadow != nullptr) {
        readColor(*shadow, "color", def.shadow.color);
        readFloat(*shadow, "intensity", def.shadow.intensity);
        readFloat(*shadow, "yscale", def.shadow.yScale);
        // `fade.range = top_y, bot_y`, in MUGEN's stage-y direction. Omitted
        // leaves both zero, which `ShadowStyle::fadeActive` reads as "no fade"
        // -- the same gate the reference uses (`top < bottom`).
        Vec2 fade;
        readVec2(*shadow, "fade.range", fade);
        def.shadow.fadeTop = fade.x;
        def.shadow.fadeBottom = fade.y;
    }

    // `[Reflection] intensity`, not the `reflect` flag older documentation
    // describes: the 1.0 binary reads `intensity` (SHADOW-RE §0 fact 7). Zero
    // means off; the fighter/helper draw path mirrors the current sprite at
    // this alpha when it is positive.
    if (const IniSection* reflection = document.section("reflection");
        reflection != nullptr) {
        readFloat(*reflection, "intensity", def.shadow.reflectionIntensity);
    }

    if (const IniSection* bg = document.section("bgdef"); bg != nullptr) {
        def.spriteFile = IniFile::get(*bg, "spr");
    }

    // The layers themselves. `[BGdef]` is the sprite file, `[BG n]` are the
    // layers, and the space in the prefix is what keeps the two apart.
    def.background = parseBackground(document, "bgdef", "bg");
    return def;
}

bool Stage::load(FileSource& files, const std::string& defPath, std::string* errorOut) {
    defPath_ = defPath;

    std::unique_ptr<FileData> document = files.read(defPath);
    if (!document) {
        if (errorOut != nullptr) *errorOut = "stage def not found: " + defPath;
        return false;
    }

    def_ = parseStageDef(IniFile::parse(std::string_view(
        reinterpret_cast<const char*>(document->data()), document->size())));

    const size_t slash = defPath.find_last_of('/');
    const std::string folder =
        (slash == std::string::npos) ? std::string() : defPath.substr(0, slash + 1);

    if (!def_.spriteFile.empty()) {
        const std::string path = resolveStageFile(folder, def_.spriteFile);
        std::string error;
        if (!sprites_.load(files, path, &error)) {
            AF_LOGW("stage '%s': sprites failed: %s", def_.name.c_str(), error.c_str());
        }
    }

    AF_LOGI("stage '%s' by %s: %zu layers, floor y %.0f, camera %.0f..%.0f, %zu sprites, "
            "shadow %.0f%% yscale %.2f",
            def_.name.empty() ? "(unnamed)" : def_.name.c_str(),
            def_.author.empty() ? "(unknown)" : def_.author.c_str(), def_.background.elements.size(),
            def_.zOffset, def_.boundLeft, def_.boundRight, sprites_.size(),
            100.0f * def_.shadow.intensity / 256.0f, def_.shadow.yScale);

    loaded_ = true;
    return true;
}

}  // namespace af
