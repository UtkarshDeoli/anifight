// Host test for the on-screen pad's layout and matching (engine/touch_layout).
//
// The claim being tested is the one the whole pad rests on: that where a control is
// and what a finger on it means are separate, testable things which do not need a
// device, a screen or a thumb. Everything here is arithmetic over numbers a saved
// settings file could contain.
//
// The behaviours pinned are the ones adopted from PPSSPP's Android pad, because they
// are what a player who has used that pad will expect: positions as fractions of the
// screen, a d-pad read from the *angle* of the thumb so that diagonals come out of a
// slide, a dead zone in the middle of the cross, a sticky d-pad, gliding, and the
// attack buttons moving and scaling as one cluster rather than six.
//
// Build and run:
//   clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_touch_layout \
//       tools/hosttest/test_touch_layout.cpp \
//       app/src/main/cpp/engine/touch_layout.cpp app/src/main/cpp/core/ini.cpp

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "core/ini.h"
#include "engine/input.h"
#include "engine/touch_layout.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    ++failures;
    std::printf("FAIL: %s\n", what.c_str());
}

constexpr float kWidth = 2340.0f;
constexpr float kHeight = 1080.0f;

af::touchpad::Contact finger(int id, float x, float y) {
    af::touchpad::Contact contact;
    contact.id = id;
    contact.x = x;
    contact.y = y;
    return contact;
}

// The centre of a control that has been resolved for the screen.
struct Point {
    float x;
    float y;
};

}  // namespace

