// Checks the stage-owned shadow model: the `[Shadow]` and `[Reflection]`
// sections of a stage .def, the `[Config] drawshadows` switch, and the alpha
// rule shared by the player and object draw paths.
//
// The alpha rule is pure arithmetic (`engine/shadow.h`), so it is checked here
// at its boundaries rather than only through a rendered frame. The stage
// parsing is checked against synthetic .def text, which keeps this test free of
// any fixture.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//         -o /tmp/test_stage_shadow tools/hosttest/test_stage_shadow.cpp \
//         tools/hosttest/shim/sprite_bank_stub.cpp \
//         app/src/main/cpp/engine/stage.cpp \
//         app/src/main/cpp/engine/defs.cpp \
//         app/src/main/cpp/engine/anim.cpp \
//         app/src/main/cpp/core/ini.cpp
//     /tmp/test_stage_shadow

#include <cmath>
#include <cstdio>
#include <string>

#include "core/ini.h"
#include "engine/defs.h"
#include "engine/shadow.h"
#include "engine/stage.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++failures;
}

void checkNear(float actual, float expected, const std::string& what) {
    if (std::fabs(actual - expected) <= 1e-4f) return;
    std::fprintf(stderr, "FAIL: %s (got %.6f, expected %.6f)\n", what.c_str(),
                 static_cast<double>(actual), static_cast<double>(expected));
    ++failures;
}

af::StageDef parseStage(const std::string& text) {
    return af::parseStageDef(af::IniFile::parse(text));
}

}  // namespace

