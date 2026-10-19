// Host test for the `[Reflection]` draw of fighters and helpers.
//
// A reflection is the fighter's *current animation frame*, mirrored about the
// ground plane and drawn at the stage's `[Reflection] intensity` as alpha
// (SHADOW-RE.md §0 fact 7, §3.2). There is no display here, so the shim's
// `drawSpriteAt` records the call instead: its sprite, axis point, facing,
// scale, tint, alpha and vertical mirror are exactly the facts the reference
// determines and the ones a wrong sign or a missing gate would change.
//
// The fixture is synthetic -- a one-frame action and a bare `[Statedef 0]` --
// so this runs without any character files:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//         -o /tmp/test_reflection_draw tools/hosttest/test_reflection_draw.cpp \
//         tools/hosttest/shim/sprite_bank_stub.cpp \
//         app/src/main/cpp/engine/statemachine.cpp \
//         app/src/main/cpp/engine/cns.cpp \
//         app/src/main/cpp/engine/cmd.cpp \
//         app/src/main/cpp/engine/expr.cpp \
//         app/src/main/cpp/engine/character.cpp \
//         app/src/main/cpp/engine/victory_flow.cpp \
//         app/src/main/cpp/engine/anim.cpp \
//         app/src/main/cpp/engine/fighter.cpp \
//         app/src/main/cpp/audio/snd_bank.cpp \
//         app/src/main/cpp/core/ini.cpp
//     /tmp/test_reflection_draw

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "draw_recorder.h"
#include "engine/anim.h"
#include "engine/character.h"
#include "engine/fighter.h"
#include "engine/shadow.h"
#include "gfx/renderer.h"

namespace {

constexpr float kFloorY = 240.0f;
constexpr float kOriginX = 0.0f;

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

bool isWhite(const af::Color& c) {
    return std::fabs(c.r - 1.0f) < 1e-4f && std::fabs(c.g - 1.0f) < 1e-4f &&
           std::fabs(c.b - 1.0f) < 1e-4f && std::fabs(c.a - 1.0f) < 1e-4f;
}

// A synthetic character whose action 0 is one frame, plus the fighter that
// draws it. `air` is the frame tuple, e.g. `7,3, 11,-20, -1`.
struct Rig {
    af::Character character;
    af::Fighter body;
    af::Renderer renderer;

    explicit Rig(const std::string& air) {
        character.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[Statedef -1]\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 0]\n" + air + "\n", character.animations());
        body.setup(&character, 320.0f, 1);
        body.setScale(1.0f);
    }
};

std::vector<af::draw_stub::DrawCall> draw(Rig& rig, af::ShadowStyle shadow,
                                          bool drawShadows = true) {
    af::draw_stub::clear();
    af::draw_stub::record(true);
    rig.body.draw(rig.renderer, kOriginX, kFloorY, shadow, drawShadows);
    af::draw_stub::record(false);
    return af::draw_stub::recorded();
}

af::ShadowStyle reflectionOnly(float intensity) {
    af::ShadowStyle shadow;
    shadow.intensity = 0.0f;  // isolate the reflection from the contact shadow
    shadow.reflectionIntensity = intensity;
    return shadow;
}

}  // namespace

