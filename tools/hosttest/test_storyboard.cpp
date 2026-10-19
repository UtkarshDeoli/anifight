// Checks the storyboard format and clock: parsing, layer/sound keys, the
// inheritance rules, and the scene/fade arithmetic.
//
// The drawing half is deliberately not host-tested, as with the victory, Win
// and Continue cards: it needs a renderer and a GL context. What is worth
// pinning is the part a device test cannot show -- that `end.time` accumulates,
// that a scene owns exactly the ticks before the next scene starts, that an
// omitted `layerall.pos`/`clearcolor` is inherited, and that the two fade ramps
// meet at the right tick with the right colour.
//
// The shipped screenpack's gameover.def is parsed too when its directory is
// passed, so the file the app actually plays is covered rather than only a
// synthetic one.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_storyboard \
//         tools/hosttest/test_storyboard.cpp \
//         app/src/main/cpp/engine/storyboard.cpp \
//         app/src/main/cpp/engine/anim.cpp \
//         app/src/main/cpp/core/ini.cpp
//     /tmp/test_storyboard tools/screenpack/data

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "core/ini.h"
#include "engine/storyboard.h"

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

void checkNear(float actual, float expected, const char* what) {
    if (std::fabs(actual - expected) < 1e-5f) return;
    std::fprintf(stderr, "FAIL: %s (expected %.4f, got %.4f)\n", what, expected, actual);
    ++failures;
}

void checkColor(const af::Color& color, float r, float g, float b, const char* what) {
    if (std::fabs(color.r - r) < 1e-4f && std::fabs(color.g - g) < 1e-4f &&
        std::fabs(color.b - b) < 1e-4f) {
        return;
    }
    std::fprintf(stderr, "FAIL: %s (got %.3f, %.3f, %.3f)\n", what, color.r, color.g, color.b);
    ++failures;
}

// The synthetic document exercises every key the player reads, including the
// two inheritance cases and a second scene that deliberately omits them.
constexpr const char* kDocument = R"(
[Info]
localcoord = 640, 480

[SceneDef]
spr = "art/my.sff"
snd = sounds.snd
font0 = big.fnt
font3 = small.fnt
startscene = 1

[Scene intro]
fadein.time = 10
fadein.col = 255,255,255
fadeout.time = 20
fadeout.col = 0,255,0
clearcolor = 10,20,30
layerall.pos = 100,50
layer0.anim = 2
layer0.starttime = 4
layer1.text = "HELLO\nWORLD"
layer1.font = 3,2,1, 10,20,30
layer1.offset = -4,6
layer1.scale = 2,3
layer1.starttime = 5
layer1.endtime = 25
layer1.textdelay = 2
sound0.value = 5,7
sound0.starttime = 8
sound0.volumescale = 50
sound0.pan = -1
bgm = theme.mp3
bgm.loop = 0
end.time = 40

[Scene outro]
end.time = 30
fadein.time = 5
layer0.text = "BYE"
layer0.font = 0
sound0.value = 6,0
sound1.value = 6,1
sound1.starttime = 3

[Begin Action 2]
9,1, 10,-20, 3
9,2, 10,-20, 4,, A
)";