int main() {
    // ------------------------------------------------------- stage defaults
    {
        const af::StageDef def = parseStage("");
        const af::ShadowStyle& shadow = def.shadow;
        checkNear(shadow.color.r, 0.0f, "[Shadow] color default r is 0");
        checkNear(shadow.color.g, 0.0f, "[Shadow] color default g is 0");
        checkNear(shadow.color.b, 0.0f, "[Shadow] color default b is 0");
        checkNear(shadow.intensity, 128.0f, "[Shadow] intensity default 128");
        checkNear(shadow.yScale, 0.4f, "[Shadow] yscale default 0.4");
        checkNear(shadow.fadeTop, 0.0f, "[Shadow] fade.range top defaults to 0");
        checkNear(shadow.fadeBottom, 0.0f, "[Shadow] fade.range bottom defaults to 0");
        check(!shadow.fadeActive(), "an omitted fade.range is inactive");
        checkNear(shadow.reflectionIntensity, 0.0f,
                  "[Reflection] intensity defaults to 0 (off)");
    }

    // ------------------------------------------------------- explicit values
    {
        const af::StageDef def = parseStage(
            "[Shadow]\n"
            "color = 40, 80, 120\n"
            "intensity = 200\n"
            "yscale = -0.5\n"
            "fade.range = -100, 0\n"
            "\n"
            "[Reflection]\n"
            "intensity = 77\n");
        const af::ShadowStyle& shadow = def.shadow;
        checkNear(shadow.color.r, 40.0f / 255.0f, "[Shadow] color r is parsed 0..255");
        checkNear(shadow.color.g, 80.0f / 255.0f, "[Shadow] color g is parsed 0..255");
        checkNear(shadow.color.b, 120.0f / 255.0f, "[Shadow] color b is parsed 0..255");
        checkNear(shadow.intensity, 200.0f, "[Shadow] intensity is parsed");
        checkNear(shadow.yScale, -0.5f, "[Shadow] a negative yscale is kept");
        checkNear(shadow.fadeTop, -100.0f, "[Shadow] fade.range top is parsed");
        checkNear(shadow.fadeBottom, 0.0f, "[Shadow] fade.range bottom is parsed");
        check(shadow.fadeActive(), "top < bottom makes the fade active");
        checkNear(shadow.reflectionIntensity, 77.0f, "[Reflection] intensity is parsed");
    }

    // A reversed or equal range is inactive, which is the reference's own gate
    // (`top < bottom`).
    {
        const af::StageDef def = parseStage("[Shadow]\nfade.range = 0, -100\n");
        check(!def.shadow.fadeActive(), "a reversed fade.range is inactive");
    }

    // ------------------------------------------------ stage coordinate space
    //
    // A stage's numbers are written in its own `localcoord` space. The fight
    // window is always 240 units high, so the translation is the height ratio:
    // a 1280x720 stage becomes a third as large, a 640x480 one half, and a
    // 320x240 one is unchanged. `xscale`/`yscale` multiply on top.
    {
        const af::StageDef defaults = parseStage("");
        checkNear(defaults.localCoord.x, 320.0f, "localcoord defaults to 320 wide");
        checkNear(defaults.localCoord.y, 240.0f, "localcoord defaults to 240 high");
        const af::StageTranslation one = af::stageTranslation(defaults, 240.0f);
        checkNear(one.x, 1.0f, "a 320x240 stage is drawn 1:1");
        checkNear(one.y, 1.0f, "a 320x240 stage is drawn 1:1 vertically");

        const af::StageDef hd = parseStage(
            "[StageInfo]\n"
            "localcoord = 1280, 720\n"
            "zoffset = 670\n");
        checkNear(hd.localCoord.x, 1280.0f, "localcoord width is parsed");
        checkNear(hd.localCoord.y, 720.0f, "localcoord height is parsed");
        const af::StageTranslation third = af::stageTranslation(hd, 240.0f);
        checkNear(third.x, 1.0f / 3.0f, "1280x720 translates by the height ratio");
        checkNear(third.y, 1.0f / 3.0f, "1280x720 translates by the height ratio");
        checkNear(af::stageTranslation(hd, 240.0f).apply(af::Vec2{0.0f, 670.0f}).y,
                  670.0f / 3.0f, "the roof's floor lands at 223 of 240");
        checkNear(third.apply(af::Vec2{-280.0f, 0.0f}).x, -280.0f / 3.0f,
                  "the roof's player starts translate with the stage");

        const af::StageTranslation half =
            af::stageTranslation(parseStage("[StageInfo]\nlocalcoord = 640, 480\n"), 240.0f);
        checkNear(half.x, 0.5f, "640x480 translates by half");

        const af::StageDef scaled = parseStage(
            "[StageInfo]\n"
            "localcoord = 1280, 720\n"
            "xscale = 2\n"
            "yscale = 0.5\n");
        checkNear(scaled.scaleX, 2.0f, "[StageInfo] xscale is parsed");
        checkNear(scaled.scaleY, 0.5f, "[StageInfo] yscale is parsed");
        const af::StageTranslation withScale = af::stageTranslation(scaled, 240.0f);
        checkNear(withScale.x, 2.0f / 3.0f, "xscale multiplies the translation");
        checkNear(withScale.y, 1.0f / 6.0f, "yscale multiplies the translation");

        // A malformed coordinate space falls back rather than making the
        // translation infinite; the parser replaces non-positive sizes.
        const af::StageDef broken = parseStage("[StageInfo]\nlocalcoord = 0, 0\n");
        const af::StageTranslation fallback = af::stageTranslation(broken, 240.0f);
        checkNear(fallback.x, 1.0f, "a zero localcoord falls back to 320x240");
        checkNear(fallback.y, 1.0f, "a zero localcoord falls back to 320x240");
    }

    // --------------------------------------------------- sprite file resolving
    //
    // `spr` is resolved from the data root, not from the .def's own folder, so
    // a stage in `stages/` legally writes either `x.sff` or `stages/x.sff`.
    // Prepending the folder to the second spelling is a silent black stage --
    // it asks for `stages/stages/x.sff` -- and three corpus stages use it.
    {
        check(af::resolveStageFile("stages/", "roof.sff") == "stages/roof.sff",
              "a bare spr is resolved inside the stage folder");
        check(af::resolveStageFile("stages/", "stages/roof.sff") == "stages/roof.sff",
              "a fully-qualified spr is not given the folder twice");
        check(af::resolveStageFile("stages/", "/data/stages/roof.sff") ==
                  "/data/stages/roof.sff",
              "an absolute spr is taken as written");
        check(af::resolveStageFile("", "roof.sff") == "roof.sff",
              "a .def at the root resolves against the root");
        check(af::resolveStageFile("stages/", "stages2/roof.sff") == "stages/stages2/roof.sff",
              "the folder comparison includes its separator, not a bare prefix");
        check(af::resolveStageFile("stages/", "") == std::string(),
              "an empty spr resolves to nothing");
    }

    // ------------------------------------------------------------ stage list
    // `[Info] displayname` is the name MUGEN documents for the stage select
    // list ("the name to display in the stage select list"). It defaults to
    // `name`, so the select screen only ever has to read this one field.
    {
        const af::StageDef named = parseStage(
            "[Info]\n"
            "name = \"Training Ground\"\n"
            "displayname = \"Training\"\n");
        check(named.name == "Training Ground", "[Info] name is parsed");
        check(named.displayName == "Training",
              "[Info] displayname is parsed and kept separate from name");

        const af::StageDef fallback = parseStage("[Info]\nname = \"Rooftop\"\n");
        check(fallback.displayName == "Rooftop",
              "an omitted displayname falls back to name");

        const af::StageDef empty = parseStage("");
        check(empty.displayName.empty(), "a stage with no [Info] has no display name");
    }

    // ---------------------------------------------------- drawshadows switch
    {
        const af::Config defaults = af::parseConfig(af::IniFile::parse(""));
        check(defaults.drawShadows, "[Config] drawshadows defaults on");

        const af::Config off = af::parseConfig(af::IniFile::parse(
            "[Config]\ndrawshadows = 0\n"));
        check(!off.drawShadows, "[Config] drawshadows = 0 disables the passes");

        const af::Config on = af::parseConfig(af::IniFile::parse(
            "[Config]\ndrawshadows = 1\n"));
        check(on.drawShadows, "[Config] drawshadows = 1 keeps the passes on");
    }

    // ----------------------------------------------------------- alpha rule
    // 0..256 intensity, 256 = darkest, and the result is a 0..1 alpha.
    {
        using af::shadowAlpha;
        // No fade: the alpha is intensity / 256.
        checkNear(shadowAlpha(256.0f, 0.0f, 0.0f, 0.0f), 1.0f,
                  "no fade, intensity 256 is fully opaque");
        checkNear(shadowAlpha(128.0f, 0.0f, 0.0f, -50.0f), 0.5f,
                  "no fade, intensity 128 is 0.5");
        checkNear(shadowAlpha(0.0f, 0.0f, 0.0f, -50.0f), 0.0f,
                  "intensity 0 draws nothing");
        checkNear(shadowAlpha(512.0f, 0.0f, 0.0f, -50.0f), 1.0f,
                  "intensity above 256 clamps to opaque");
        checkNear(shadowAlpha(-40.0f, 0.0f, 0.0f, -50.0f), 0.0f,
                  "a negative intensity clamps to nothing");

        // Fade: at/above the top is zero, at/below the bottom is full, linear
        // in between. With top = 0 and bottom = 100, depth 50 is halfway.
        checkNear(shadowAlpha(256.0f, 0.0f, 100.0f, 0.0f), 0.0f,
                  "fade: at the top the shadow is gone");
        checkNear(shadowAlpha(256.0f, 0.0f, 100.0f, -20.0f), 0.0f,
                  "fade: above the top the shadow is gone");
        checkNear(shadowAlpha(256.0f, 0.0f, 100.0f, 150.0f), 1.0f,
                  "fade: below the bottom the shadow is full");
        checkNear(shadowAlpha(256.0f, 0.0f, 100.0f, 50.0f), 0.5f,
                  "fade: the midpoint is halfway");

        // addalpha: the base is 256 - dst before the fade ramp.
        checkNear(shadowAlpha(256.0f, 0.0f, 0.0f, 0.0f, 64.0f), 0.75f,
                  "addalpha dst 64 leaves a 192/256 base");
        checkNear(shadowAlpha(256.0f, 0.0f, 0.0f, 0.0f, 256.0f), 0.0f,
                  "addalpha dst 256 removes the shadow");
        checkNear(shadowAlpha(256.0f, 0.0f, 100.0f, 50.0f, 0.0f), 0.5f,
                  "addalpha and fade compose multiplicatively");
        checkNear(shadowAlpha(128.0f, 0.0f, 100.0f, 50.0f, 128.0f), 0.125f,
                  "addalpha, fade and intensity all multiply");
    }

    // ------------------------------------------------------ reflection rule
    //
    // `[Reflection] intensity` is its own alpha on the same 0..256 scale, and
    // unlike the shadow it does not run through the fade or addalpha chain.
    {
        af::ShadowStyle style;  // defaults: reflectionIntensity = 0
        check(!af::reflectionActive(style), "reflection intensity 0 is off");
        checkNear(af::reflectionAlpha(style), 0.0f, "reflection alpha is 0 when off");

        style.reflectionIntensity = 128.0f;
        check(af::reflectionActive(style), "a positive reflection intensity is on");
        checkNear(af::reflectionAlpha(style), 0.5f, "reflection alpha is intensity / 256");

        style.reflectionIntensity = 256.0f;
        checkNear(af::reflectionAlpha(style), 1.0f, "reflection intensity 256 is opaque");

        style.reflectionIntensity = 512.0f;
        checkNear(af::reflectionAlpha(style), 1.0f,
                  "reflection intensity above 256 clamps to opaque");

        style.reflectionIntensity = -8.0f;
        check(!af::reflectionActive(style), "a negative reflection intensity is off");
        checkNear(af::reflectionAlpha(style), 0.0f,
                  "a negative reflection intensity clamps to nothing");
    }

    // The mirror is about the ground plane: a body axis `h` above the floor
    // reflects to `h` below it, and points on the floor stay put.
    {
        using af::reflectionY;
        checkNear(reflectionY(240.0f, 240.0f), 240.0f,
                  "a body axis on the floor reflects onto itself");
        checkNear(reflectionY(240.0f, 200.0f), 280.0f,
                  "an axis 40 above the floor reflects 40 below it");
        checkNear(reflectionY(240.0f, 260.0f), 220.0f,
                  "an axis 20 below the floor reflects 20 above it");
        checkNear(reflectionY(0.0f, -15.0f), 15.0f,
                  "the mirror is independent of the floor's screen position");
    }

    // --------------------------------------------------- object colour rules
    {
        const af::ShadowStyle stage;
        check(!af::objectShadowEnabled(af::ShadowRgb{0, 0, 0}),
              "an object shadow of 0,0,0 means none");
        check(af::objectShadowEnabled(af::ShadowRgb{0, 0, 1}),
              "any non-zero component asks for a shadow");

        const af::Color stageColor = af::resolveObjectShadowColor(
            af::ShadowRgb{-1, 0, 0}, stage, /*anyComponentSelectsStage=*/false);
        checkNear(stageColor.r, stage.color.r, "Explod r == -1 selects the stage colour");

        // Explod reads only r; -1 in g or b is an ordinary (clamped) value.
        const af::Color explodGreen = af::resolveObjectShadowColor(
            af::ShadowRgb{0, -1, 0}, stage, /*anyComponentSelectsStage=*/false);
        checkNear(explodGreen.g, 0.0f, "Explod g == -1 does not select the stage colour");

        // Projectile reads any component.
        const af::Color projectileGreen = af::resolveObjectShadowColor(
            af::ShadowRgb{0, -1, 0}, stage, /*anyComponentSelectsStage=*/true);
        checkNear(projectileGreen.r, stage.color.r,
                  "Projectile g == -1 selects the stage colour");
        checkNear(projectileGreen.g, stage.color.g,
                  "Projectile g == -1 selects the whole stage colour");
    }

    if (failures == 0) {
        std::printf("test_stage_shadow: all checks passed\n");
        return 0;
    }
    std::fprintf(stderr, "test_stage_shadow: %d failure(s)\n", failures);
    return 1;
}