int main() {
    using namespace af::touchpad;

    Layout layout;
    layout.resetToDefaults();
    check(layout.attacks.configured() && layout.directions.configured() &&
              layout.start.configured(),
          "the shipped layout configures all three controls");
    check(layout.attacks.x > 0.5f, "the attacks are on the right, where a thumb reaches");
    check(layout.directions.x < 0.5f, "and the directions on the left");

    const Resolved geometry = resolve(layout, kWidth, kHeight);

    // ------------------------------------------------------------ the arithmetic
    {
        // The six attacks are a 3x2 grid, evenly pitched, with the kicks below the
        // punches as they are on a cabinet.
        const float pitch = geometry.buttons[1].x - geometry.buttons[0].x;
        check(pitch > 0.0f, "the attack columns run left to right");
        for (int column = 0; column < 2; ++column) {
            check(std::fabs((geometry.buttons[column + 1].x - geometry.buttons[column].x) - pitch) <
                      0.5f,
                  "the top row is evenly spaced");
            check(std::fabs((geometry.buttons[column + 4].x - geometry.buttons[column + 3].x) -
                            pitch) < 0.5f,
                  "and so is the bottom row");
            check(std::fabs(geometry.buttons[column + 3].x - geometry.buttons[column].x) < 0.5f,
                  "and the bottom row lines up with it");
        }
        check(geometry.buttons[3].y > geometry.buttons[0].y, "the kicks are below the punches");
        check(geometry.buttons[0].w == geometry.buttons[5].w, "every attack button is one size");

        // The cluster is centred where the layout says it is.
        const float centreX = (geometry.buttons[0].x + geometry.buttons[2].x +
                               geometry.buttons[2].w) * 0.5f;
        check(std::fabs(centreX - layout.attacks.x * kWidth) < 0.5f,
              "the attack cluster is centred on its placement");

        // The d-pad's four arms sit on the compass points around its centre.
        check(std::fabs((geometry.buttons[6].x + geometry.buttons[6].w * 0.5f) -
                        geometry.dpadCentreX) < 0.5f,
              "the up arm is centred above the d-pad");
        check(geometry.buttons[6].y < geometry.dpadCentreY, "and above it");
        check(geometry.buttons[7].y > geometry.dpadCentreY, "the down arm is below it");
        check(geometry.buttons[8].x < geometry.dpadCentreX, "the left arm is left of it");
        check(geometry.buttons[9].x > geometry.dpadCentreX, "and the right arm right of it");

        // Start is a bar under the middle of the screen, out of both thumb clusters.
        const Resolved::Button& start = geometry.buttons[10];
        check(start.w > start.h * 2.0f, "start is a bar rather than a square");
        check(std::fabs((start.x + start.w * 0.5f) - kWidth * 0.5f) < 0.5f,
              "centred on the screen");
        check(start.y > geometry.buttons[3].y, "and below both clusters");

        // Scaling a control scales it, and the cluster grows around its centre.
        Layout bigger = layout;
        bigger.attacks.scale = 1.5f;
        const Resolved larger = resolve(bigger, kWidth, kHeight);
        check(larger.buttons[0].w > geometry.buttons[0].w, "a scale multiplier enlarges a button");
        const float largerCentreX =
            (larger.buttons[0].x + larger.buttons[2].x + larger.buttons[2].w) * 0.5f;
        check(std::fabs(largerCentreX - centreX) < 0.5f, "without moving the cluster");

        Layout wider = layout;
        wider.attackSpacing = 1.5f;
        const Resolved spread = resolve(wider, kWidth, kHeight);
        check(spread.buttons[1].x - spread.buttons[0].x > pitch,
              "and a spacing multiplier widens the gap between them");
    }

    // ------------------------------------------------------- what a finger means
    {
        uint32_t held[4] = {0, 0, 0, 0};

        // Dead centre of the low punch.
        const float lpX = geometry.buttons[0].x + geometry.buttons[0].w * 0.5f;
        const float lpY = geometry.buttons[0].y + geometry.buttons[0].h * 0.5f;
        uint32_t mask = buttonsFor({finger(0, lpX, lpY)}, layout, kWidth, kHeight, held, 4);
        check(mask == af::Button::kX, "a finger on LP presses exactly LP");

        // The middle of the d-pad is not a direction: a thumb resting there should not
        // walk, which is what the dead zone is for.
        mask = buttonsFor({finger(1, geometry.dpadCentreX, geometry.dpadCentreY)}, layout, kWidth,
                          kHeight, held, 4);
        check(mask == 0u, "the middle of the d-pad presses nothing");

        // Straight out to the right.
        const float reach = geometry.dpadSize * 1.2f;
        mask = buttonsFor({finger(2, geometry.dpadCentreX + reach, geometry.dpadCentreY)}, layout,
                          kWidth, kHeight, held, 4);
        check(mask == af::Button::kRight, "and a thumb held out to the right walks forward");

        // Up and to the right, which is the diagonal a jump-in needs.
        mask = buttonsFor(
            {finger(3, geometry.dpadCentreX + reach * 0.7f, geometry.dpadCentreY - reach * 0.7f)},
            layout, kWidth, kHeight, held, 4);
        check(mask == (af::Button::kUp | af::Button::kRight),
              "a thumb up and to the right gives both, which is what a diagonal is");

        // Two thumbs at once: a direction and an attack. On a touchscreen there is no
        // other way to walk forward and punch.
        mask = buttonsFor({finger(0, lpX, lpY), finger(2, geometry.dpadCentreX + reach,
                                                      geometry.dpadCentreY)},
                          layout, kWidth, kHeight, held, 4);
        check(mask == (af::Button::kX | af::Button::kRight),
              "two fingers hold a direction and an attack at the same time");

        // Four-way mode reads the same up-and-right thumb as a plain right, which is
        // what "disable diagonals" means.
        Layout fourWay = layout;
        fourWay.eightWay = false;
        mask = buttonsFor(
            {finger(3, geometry.dpadCentreX + reach * 0.7f, geometry.dpadCentreY - reach * 0.7f)},
            fourWay, kWidth, kHeight, held, 4);
        check((mask & (af::Button::kUp | af::Button::kDown)) == 0u,
              "with diagonals off the same thumb gives no vertical direction");
    }

    // ------------------------------------------------------- sticky and gliding
    {
        uint32_t held[4] = {0, 0, 0, 0};
        const float reach = geometry.dpadSize * 1.2f;

        // A thumb that slides right off the pad. Without stickiness the direction goes
        // with it, which is how a dash turns into a walk.
        Layout loose = layout;
        loose.sticky = false;
        buttonsFor({finger(0, geometry.dpadCentreX + reach, geometry.dpadCentreY)}, loose,
                   kWidth, kHeight, held, 4);
        uint32_t mask = buttonsFor(
            {finger(0, geometry.dpadCentreX + reach * 4.0f, geometry.dpadCentreY)}, loose, kWidth,
            kHeight, held, 4);
        check(mask == 0u, "a thumb that slides off a loose d-pad lets go");

        // And with it on, the direction is held even though the thumb has left.
        held[0] = 0;
        Layout sticky = layout;
        sticky.sticky = true;
        buttonsFor({finger(1, geometry.dpadCentreX + reach, geometry.dpadCentreY)}, sticky,
                   kWidth, kHeight, held, 4);
        mask = buttonsFor(
            {finger(1, geometry.dpadCentreX + reach * 4.0f, geometry.dpadCentreY)}, sticky, kWidth,
            kHeight, held, 4);
        check(mask == af::Button::kRight, "a sticky d-pad keeps the direction when the thumb leaves");

        // Gliding: a thumb that rolls from LP onto HP. With gliding on it presses HP;
        // with it off it keeps the LP it landed on until it lifts.
        held[2] = 0;
        const float hpX = geometry.buttons[2].x + geometry.buttons[2].w * 0.5f;
        const float hpY = geometry.buttons[2].y + geometry.buttons[2].h * 0.5f;
        const float lpX = geometry.buttons[0].x + geometry.buttons[0].w * 0.5f;
        const float lpY = geometry.buttons[0].y + geometry.buttons[0].h * 0.5f;
        buttonsFor({finger(2, lpX, lpY)}, layout, kWidth, kHeight, held, 4);
        mask = buttonsFor({finger(2, hpX, hpY)}, layout, kWidth, kHeight, held, 4);
        check(mask == af::Button::kZ, "rolling a thumb onto HP presses HP");

        held[2] = 0;
        Layout noGlide = layout;
        noGlide.gliding = false;
        buttonsFor({finger(2, lpX, lpY)}, noGlide, kWidth, kHeight, held, 4);
        mask = buttonsFor({finger(2, hpX, hpY)}, noGlide, kWidth, kHeight, held, 4);
        check(mask == af::Button::kX, "and with gliding off it keeps the button it landed on");

        // A finger that lifts is only forgotten if the caller stops sending it, so
        // holding a mask in hand over the lift must not leave the button stuck on.
        held[2] = af::Button::kX;
        mask = buttonsFor({}, layout, kWidth, kHeight, held, 4);
        check(mask == 0u, "a finger that has gone presses nothing");
    }

    // ------------------------------------------------------------- the settings file
    {
        Layout written;
        written.resetToDefaults();
        written.attacks.x = 0.8125f;
        written.attacks.y = 0.625f;
        written.attacks.scale = 1.25f;
        written.directions.x = 0.125f;
        written.start.y = 0.9375f;
        written.attackSpacing = 1.2f;
        written.directionRadius = 0.9f;
        written.opacityPercent = 55;
        written.hideSeconds = 3.0f;
        written.sticky = true;
        written.eightWay = false;
        written.gliding = false;
        written.snapGrid = 0.03125f;

        const std::string text = written.toIniText();
        const af::IniFile document = af::IniFile::parse(text);
        Layout readBack;
        readBack.resetToDefaults();
        readBack.read(document);

        check(std::fabs(readBack.attacks.x - written.attacks.x) < 0.001f,
              "a written layout reads back with the same attack x");
        check(std::fabs(readBack.attacks.y - written.attacks.y) < 0.001f, "and y");
        check(std::fabs(readBack.attacks.scale - written.attacks.scale) < 0.01f, "and scale");
        check(std::fabs(readBack.directions.x - written.directions.x) < 0.001f,
              "and the d-pad's position");
        check(std::fabs(readBack.start.y - written.start.y) < 0.001f, "and start's");
        check(std::fabs(readBack.attackSpacing - written.attackSpacing) < 0.01f, "and the spacing");
        check(std::fabs(readBack.directionRadius - written.directionRadius) < 0.01f,
              "and the d-pad's reach");
        check(readBack.opacityPercent == written.opacityPercent, "and the brightness");
        check(std::fabs(readBack.hideSeconds - written.hideSeconds) < 0.01f,
              "and how long it waits before fading");
        check(readBack.sticky == written.sticky, "and stickiness");
        check(readBack.eightWay == written.eightWay, "and the diagonal setting, inverted key and all");
        check(readBack.gliding == written.gliding, "and gliding");
        check(std::fabs(readBack.snapGrid - written.snapGrid) < 0.001f, "and the snap grid");

        // A document that says nothing at all leaves the defaults standing, which is
        // what a first run looks like.
        const af::IniFile empty = af::IniFile::parse("");
        Layout untouched;
        untouched.resetToDefaults();
        untouched.read(empty);
        check(std::fabs(untouched.attacks.x - 0.708f) < 0.001f,
              "a settings file with no pad section keeps the shipped layout");
    }

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}