int main() {
    // ------------------------------------------------------ geometry, alpha
    //
    // Frame `7,3, 11,-20`: the body draws at x = 320 + 11 = 331 and its axis
    // at y = 240 - 20 = 220. The reflection is that axis mirrored about the
    // floor, 240 + (240 - 220) = 260, with the same x and a vertical flip.
    // Intensity 160 of 256 is alpha 0.625.
    {
        Rig rig("7,3, 11,-20, -1");
        const std::vector<af::draw_stub::DrawCall> calls =
            draw(rig, reflectionOnly(160.0f));
        check(calls.size() == 2, "reflection and body are the two draws");
        if (calls.size() == 2) {
            const af::draw_stub::DrawCall& reflection = calls[0];
            const af::draw_stub::DrawCall& body = calls[1];

            check(reflection.group == 7 && reflection.image == 3,
                  "the reflection uses the current frame's sprite");
            checkNear(reflection.x, 331.0f, "the reflection shares the body's x");
            checkNear(reflection.y, 260.0f,
                      "the reflection's axis is the body's mirrored about the floor");
            check(reflection.facing == 1, "the reflection faces like the body");
            check(reflection.flipV, "the reflection is vertically mirrored");
            checkNear(reflection.scaleX, 1.0f, "the reflection has the body's x scale");
            checkNear(reflection.scaleY, 1.0f,
                      "the reflection is a mirror, not the shadow's yscale squash");
            check(isWhite(reflection.tint), "the reflection is untinted");
            check(reflection.trans.blend == af::Blend::Normal,
                  "the reflection blends normally");
            checkNear(reflection.trans.srcAlpha, 160.0f / 256.0f,
                      "reflection alpha is intensity / 256");
            checkNear(reflection.trans.dstAlpha, 1.0f,
                      "the reflection leaves the scene behind it intact");
            check(reflection.trans == af::TransParams{af::Blend::Normal, 0.625f, 1.0f},
                  "the whole TransParams pair is the reflection's");

            checkNear(body.x, 331.0f, "the body still draws where it did");
            checkNear(body.y, 220.0f, "the body's axis is unchanged");
            check(!body.flipV, "the body itself is not vertically flipped");
        }
    }

    // Intensity is an alpha on MUGEN's 0..256 scale and is clamped both ways.
    {
        Rig rig("7,3, 11,-20, -1");
        const std::vector<af::draw_stub::DrawCall> calls =
            draw(rig, reflectionOnly(512.0f));
        check(calls.size() == 2, "an over-range intensity still draws a reflection");
        if (calls.size() == 2) {
            checkNear(calls[0].trans.srcAlpha, 1.0f, "intensity above 256 clamps to opaque");
        }
    }

    // A horizontal flip is the body's, with the vertical mirror on top. The
    // axis x does not move: `drawSpriteAt` mirrors the art about the axis.
    {
        Rig rig("7,3, 11,-20, -1, H");
        const std::vector<af::draw_stub::DrawCall> calls =
            draw(rig, reflectionOnly(64.0f));
        check(calls.size() == 2, "a flipped frame still draws a reflection and a body");
        if (calls.size() == 2) {
            check(calls[0].facing == -1, "the reflection follows the frame's H flag");
            check(calls[1].facing == -1, "and the body does too");
            checkNear(calls[0].x, calls[1].x, "facing does not move either axis");
            check(calls[0].flipV, "the reflection is still vertically mirrored");
        }
    }

    // ------------------------------------------------------------ the gates
    //
    // Reflection intensity 0 is off; so are `[Config] drawshadows`, the
    // machine's `noshadow` assert, and invisibility (SHADOW-RE.md §3.2 and
    // task semantics). The body is drawn in every one of those cases.
    {
        Rig rig("7,3, 11,-20, -1");
        const std::vector<af::draw_stub::DrawCall> calls =
            draw(rig, reflectionOnly(0.0f));
        check(calls.size() == 1, "reflection intensity 0 draws only the body");
    }
    {
        Rig rig("7,3, 11,-20, -1");
        const std::vector<af::draw_stub::DrawCall> calls =
            draw(rig, reflectionOnly(128.0f), /*drawShadows=*/false);
        check(calls.size() == 1, "drawshadows = 0 draws only the body");
    }
    {
        Rig rig("7,3, 11,-20, -1");
        rig.body.machine().setAssertFlag(af::StateMachine::kAssertNoShadow);
        const std::vector<af::draw_stub::DrawCall> calls =
            draw(rig, reflectionOnly(128.0f));
        check(calls.size() == 1, "noshadow hides the reflection, not the body");
    }
    {
        Rig rig("7,3, 11,-20, -1");
        rig.body.machine().setInvisibleTicks(2);
        const std::vector<af::draw_stub::DrawCall> calls =
            draw(rig, reflectionOnly(128.0f));
        check(calls.size() == 1, "an invisible fighter draws no reflection");
    }

    // --------------------------------------------------- order with a shadow
    //
    // The reference draws the reflection from the same pass as the shadow,
    // before the body. With both turned on the recorder must see shadow,
    // reflection, body -- and the shadow's own geometry and alpha must not
    // have moved.
    {
        Rig rig("7,3, 11,-20, -1");
        af::ShadowStyle shadow;
        shadow.intensity = 128.0f;
        shadow.reflectionIntensity = 128.0f;
        const std::vector<af::draw_stub::DrawCall> calls = draw(rig, shadow);
        check(calls.size() == 3, "shadow, reflection and body are three draws");
        if (calls.size() == 3) {
            const af::draw_stub::DrawCall& contact = calls[0];
            const af::draw_stub::DrawCall& reflection = calls[1];
            check(contact.flipV == false && std::fabs(contact.scaleY - 0.4f) < 1e-4f,
                  "the shadow keeps its positive-yscale squash");
            checkNear(contact.y, 232.0f,
                      "the shadow keeps its ground projection (240 + -20 * 0.4)");
            checkNear(contact.trans.srcAlpha, 0.5f, "the shadow keeps its own alpha chain");
            check(reflection.flipV && std::fabs(reflection.scaleY - 1.0f) < 1e-4f,
                  "the reflection follows it, mirrored and unsquashed");
            checkNear(reflection.y, 260.0f, "the reflection's mirrored axis is unchanged");
            check(calls[2].flipV == false &&
                      calls[2].trans == af::TransParams{af::Blend::Normal, 1.0f, 1.0f},
                  "the body draws last and unchanged");
        }
    }

    if (failures == 0) {
        std::printf("test_reflection_draw: all checks passed\n");
        return 0;
    }
    std::fprintf(stderr, "test_reflection_draw: %d failure(s)\n", failures);
    return 1;
}
