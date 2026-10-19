#include "engine/storyboard.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

namespace af {

namespace {

// MUGEN's storyboard values are comma lists with the same leniency as every
// other .def value. A malformed entry leaves the field alone rather than
// invalidating the scene.
bool parsePair(std::string_view value, Vec2& out) {
    const std::vector<std::string> parts = splitCommas(value);
    if (parts.size() < 2) return false;
    float x = 0.0f;
    float y = 0.0f;
    if (!tryParseFloat(parts[0], x) || !tryParseFloat(parts[1], y)) return false;
    out.x = x;
    out.y = y;
    return true;
}

int parseIntOr(std::string_view value, int fallback) {
    int out = fallback;
    return tryParseInt(value, out) ? out : fallback;
}

int clampByte(int value) {
    return std::clamp(value, 0, 255);
}

bool parseRgb(std::string_view value, Color& out) {
    const std::vector<std::string> parts = splitCommas(value);
    if (parts.size() < 3) return false;
    int r = 0;
    int g = 0;
    int b = 0;
    if (!tryParseInt(parts[0], r) || !tryParseInt(parts[1], g) || !tryParseInt(parts[2], b)) {
        return false;
    }
    out = Color::rgba8(static_cast<uint8_t>(clampByte(r)), static_cast<uint8_t>(clampByte(g)),
                       static_cast<uint8_t>(clampByte(b)));
    return true;
}

// `[Scene ?]` text is quoted and uses literal `\n` for a line break. By the
// time it reaches here IniFile has kept the quotes, so they come off first.
std::string decodeText(std::string_view value) {
    const std::string quoted = unquote(value);
    std::string out;
    out.reserve(quoted.size());
    for (size_t i = 0; i < quoted.size(); ++i) {
        if (quoted[i] == '\\' && i + 1 < quoted.size() && quoted[i + 1] == 'n') {
            out.push_back('\n');
            ++i;
        } else {
            out.push_back(quoted[i]);
        }
    }
    return out;
}

// Reads a numbered child key such as `layer12.offset` or `sound3.value`.
// Returns false for anything with another prefix, for a field before the dot,
// and for an index outside the documented 0..99 range.
bool indexedKey(std::string_view key, std::string_view prefix, int& index,
                std::string_view& field) {
    if (key.size() <= prefix.size() || key.substr(0, prefix.size()) != prefix) return false;
    size_t at = prefix.size();
    const size_t digitsBegin = at;
    int value = 0;
    while (at < key.size() && std::isdigit(static_cast<unsigned char>(key[at]))) {
        value = value * 10 + (key[at] - '0');
        ++at;
    }
    if (at == digitsBegin) return false;
    if (at >= key.size() || key[at] != '.') return false;
    if (value < 0 || value > 99) return false;
    index = value;
    field = key.substr(at + 1);
    return true;
}

// `layerN.font = font_no, bank, align, r, g, b`. Missing fields take the
// reference's defaults (bank 0, centre, white); an omitted font number leaves
// the layer text-less rather than guessing font 0.
void parseLayerFont(std::string_view value, StoryboardLayer& layer) {
    const std::vector<std::string> parts = splitCommas(value);
    if (parts.empty()) return;
    int fontNo = 0;
    if (tryParseInt(parts[0], fontNo)) layer.font = fontNo;
    if (parts.size() >= 2) {
        int bank = layer.bank;
        if (tryParseInt(parts[1], bank)) layer.bank = bank;
    }
    if (parts.size() >= 3) {
        int align = layer.align;
        if (tryParseInt(parts[2], align)) layer.align = align;
    }
    if (parts.size() >= 6) {
        int r = 255;
        int g = 255;
        int b = 255;
        if (tryParseInt(parts[3], r) && tryParseInt(parts[4], g) && tryParseInt(parts[5], b)) {
            layer.color = Color::rgba8(static_cast<uint8_t>(clampByte(r)),
                                       static_cast<uint8_t>(clampByte(g)),
                                       static_cast<uint8_t>(clampByte(b)));
        }
    }
}

}  // namespace

StoryboardDef parseStoryboard(const IniFile& document) {
    StoryboardDef def;

    if (const IniSection* info = document.section("info")) {
        Vec2 local = def.localCoord;
        if (parsePair(IniFile::get(*info, "localcoord"), local) && local.x > 0.0f &&
            local.y > 0.0f) {
            def.localCoord = local;
        }
    }

    if (const IniSection* sceneDef = document.section("scenedef")) {
        def.spriteFile = unquote(IniFile::get(*sceneDef, "spr"));
        def.soundFile = unquote(IniFile::get(*sceneDef, "snd"));
        for (int i = 0; i < 10; ++i) {
            const std::string key = "font" + std::to_string(i);
            def.fontFiles[static_cast<size_t>(i)] = unquote(IniFile::get(*sceneDef, key));
        }
        def.startScene = std::max(0, IniFile::getInt(*sceneDef, "startscene", 0));
    }

    // The two documented inheritance rules. `layerall.pos` is one value carried
    // from scene to scene, and `clearcolor` is explicitly "previous scene's
    // value when omitted"; the first scene defaults to no clear. `bgm` behaves
    // the same way -- it keeps playing until a scene changes it -- so it is
    // carried too, with an empty value meaning "stopped".
    Vec2 layerAllPos;
    bool clearScreen = false;
    Color clearColor{0.0f, 0.0f, 0.0f, 1.0f};
    std::string bgm;
    bool bgmLoop = false;

    for (const IniSection& section : document.sections()) {
        if (section.name.size() <= 6 || section.name.compare(0, 6, "scene ") != 0) continue;

        StoryboardScene scene;
        scene.id = section.name.substr(6);
        scene.layerAllPos = layerAllPos;
        scene.clearScreen = clearScreen;
        scene.clearColor = clearColor;
        scene.bgm = bgm;
        scene.bgmLoop = bgmLoop;

        for (const IniLine& line : section.lines) {
            if (line.key.empty()) continue;

            if (line.key == "end.time") {
                scene.endTime = std::max(0, parseIntOr(line.value, 0));
            } else if (line.key == "fadein.time") {
                scene.fadeInTime = std::max(0, parseIntOr(line.value, 0));
            } else if (line.key == "fadeout.time") {
                scene.fadeOutTime = std::max(0, parseIntOr(line.value, 0));
            } else if (line.key == "fadein.col") {
                parseRgb(line.value, scene.fadeInColor);
            } else if (line.key == "fadeout.col") {
                parseRgb(line.value, scene.fadeOutColor);
            } else if (line.key == "clearcolor") {
                const std::vector<std::string> parts = splitCommas(line.value);
                int r = 0;
                if (!parts.empty() && tryParseInt(parts[0], r)) {
                    if (r < 0) {
                        // `r = -1` is MUGEN's "leave the screen as it is".
                        // This screen cannot preserve the previous frame --
                        // Engine clears black before every draw -- so the flag
                        // survives parsing and the screen documents the
                        // substitution it makes.
                        scene.clearScreen = false;
                    } else if (parseRgb(line.value, scene.clearColor)) {
                        scene.clearScreen = true;
                    }
                }
            } else if (line.key == "layerall.pos") {
                parsePair(line.value, scene.layerAllPos);
            } else if (line.key == "bgm") {
                scene.bgm = unquote(line.value);
            } else if (line.key == "bgm.loop") {
                scene.bgmLoop = IniFile::getBool(section, "bgm.loop", scene.bgmLoop);
            } else if (line.key == "bg.name") {
                // Recognised only so the screen can say it is unsupported.
                scene.bgName = unquote(line.value);
            } else {
                int index = 0;
                std::string_view field;
                if (indexedKey(line.key, "layer", index, field)) {
                    StoryboardLayer& layer = scene.layers[static_cast<size_t>(index)];
                    if (field == "anim") {
                        int anim = -1;
                        if (tryParseInt(line.value, anim)) {
                            layer.anim = anim;
                            layer.present = true;
                        }
                    } else if (field == "text") {
                        layer.text = decodeText(line.value);
                        layer.hasText = true;
                        layer.present = true;
                    } else if (field == "font") {
                        parseLayerFont(line.value, layer);
                        layer.present = true;
                    } else if (field == "offset") {
                        parsePair(line.value, layer.offset);
                        layer.present = true;
                    } else if (field == "scale") {
                        parsePair(line.value, layer.scale);
                        layer.present = true;
                    } else if (field == "starttime") {
                        layer.startTime = std::max(0, parseIntOr(line.value, 0));
                        layer.present = true;
                    } else if (field == "endtime") {
                        layer.endTime = parseIntOr(line.value, -1);
                        layer.present = true;
                    } else if (field == "textdelay") {
                        layer.textDelay = std::max(0, parseIntOr(line.value, 0));
                        layer.present = true;
                    }
                    // `layerN.window`, `layerN.layerno` and `layerN.pos` are not
                    // storyboard keys; anything else is ignored like MUGEN
                    // ignores unknown keys.
                } else if (indexedKey(line.key, "sound", index, field)) {
                    StoryboardSound& sound = scene.sounds[static_cast<size_t>(index)];
                    if (field == "value") {
                        const std::vector<std::string> parts = splitCommas(line.value);
                        int group = 0;
                        int sample = 0;
                        if (parts.size() >= 2 && tryParseInt(parts[0], group) &&
                            tryParseInt(parts[1], sample)) {
                            sound.group = group;
                            sound.sample = sample;
                            sound.present = true;
                        }
                    } else if (field == "starttime") {
                        sound.startTime = std::max(0, parseIntOr(line.value, 0));
                    } else if (field == "volumescale") {
                        float volume = sound.volumeScale;
                        if (tryParseFloat(line.value, volume)) sound.volumeScale = volume;
                    } else if (field == "pan") {
                        int pan = 0;
                        if (tryParseInt(line.value, pan)) sound.pan = pan;
                    }
                }
            }
        }

        layerAllPos = scene.layerAllPos;
        clearScreen = scene.clearScreen;
        clearColor = scene.clearColor;
        bgm = scene.bgm;
        bgmLoop = scene.bgmLoop;

        def.scenes.push_back(std::move(scene));
    }

    // Inline actions use the same parser as a screenpack's system.def or a
    // character's .air; collision boxes are irrelevant to a storyboard and are
    // simply ignored by the draw.
    parseAnimations(document, def.animations);

    return def;
}

StoryboardTiming::StoryboardTiming(const std::vector<Scene>& scenes) {
    long long at = 0;
    starts_.reserve(scenes.size());
    lengths_.reserve(scenes.size());
    fadeIns_.reserve(scenes.size());
    fadeOuts_.reserve(scenes.size());
    for (const Scene& scene : scenes) {
        const long long length = scene.endTime > 0 ? static_cast<long long>(scene.endTime) : 0;
        starts_.push_back(at);
        lengths_.push_back(length);
        fadeIns_.push_back(scene.fadeInTime > 0 ? static_cast<long long>(scene.fadeInTime) : 0);
        fadeOuts_.push_back(scene.fadeOutTime > 0 ? static_cast<long long>(scene.fadeOutTime) : 0);
        at += length;
    }
    total_ = at;
}

StoryboardTiming StoryboardTiming::fromStoryboard(const StoryboardDef& def) {
    std::vector<Scene> scenes;
    scenes.reserve(def.scenes.size());
    for (const StoryboardScene& scene : def.scenes) {
        scenes.push_back(Scene{scene.endTime, scene.fadeInTime, scene.fadeOutTime});
    }
    return StoryboardTiming(scenes);
}

void StoryboardTiming::startAtScene(size_t scene) {
    if (starts_.empty()) return;
    tick_ = starts_[std::min(scene, starts_.size() - 1)];
}

size_t StoryboardTiming::sceneAt(long long tick) const {
    if (starts_.empty()) return 0;
    if (tick < 0) tick = 0;
    for (size_t i = 0; i < starts_.size(); ++i) {
        if (tick < starts_[i] + lengths_[i]) return i;
    }
    return starts_.size() - 1;
}

long long StoryboardTiming::sceneStart(size_t scene) const {
    if (starts_.empty()) return 0;
    return starts_[std::min(scene, starts_.size() - 1)];
}

long long StoryboardTiming::sceneLength(size_t scene) const {
    if (lengths_.empty()) return 0;
    return lengths_[std::min(scene, lengths_.size() - 1)];
}

long long StoryboardTiming::sceneLocalTick(long long tick) const {
    if (starts_.empty()) return 0;
    const long long local = tick - sceneStart(sceneAt(tick));
    return local > 0 ? local : 0;
}

StoryboardTiming::Fade StoryboardTiming::fadeAt(long long tick) const {
    if (starts_.empty()) return {};

    const size_t scene = sceneAt(tick);
    const long long local = sceneLocalTick(tick);

    float alpha = 0.0f;
    bool useFadeOutColor = false;

    // Fade-in: opaque at the scene's first tick, clear on `fadein.time`. A
    // zero-length fade-in is not an overlay at all.
    const long long fadeIn = fadeIns_[scene];
    if (fadeIn > 0 && local < fadeIn) {
        alpha = 1.0f - static_cast<float>(local) / static_cast<float>(fadeIn);
    }

    // Fade-out: clear until `end - fadeout.time`, then darkens to opaque on the
    // scene's `end.time`. `fadeout.time` larger than the scene is clamped so the
    // ramp starts at the scene's beginning instead of leaving the screen clear
    // through a fade the file explicitly asked for.
    const long long fadeOut = fadeOuts_[scene];
    if (fadeOut > 0) {
        long long outStart = lengths_[scene] - fadeOut;
        if (outStart < 0) outStart = 0;
        if (local >= outStart) {
            float outAlpha = static_cast<float>(local - outStart) / static_cast<float>(fadeOut);
            if (outAlpha > 1.0f) outAlpha = 1.0f;
            if (outAlpha > alpha) {
                alpha = outAlpha;
                useFadeOutColor = true;
            }
        }
    }

    return Fade{alpha, useFadeOutColor};
}

}  // namespace af