void testParsing() {
    const af::StoryboardDef def = af::parseStoryboard(af::IniFile::parse(kDocument));

    checkNear(def.localCoord.x, 640.0f, "localcoord width");
    checkNear(def.localCoord.y, 480.0f, "localcoord height");
    check(def.spriteFile == "art/my.sff", "spr is unquoted");
    check(def.soundFile == "sounds.snd", "snd is read");
    check(def.fontFiles[0] == "big.fnt", "font0 is read");
    check(def.fontFiles[3] == "small.fnt", "font3 is read");
    check(def.fontFiles[1].empty(), "an omitted font slot stays empty");
    check(def.startScene == 1, "startscene is read");

    check(def.scenes.size() == 2, "one scene per [Scene ?] section");
    if (def.scenes.size() != 2) return;

    const af::StoryboardScene& intro = def.scenes[0];
    check(intro.id == "intro", "the scene id is the section suffix");
    check(intro.endTime == 40, "end.time is read");
    check(intro.fadeInTime == 10, "fadein.time is read");
    check(intro.fadeOutTime == 20, "fadeout.time is read");
    checkColor(intro.fadeInColor, 1.0f, 1.0f, 1.0f, "fadein.col is read");
    checkColor(intro.fadeOutColor, 0.0f, 1.0f, 0.0f, "fadeout.col is read");
    check(intro.clearScreen, "a real clearcolor clears");
    checkColor(intro.clearColor, 10.0f / 255.0f, 20.0f / 255.0f, 30.0f / 255.0f,
               "clearcolor is read");
    checkNear(intro.layerAllPos.x, 100.0f, "layerall.pos x");
    checkNear(intro.layerAllPos.y, 50.0f, "layerall.pos y");

    const af::StoryboardLayer& animLayer = intro.layers[0];
    check(animLayer.present, "layer0 is present");
    check(animLayer.anim == 2, "layer0.anim is read");
    check(animLayer.startTime == 4, "layer0.starttime is read");

    const af::StoryboardLayer& textLayer = intro.layers[1];
    check(textLayer.present && textLayer.hasText, "layer1 is text");
    check(textLayer.text == "HELLO\nWORLD", "quotes come off and \\n becomes a line break");
    check(textLayer.font == 3, "layer font number is read");
    check(textLayer.bank == 2, "layer font bank is read");
    check(textLayer.align == 1, "layer font alignment is read");
    checkColor(textLayer.color, 10.0f / 255.0f, 20.0f / 255.0f, 30.0f / 255.0f,
               "layer font colour is read");
    checkNear(textLayer.offset.x, -4.0f, "layer offset x");
    checkNear(textLayer.offset.y, 6.0f, "layer offset y");
    checkNear(textLayer.scale.x, 2.0f, "layer scale x");
    checkNear(textLayer.scale.y, 3.0f, "layer scale y");
    check(textLayer.startTime == 5, "layer text starttime");
    check(textLayer.endTime == 25, "layer endtime");
    check(textLayer.textDelay == 2, "layer textdelay");

    const af::StoryboardSound& sound = intro.sounds[0];
    check(sound.present, "sound0 is present");
    check(sound.group == 5 && sound.sample == 7, "sound0.value is read as group, sample");
    check(sound.startTime == 8, "sound0.starttime");
    checkNear(sound.volumeScale, 50.0f, "sound0.volumescale is a percentage");
    check(sound.pan == -1, "sound0.pan");

    check(intro.bgm == "theme.mp3", "bgm is read");
    check(!intro.bgmLoop, "bgm.loop = 0 is read");

    // Scene 2 inherits `layerall.pos`, `clearcolor` and `bgm`; its own fade
    // colours and fade-out time keep their defaults.
    const af::StoryboardScene& outro = def.scenes[1];
    check(outro.endTime == 30, "the second scene's end.time is independent");
    checkNear(outro.layerAllPos.x, 100.0f, "layerall.pos is inherited");
    checkNear(outro.layerAllPos.y, 50.0f, "layerall.pos is inherited on both axes");
    check(outro.clearScreen, "clearcolor is inherited");
    checkColor(outro.clearColor, 10.0f / 255.0f, 20.0f / 255.0f, 30.0f / 255.0f,
               "the inherited clear colour is the same colour");
    check(outro.bgm == "theme.mp3", "bgm keeps playing when a scene omits it");
    check(outro.fadeInTime == 5, "the second scene's fadein.time is independent");
    check(outro.fadeOutTime == 0, "an omitted fadeout.time is zero");
    check(!outro.layers[1].present, "an unused layer is not present");
    check(outro.layers[0].hasText && outro.layers[0].text == "BYE", "the second scene's text");
    check(outro.layers[0].font == 0, "font 0 is a valid storyboard font slot");
    check(outro.sounds[0].present && outro.sounds[0].group == 6, "sound0 is redefined");
    check(outro.sounds[1].present && outro.sounds[1].startTime == 3, "sound1 is independent");

    // Inline actions go through the same AIR parser the screenpack uses.
    const af::Animation* animation = def.animations.find(2);
    check(animation != nullptr, "the inline [Begin Action 2] is parsed");
    if (animation != nullptr) {
        check(animation->frames.size() == 2, "the action has both frames");
        check(animation->frameAt(0) == 0, "the first frame lasts three ticks");
        check(animation->frameAt(3) == 1, "the second frame starts on tick 3");
        check(animation->frameDrawn(0).group == 9 && animation->frameDrawn(0).image == 1,
              "frame coordinates are read");
    }
}

