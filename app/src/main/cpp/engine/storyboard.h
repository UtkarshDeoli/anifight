// MUGEN 1.0 storyboards: the scene list behind `[Game Over Screen]`,
// `[Default Ending]` and `[End Credits]`.
//
// A storyboard is one .def file with three kinds of section:
//
//     [Info]       localcoord = width, height
//     [SceneDef]   spr, snd, font0..font9, startscene
//     [Scene ?]    one scene each, plus inline `[Begin Action N]` blocks
//
// Everything on screen is a *layer*. A scene draws layers 0..99 in numeric
// order -- 0 is the bottom -- each of which is one of:
//
//     layerN.anim       an animation from this same file
//     layerN.text       text, drawn with one of the storyboard's fonts
//
// positioned at `layerall.pos` plus the layer's own offset. `starttime` and
// `endtime` are scene-local tick counts; `end.time` ends the scene itself.
// `fadein`/`fadeout` are a whole-screen overlay, and `clearcolor` is the
// backdrop under everything.
//
// Two inheritance rules matter and are easy to get wrong:
//
//   * `layerall.pos` and `clearcolor` are *sticky*. The reference documents the
//     first as "inherited from the previous scene" and the second explicitly:
//     an omitted `clearcolor` in a later scene keeps the previous one.
//   * `bgm` plays across scenes until another scene changes it, so omitting it
//     means "keep playing", not "silence".
//
// The layer/sound range is 0..99 in the reference's documentation. This parser
// keeps that whole range: the task that added the player only needs 0..9, but
// rejecting 10 would fail a storyboard the format accepts, and nothing here
// costs anything per unused slot.
//
// `StoryboardTiming` is the pure half -- scene boundaries, fade arithmetic and
// the total length -- so the host suite can pin the clock without a renderer.
//
// Deliberately not modelled: `bg.name` background objects (their own `[BGnDef]`
// and controller syntax), `layerN.window`, `layerN.layerno` and `layerN.pos`.
// Only `window`/`layerno`/`pos` are documented for *screenpack elements*, not
// storyboard layers; a layer's position is `layerall.pos + layerN.offset`.
// `soundN.volumescale` and `soundN.pan` are parsed here and converted to the
// mixer's units at the play site (`Mixer::volumeScaleToGain` /
// `storyboardPanToMixer`), then played through `Mixer::playFromFull` so they
// apply to this bank only. The docs allow no amplification, so a `volumescale`
// above 100 is clamped and reported once.
#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "core/ini.h"
#include "engine/anim.h"
#include "engine/defs.h"
#include "gfx/renderer.h"

namespace af {

// One `layerN.*` element. A layer is an animation, text, or both; MUGEN's
// sample storyboards use one or the other, and drawing both is the least
// surprising reading of a file that names both.
struct StoryboardLayer {
    bool present = false;

    int anim = -1;  // action number, -1 when the layer has no animation
    bool hasText = false;
    std::string text;  // already unquoted; `\n` decoded to a line break

    // `layerN.font = font_no, bank, align, r, g, b`. `font` indexes
    // `[SceneDef] font0..font9` directly (0..9), -1 when the key was omitted.
    // `align` keeps the storyboard documentation's convention: 1 left,
    // 0 centre, -1 right. (This is the opposite sense from
    // `textAlignFromInt`, which follows the screenpack element convention; see
    // the note in storyboard_screen.cpp.)
    int font = -1;
    int bank = 0;
    int align = 0;
    Color color{1.0f, 1.0f, 1.0f, 1.0f};

    Vec2 offset;
    // `layerN.scale` is not in the reference's storyboard table, but older
    // storyboards in the wild write it and the draw is one multiply. Accepting
    // it is an extension, not an interpretation of a documented key.
    Vec2 scale{1.0f, 1.0f};

    int startTime = 0;
    // -1 means "until the scene ends"; `endtime` is scene-local and exclusive,
    // like `end.time`.
    int endTime = -1;
    // Ticks per revealed character; 0 shows the whole string at once.
    int textDelay = 0;
};

// One `soundN.*` element. `value = group, sample`, played `starttime` ticks
// into the scene. `volumeScale` is MUGEN's percentage (100 = as authored) and
// `pan` is the docs' -127..127 (0 centre); both are stored raw and converted at
// the play site, where the mixer's own units are known.
struct StoryboardSound {
    bool present = false;
    int group = 0;
    int sample = 0;
    int startTime = 0;
    float volumeScale = 100.0f;
    int pan = 0;
};

struct StoryboardScene {
    std::string id;  // the suffix of `[Scene <id>]`, lower-cased by IniFile
    int endTime = 0;

