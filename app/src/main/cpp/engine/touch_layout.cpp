#include "engine/touch_layout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "core/ini.h"

namespace af {
namespace touchpad {
namespace {

// The shipped layout, as fractions of the screen.
//
// These are the numbers the pad was drawn at when it was hard-coded in the fight
// screen's own coordinate space, converted once: a phone held sideways is wider
// than it is tall, the virtual 320x240 screen is letterboxed into the middle of it,
// and the pad sat inside that. Expressed as screen fractions they describe the same
// arrangement on any screen, which is the whole point of adopting PPSSPP's format.
constexpr float kDefaultAttackX = 0.708f;
constexpr float kDefaultAttackY = 0.708f;
constexpr float kDefaultDPadX = 0.285f;
constexpr float kDefaultDPadY = 0.708f;
constexpr float kDefaultStartX = 0.5f;
constexpr float kDefaultStartY = 0.933f;

// One button is this fraction of the screen's *shorter* side. The shorter side,
// because a button has to be thumb-sized, and a thumb does not care how wide the
// phone is: 0.133 of 1080 is 144 pixels, which is the size the pad has always been
// and is comfortably larger than a fingertip.
constexpr float kButtonFraction = 0.133f;
// The gap between neighbouring attack buttons, as a multiple of a button.
constexpr float kAttackPitch = 1.125f;
// How far a d-pad arm sits from the d-pad's centre, as a multiple of a button.
constexpr float kArmReach = 1.15f;
// The d-pad's own hit radius, in buttons. Larger than the arms so that a thumb
// anywhere in the cross is inside it: the arms are only where the arrows are drawn.
constexpr float kDPadHitRadius = 1.7f;
// Start is a bar rather than a square.
constexpr float kStartWidthFraction = 3.0f;

float clamp01(float value, float fallback) {
    if (!(value >= 0.0f)) return fallback;
    return std::min(1.0f, value);
}

// A key that is present but unreadable leaves the field alone, which is the same
// treatment a missing key gets.
void readPlacement(const IniFile& document, const IniSection* section, const char* xKey,
                   const char* yKey, const char* scaleKey, const char* showKey,
                   Placement& out) {
    if (section == nullptr) return;
    const float x = IniFile::getFloat(*section, xKey, out.x);
    const float y = IniFile::getFloat(*section, yKey, out.y);
    if (x >= 0.0f) out.x = std::min(1.0f, x);
    if (y >= 0.0f) out.y = std::min(1.0f, y);
    out.scale = std::max(0.25f, IniFile::getFloat(*section, scaleKey, out.scale));
    if (showKey != nullptr) out.show = IniFile::getBool(*section, showKey, out.show);
    (void)document;
}

}  // namespace

void Layout::resetToDefaults() {
    attacks = Placement{};
    attacks.x = kDefaultAttackX;
    attacks.y = kDefaultAttackY;
    attacks.scale = 1.0f;
    attacks.show = true;

    directions = Placement{};
    directions.x = kDefaultDPadX;
    directions.y = kDefaultDPadY;
    directions.scale = 1.0f;
    directions.show = true;

    start = Placement{};
    start.x = kDefaultStartX;
    start.y = kDefaultStartY;
    start.scale = 1.0f;
    start.show = true;

    attackSpacing = 1.0f;
    directionRadius = 1.0f;
    opacityPercent = 65;
    hideSeconds = 8.0f;
    idleOpacity = 0.30f;
    sticky = false;
    eightWay = true;
    gliding = true;
    snapGrid = 0.0f;
}

void Layout::read(const IniFile& document) {
    // A section that is not there at all is not an error: it is a first run, and the
    // defaults stand.
    const IniSection* section = document.section("touchcontrols");
    if (section == nullptr) return;

    readPlacement(document, section, "actionbuttoncenterx", "actionbuttoncentery",
                  "actionbuttonscale", nullptr, attacks);
    readPlacement(document, section, "dpadx", "dpady", "dpadscale", "showtouchdpad",
                  directions);
    readPlacement(document, section, "startkeyx", "startkeyy", "startkeyscale",
                  "showtouchstart", start);

    attackSpacing = std::max(0.5f, IniFile::getFloat(*section, "actionbuttonspacing", attackSpacing));
    directionRadius = std::max(0.5f, IniFile::getFloat(*section, "dpadradius", directionRadius));
    opacityPercent = std::clamp(IniFile::getInt(*section, "touchbuttonopacity", opacityPercent), 5, 100);
    hideSeconds = std::max(0.0f, IniFile::getFloat(*section, "touchbuttonhideseconds", hideSeconds));
    idleOpacity = std::clamp(IniFile::getFloat(*section, "touchbuttonidleopacity", idleOpacity), 0.05f, 1.0f);
    sticky = IniFile::getBool(*section, "stickytouchdpad", sticky);
    // The key is named for what it turns *off*, as PPSSPP's is, because that is the
    // way round a player meets it: diagonals are on unless disabled.
    eightWay = !IniFile::getBool(*section, "disabledpaddiagonals", !eightWay);
    gliding = IniFile::getBool(*section, "touchgliding", gliding);
    snapGrid = std::max(0.0f, IniFile::getFloat(*section, "touchsnapgridsize", snapGrid));

    // Anything still unconfigured gets the default, which is how `-1` is meant to be
    // read: a layout written by an older version that had a key this one does not.
    if (!attacks.configured()) {
        const float x = attacks.x;
        const float y = attacks.y;
        attacks = Placement{};
        attacks.x = x >= 0.0f ? x : kDefaultAttackX;
        attacks.y = y >= 0.0f ? y : kDefaultAttackY;
    }
    if (!directions.configured()) {
        const float x = directions.x;
        const float y = directions.y;
        directions = Placement{};
        directions.x = x >= 0.0f ? x : kDefaultDPadX;
        directions.y = y >= 0.0f ? y : kDefaultDPadY;
    }
    if (!start.configured()) {
        const float x = start.x;
        const float y = start.y;
        start = Placement{};
        start.x = x >= 0.0f ? x : kDefaultStartX;
        start.y = y >= 0.0f ? y : kDefaultStartY;
    }
}

std::string Layout::toIniText() const {
    char text[1024];
    std::snprintf(text, sizeof(text),
                  "[TouchControls]\n"
                  "; Fractions of the screen, as PPSSPP stores its own pad: 0,0 is the\n"
                  "; top-left corner and 1,1 the bottom-right.\n"
                  "ActionButtonCenterX = %.4f\n"
                  "ActionButtonCenterY = %.4f\n"
                  "ActionButtonScale = %.3f\n"
                  "ActionButtonSpacing = %.3f\n"
                  "DPadX = %.4f\n"
                  "DPadY = %.4f\n"
                  "DPadScale = %.3f\n"
                  "DPadRadius = %.3f\n"
                  "ShowTouchDpad = %s\n"
                  "StartKeyX = %.4f\n"
                  "StartKeyY = %.4f\n"
                  "StartKeyScale = %.3f\n"
                  "ShowTouchStart = %s\n"
                  "TouchButtonOpacity = %d\n"
                  "TouchButtonHideSeconds = %.1f\n"
                  "TouchButtonIdleOpacity = %.2f\n"
                  "StickyTouchDPad = %s\n"
                  "DisableDpadDiagonals = %s\n"
                  "TouchGliding = %s\n"
                  "TouchSnapGridSize = %.4f\n",
                  attacks.x, attacks.y, attacks.scale, attackSpacing, directions.x,
                  directions.y, directions.scale, directionRadius,
                  directions.show ? "true" : "false", start.x, start.y, start.scale,
                  start.show ? "true" : "false", opacityPercent, hideSeconds, idleOpacity,
                  sticky ? "true" : "false", eightWay ? "false" : "true",
                  gliding ? "true" : "false", snapGrid);
    return text;
}

Resolved resolve(const Layout& layout, float screenWidth, float screenHeight) {
    Resolved out{};
    const float shorter = std::max(1.0f, std::min(screenWidth, screenHeight));

    const float attackSize = std::max(8.0f, shorter * kButtonFraction * layout.attacks.scale);
    const float attackPitch = attackSize * kAttackPitch * layout.attackSpacing;
    // The cluster is centred on its placement: three buttons and two gaps across,
    // two rows and one gap down.
    const float attackLeft =
        layout.attacks.x * screenWidth - (attackSize * 3.0f + (attackPitch - attackSize) * 2.0f) * 0.5f;
    const float attackTop = layout.attacks.y * screenHeight - (attackSize * 2.0f + (attackPitch - attackSize)) * 0.5f;

    static constexpr char kAttackLabels[6][3] = {"LP", "MP", "HP", "LK", "MK", "HK"};
    static constexpr uint32_t kAttackButtons[6] = {Button::kX, Button::kY, Button::kZ,
                                                   Button::kA, Button::kB, Button::kC};
    for (int i = 0; i < 6; ++i) {
        const int column = i % 3;
        const int row = i / 3;
        Resolved::Button& button = out.buttons[static_cast<size_t>(i)];
        button.x = attackLeft + static_cast<float>(column) * attackPitch;
        button.y = attackTop + static_cast<float>(row) * attackPitch;
        button.w = attackSize;
        button.h = attackSize;
        button.button = kAttackButtons[i];
        button.label = kAttackLabels[i];
        button.control = Resolved::Control::Attacks;
    }

    const float dirSize = std::max(8.0f, shorter * kButtonFraction * layout.directions.scale);
    const float armReach = dirSize * kArmReach * layout.directionRadius;
    out.dpadCentreX = layout.directions.x * screenWidth;
    out.dpadCentreY = layout.directions.y * screenHeight;
    out.dpadSize = dirSize;
    out.dpadRadius = dirSize * kDPadHitRadius * layout.directionRadius;

    // Up, down, left, right: the arms are drawn at the compass points, and the
    // direction itself is read from the angle the thumb sits at rather than from
    // which arm it is nearest, so the centre of the cross is usable.
    static constexpr uint32_t kArmButtons[4] = {Button::kUp, Button::kDown, Button::kLeft,
                                                Button::kRight};
    static constexpr float kArmX[4] = {0.0f, 0.0f, -1.0f, 1.0f};
    static constexpr float kArmY[4] = {-1.0f, 1.0f, 0.0f, 0.0f};
    for (int i = 0; i < 4; ++i) {
        Resolved::Button& button = out.buttons[static_cast<size_t>(6 + i)];
        button.x = out.dpadCentreX + kArmX[i] * armReach - dirSize * 0.5f;
        button.y = out.dpadCentreY + kArmY[i] * armReach - dirSize * 0.5f;
        button.w = dirSize;
        button.h = dirSize;
        button.button = kArmButtons[i];
        button.label = nullptr;
        button.control = Resolved::Control::Directions;
    }

    const float startSize = std::max(8.0f, shorter * kButtonFraction * layout.start.scale);
    Resolved::Button& startButton = out.buttons[10];
    startButton.w = startSize * kStartWidthFraction;
    startButton.h = startSize;
    startButton.x = layout.start.x * screenWidth - startButton.w * 0.5f;
    startButton.y = layout.start.y * screenHeight - startButton.h * 0.5f;
    startButton.button = Button::kStart;
    startButton.label = "START";
    startButton.control = Resolved::Control::Start;

    return out;
}

uint32_t buttonsFor(const std::vector<Contact>& contacts, const Layout& layout, float screenWidth,
                    float screenHeight, uint32_t* fingerHeld, int fingerSlots) {
    const Resolved geometry = resolve(layout, screenWidth, screenHeight);
    uint32_t held = 0;

    for (const Contact& contact : contacts) {
        // What this finger was holding last tick. Remembering it is what makes
        // sliding and stickiness possible: a finger that has not moved keeps its
        // button even when the check below stops matching.
        uint32_t previous = 0;
        if (fingerHeld != nullptr && contact.id >= 0 && contact.id < fingerSlots) {
            previous = fingerHeld[contact.id];
        }

        uint32_t matched = 0;
        if (layout.directions.show && geometry.dpadRadius > 0.0f) {
            // The d-pad is read from the *angle* of the thumb, not from which arm is
            // nearest, which is what makes a diagonal come out of a slide rather than
            // out of a lucky landing. PPSSPP's own recipe, including its numbers: the
            // dead zone in the middle, and the four-way quantisation when diagonals
            // are disabled.
            const float dx = (contact.x - geometry.dpadCentreX) / std::max(1.0f, geometry.dpadSize);
            const float dy = (contact.y - geometry.dpadCentreY) / std::max(1.0f, geometry.dpadSize);
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance < kDPadHitRadius * layout.directionRadius) {
                if (distance >= 0.2f) {
                    const float angle = std::atan2(dy, dx) / (2.0f * 3.14159265f);
                    int index = 0;
                    if (layout.eightWay) {
                        index = static_cast<int>(std::floor(angle * 8.0f + 0.5f)) & 7;
                        if (index == 1) matched = Button::kRight | Button::kDown;
                        else if (index == 3) matched = Button::kDown | Button::kLeft;
                        else if (index == 5) matched = Button::kLeft | Button::kUp;
                        else if (index == 7) matched = Button::kUp | Button::kRight;
                    } else {
                        index = static_cast<int>(std::floor(angle * 4.0f + 0.5f)) & 3;
                    }
                    if (matched == 0) {
                        matched = index == 0 ? Button::kRight
                                             : index == 2 ? Button::kDown
                                                          : index == 4 ? Button::kLeft
                                                                       : Button::kUp;
                    }
                } else if (layout.sticky) {
                    // Inside the dead zone with a sticky pad: keep whatever was being
                    // held rather than snapping to nothing every time a thumb crosses
                    // the middle of the cross.
                    matched = previous & (Button::kUp | Button::kDown | Button::kLeft | Button::kRight);
                }
            } else if (layout.sticky) {
                matched = previous & (Button::kUp | Button::kDown | Button::kLeft | Button::kRight);
            }
        }

        for (const Resolved::Button& button : geometry.buttons) {
            if (button.button == 0) continue;
            if (button.control == Resolved::Control::Directions) continue;
            if (button.control == Resolved::Control::Attacks && !layout.attacks.show) continue;
            if (button.control == Resolved::Control::Start && !layout.start.show) continue;
            if (contact.x < button.x || contact.x >= button.x + button.w) continue;
            if (contact.y < button.y || contact.y >= button.y + button.h) continue;
            matched |= button.button;
            break;
        }

        // Gliding, or not. With it on, a finger follows the glass: sliding off a
        // button releases it and sliding onto another presses that one, which is what
        // lets a thumb roll across LP MP HP. With it off, a finger that has landed on
        // a button keeps it until it lifts and does not retarget -- the safer reading
        // on a small screen, where a thumb is always drifting somewhere.
        if (!layout.gliding && previous != 0) matched = previous;

        if (fingerHeld != nullptr && contact.id >= 0 && contact.id < fingerSlots) {
            fingerHeld[contact.id] = matched;
        }
        held |= matched;
    }

    return held;
}

float screenCentreX(float virtualWidth) { return virtualWidth * 0.5f; }

}  // namespace touchpad
}  // namespace af