void testTiming() {
    using af::StoryboardTiming;
    const std::vector<StoryboardTiming::Scene> scenes = {
        {40, 10, 20},
        {30, 5, 0},
    };
    StoryboardTiming timing(scenes);

    check(timing.totalTicks() == 70, "the storyboard lasts the sum of end.time");
    check(timing.sceneCount() == 2, "both scenes are in the clock");
    check(timing.sceneStart(0) == 0, "scene 0 starts at tick 0");
    check(timing.sceneStart(1) == 40, "scene 1 starts after scene 0's end.time");
    check(timing.sceneLength(0) == 40, "scene 0's length is end.time");
    check(timing.sceneLength(1) == 30, "scene 1's length is end.time");

    // `end.time` is exclusive: tick 40 already belongs to scene 1.
    check(timing.sceneAt(0) == 0, "tick 0 is in scene 0");
    check(timing.sceneAt(39) == 0, "the tick before end.time is in scene 0");
    check(timing.sceneAt(40) == 1, "the end.time tick is the next scene");
    check(timing.sceneAt(69) == 1, "the last visible tick is in scene 1");
    check(timing.sceneAt(70) == 1, "ticks past the end clamp to the last scene");
    check(timing.sceneAt(-5) == 0, "negative ticks clamp to the first scene");

    check(timing.sceneLocalTick(0) == 0, "scene 0 starts local at 0");
    check(timing.sceneLocalTick(39) == 39, "scene-local ticks count from the boundary");
    check(timing.sceneLocalTick(45) == 5, "scene 1's local tick is relative to its start");

    check(!timing.finished(), "a fresh clock is not finished");
    for (int i = 0; i < 70; ++i) timing.advance();
    check(timing.tick() == 70, "the clock reaches the total");
    check(timing.finished(), "the storyboard finishes on its total");
    timing.advance();
    check(timing.finished(), "it stays finished if the pop is delayed");

    // `startscene` starts the clock at that scene's first tick.
    StoryboardTiming fromSecond(scenes);
    fromSecond.startAtScene(1);
    check(fromSecond.tick() == 40, "startAtScene seeks to the scene's start");
    check(fromSecond.sceneAt(fromSecond.tick()) == 1, "the seeked scene is current");

    // Fade-in: opaque at local 0, linear to clear on fadein.time.
    StoryboardTiming::Fade fade = timing.fadeAt(0);
    checkNear(fade.alpha, 1.0f, "the fade-in starts fully covered");
    check(!fade.useFadeOutColor, "the fade-in uses the fade-in colour");
    checkNear(timing.fadeAt(5).alpha, 0.5f, "the fade-in is linear");
    checkNear(timing.fadeAt(9).alpha, 0.1f, "the fade-in reaches one step before clear");
    checkNear(timing.fadeAt(10).alpha, 0.0f, "the fade-in is clear on fadein.time");
    checkNear(timing.fadeAt(19).alpha, 0.0f, "the held scene has no overlay");

    // Fade-out: starts at end - fadeout.time (40 - 20 = 20) and darkens.
    fade = timing.fadeAt(20);
    checkNear(fade.alpha, 0.0f, "the fade-out starts fully clear");
    check(!fade.useFadeOutColor, "a zero fade-out alpha does not claim its colour");
    fade = timing.fadeAt(30);
    checkNear(fade.alpha, 0.5f, "the fade-out is linear");
    check(fade.useFadeOutColor, "the fade-out uses the fade-out colour");
    fade = timing.fadeAt(39);
    checkNear(fade.alpha, 0.95f, "the last visible tick is almost covered");
    check(fade.useFadeOutColor, "the last visible tick uses the fade-out colour");

    // The second scene's fade-in overlays the first scene's held black.
    fade = timing.fadeAt(40);
    checkNear(fade.alpha, 1.0f, "scene 1 fades in from its own start");
    check(!fade.useFadeOutColor, "scene 1's fade-in uses its own colour");
    checkNear(timing.fadeAt(44).alpha, 0.2f, "scene 1's fade-in is linear");
    checkNear(timing.fadeAt(60).alpha, 0.0f, "scene 1 has no fade-out");

    // The fade ramp is clamped when the fade is longer than its scene: the
    // ramp starts at local 0 instead of leaving the screen clear.
    const std::vector<StoryboardTiming::Scene> longFade = {{10, 0, 20}};
    StoryboardTiming longFadeTiming(longFade);
    checkNear(longFadeTiming.fadeAt(0).alpha, 0.0f, "a clamped fade-out starts clear");
    checkNear(longFadeTiming.fadeAt(9).alpha, 9.0f / 20.0f,
              "a fade-out longer than its scene still ramps");

    // Negative and zero durations collapse rather than making a broken clock.
    const std::vector<StoryboardTiming::Scene> negative = {{-5, -10, -2}, {10, 0, 0}};
    StoryboardTiming negativeTiming(negative);
    check(negativeTiming.totalTicks() == 10, "negative end.time contributes zero ticks");
    check(negativeTiming.sceneAt(0) == 1, "a zero-length scene is skipped");
    checkNear(negativeTiming.fadeAt(0).alpha, 0.0f, "negative fades are no overlay");
    check(!negativeTiming.finished(), "the non-empty scene still plays");

    StoryboardTiming empty(std::vector<StoryboardTiming::Scene>{});
    check(empty.totalTicks() == 0, "an empty storyboard lasts zero ticks");
    check(empty.finished(), "an empty storyboard is finished immediately");
    check(empty.sceneAt(0) == 0, "an empty storyboard reports a safe scene index");
    checkNear(empty.fadeAt(0).alpha, 0.0f, "an empty storyboard has no overlay");
}