    int fadeInTime = 0;
    int fadeOutTime = 0;
    // The reference only guarantees black or white, and treats an omitted
    // colour as black. Any RGB is accepted; the screen draws what was written.
    Color fadeInColor{0.0f, 0.0f, 0.0f, 1.0f};
    Color fadeOutColor{0.0f, 0.0f, 0.0f, 1.0f};

    // `clearcolor = r,g,b`, with r = -1 meaning "do not clear". The first scene
    // defaults to no clear, later scenes inherit the previous scene's value.
    bool clearScreen = false;
    Color clearColor{0.0f, 0.0f, 0.0f, 1.0f};

    Vec2 layerAllPos;

    // Resolved across scenes: an empty string after a scene that named one
    // means that scene stopped the music.
    std::string bgm;
    bool bgmLoop = false;

    // `bg.name`, recognised only so the screen can report that background
    // objects are unsupported rather than silently dropping a documented layer.
    std::string bgName;

    std::array<StoryboardLayer, 100> layers{};
    std::array<StoryboardSound, 100> sounds{};
};

struct StoryboardDef {
    Vec2 localCoord{320.0f, 240.0f};  // [Info] localcoord, default 320x240

    // [SceneDef]
    std::string spriteFile;
    std::string soundFile;
    std::array<std::string, 10> fontFiles;  // font0..font9
    int startScene = 0;
    std::vector<StoryboardScene> scenes;

    AnimationSet animations;  // inline [Begin Action N] blocks

    [[nodiscard]] bool empty() const { return scenes.empty(); }
};

// Parses a storyboard document. Unknown keys are ignored, as MUGEN ignores
// them; a malformed value leaves the field at its default rather than
// rejecting the scene, because a storyboard is presentation, not game state.
[[nodiscard]] StoryboardDef parseStoryboard(const IniFile& document);

// The per-scene clock, with no drawing in it.
//
// Boundaries are the running sum of the scenes' `end.time` values, so scene k
// occupies [start(k), start(k) + end(k)). The final tick of a scene is the tick
// before the next scene begins; `end.time` itself already belongs to the next
// scene. Negative durations collapse to zero, as `WinScreenTiming` treats them.
//
// Fades are two independent linear ramps measured from the scene's ends:
//
//     fade-in    alpha 1 at scene start, 0 at `fadein.time`
//     fade-out   alpha 0 at `end - fadeout.time`, 1 at `end`
//
// At any tick the overlay takes the larger of the two, and reports whether that
// larger value came from the fade-out (whose colour is used). Before the
// fade-out begins it is exactly 0, so it never wins against a fade-in.
class StoryboardTiming {
public:
    struct Scene {
        int endTime = 0;
        int fadeInTime = 0;
        int fadeOutTime = 0;
    };

    struct Fade {
        // 0 = fully transparent, 1 = fully covered.
        float alpha = 0.0f;
        // True when `alpha` comes from the fade-out ramp, which uses
        // `fadeout.col` rather than `fadein.col`.
        bool useFadeOutColor = false;
    };

    StoryboardTiming() = default;
    explicit StoryboardTiming(const std::vector<Scene>& scenes);

    [[nodiscard]] static StoryboardTiming fromStoryboard(const StoryboardDef& def);

    void advance() { ++tick_; }
    // Starts the clock at the first tick of `scene`. Used for
    // `[SceneDef] startscene`, which the reference documents as "start here
    // instead of scene 0", for inspecting a later scene.
    void startAtScene(size_t scene);

    [[nodiscard]] long long tick() const { return tick_; }
    // Sum of every scene's `end.time`; the tick the whole storyboard pops on.
    [[nodiscard]] long long totalTicks() const { return total_; }
    [[nodiscard]] size_t sceneCount() const { return starts_.size(); }

    // The scene containing `tick`, clamped to the first/last scene. Always a
    // valid index when the storyboard is non-empty.
    [[nodiscard]] size_t sceneAt(long long tick) const;
    [[nodiscard]] long long sceneStart(size_t scene) const;
    [[nodiscard]] long long sceneLength(size_t scene) const;
    // `tick - sceneStart(sceneAt(tick))`, clamped at zero.
    [[nodiscard]] long long sceneLocalTick(long long tick) const;

    [[nodiscard]] bool finished() const { return tick_ >= total_; }
    [[nodiscard]] Fade fadeAt(long long tick) const;

private:
    std::vector<long long> starts_;
    std::vector<long long> lengths_;
    std::vector<long long> fadeIns_;
    std::vector<long long> fadeOuts_;
    long long total_ = 0;
    long long tick_ = 0;
};

}  // namespace af