void testShippedStoryboard(const std::string& dataDir) {
    const std::string path = dataDir + "/gameover.def";
    std::ifstream file(path);
    if (!file) {
        std::printf("SKIP: %s not found\n", path.c_str());
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();

    const af::StoryboardDef def = af::parseStoryboard(af::IniFile::parse(buffer.str()));
    check(!def.empty(), "the shipped gameover.def has scenes");
    if (def.empty()) return;

    check(def.spriteFile == "system.sff", "the shipped storyboard uses system.sff");
    check(def.fontFiles[0] == "f-12x18.fnt", "the shipped storyboard names a font");
    check(def.scenes.size() == 1, "the shipped gameover has one scene");
    const af::StoryboardScene& scene = def.scenes[0];
    check(scene.endTime == 220, "the shipped gameover runs 220 ticks");
    check(scene.fadeInTime == 30 && scene.fadeOutTime == 30, "the shipped fades are symmetric");
    check(scene.clearScreen, "the shipped scene clears to its own colour");
    check(scene.layers[0].present && scene.layers[0].anim == 0, "the wordmark layer is action 0");
    check(scene.layers[1].hasText && scene.layers[1].text == "GAME OVER", "the first line");
    check(scene.layers[2].hasText && scene.layers[2].text == "TRY AGAIN", "the second line");
    check(scene.layers[2].textDelay == 3, "the second line types out");

    const af::Animation* animation = def.animations.find(0);
    check(animation != nullptr && animation->frames.size() == 4,
          "the shipped wordmark action has four frames");
    if (animation != nullptr && !animation->frames.empty()) {
        check(animation->frames.back().duration == -1, "the wordmark settles forever");
        check(animation->frames.back().group == 10 && animation->frames.back().image == 0,
              "the wordmark is system.sff group 10 image 0");
    }

    // The clock the screen builds from that file.
    const af::StoryboardTiming timing = af::StoryboardTiming::fromStoryboard(def);
    check(timing.totalTicks() == 220, "the shipped storyboard's clock totals 220");
    checkNear(timing.fadeAt(0).alpha, 1.0f, "the shipped storyboard starts black");
    checkNear(timing.fadeAt(30).alpha, 0.0f, "the shipped fade-in ends on tick 30");
    checkNear(timing.fadeAt(190).alpha, 0.0f, "the shipped fade-out starts on tick 190");
    const af::StoryboardTiming::Fade fade = timing.fadeAt(219);
    check(fade.useFadeOutColor, "the shipped storyboard ends on its fade-out colour");
    check(fade.alpha > 0.9f, "the shipped storyboard is almost covered at the end");
}

}  // namespace

int main(int argc, char** argv) {
    testParsing();
    testTiming();
    if (argc > 1) {
        testShippedStoryboard(argv[1]);
    } else {
        std::printf("SKIP: no screenpack directory argument\n");
    }

    if (failures == 0) {
        std::printf("all storyboard tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "%d storyboard test(s) failed\n", failures);
    return 1;
}
