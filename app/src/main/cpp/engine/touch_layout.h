// Where the on-screen pad's controls go, in the form PPSSPP stores its own.
//
// PPSSPP's Android pad is the most-used touch fighting-game control scheme there
// is, and the thing worth taking from it is not its art but its *format*: every
// control is a position expressed as a **fraction of the screen**, a scale, and a
// show flag, saved as a handful of flat keys, with `-1` meaning "not configured
// yet" so that a fresh install computes sensible defaults and a player's saved
// layout always wins. Positions in fractions rather than in the game's own
// coordinate space is what makes a layout survive a change of aspect ratio, a
// rotation, or -- in this engine's case -- a change of `localcoord`.
//
// What that buys a fighting game specifically: the six attack buttons are *one*
// control with a centre and a spacing, so the player can move the whole cluster
// with one drag instead of lining up six rectangles by hand, and the d-pad is a
// centre and a radius rather than four separate plates.
//
// This file is pure data and parsing, with no renderer and no input: everything
// here can be tested without a device.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/input.h"

namespace af {

class IniFile;

namespace touchpad {

// A finger, in device pixels. The pad is the one part of the game that is laid out
// in screen space rather than in the game's own coordinate space, because a control
// should be the same size under the thumb whichever way the game's screen is
// letterboxed -- which is the arrangement PPSSPP uses and the reason a saved layout
// survives a change of aspect ratio.
struct Contact {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
};

// Where a control sits, as PPSSPP's ConfigTouchPos does.
struct Placement {
    // Fractions of the screen: 0,0 is the top-left corner and 1,1 the bottom-right.
    // `kUnset` in either means the layout has never been configured and the default
    // should be computed instead.
    static constexpr float kUnset = -1.0f;
    float x = kUnset;
    float y = kUnset;
    float scale = 1.0f;
    bool show = true;

    [[nodiscard]] bool configured() const { return x >= 0.0f && y >= 0.0f; }
};

// The whole pad: three placements, the two spacings that turn them into clusters,
// and the settings that are not about position at all.
struct Layout {
    // The centre of the six attack buttons, which are laid out as a 3x2 grid around
    // it: LP MP HP across the top, LK MK HK below.
    Placement attacks;
    // The centre of the d-pad, whose four arms sit at `armReach` from it.
    Placement directions;
    // The Start bar, kept away from both thumbs because an accidental press costs a
    // round.
    Placement start;

    // Multipliers on the default gaps: 1.0 is the shipped spacing.
    float attackSpacing = 1.0f;
    float directionRadius = 1.0f;

    // How bright the pad draws while it is in use, in per cent, and how long it waits
    // before fading down to `idleOpacity`. PPSSPP's own keys are `TouchButtonOpacity`
    // (default 65) and `TouchButtonHideSeconds` (8).
    int opacityPercent = 65;
    float hideSeconds = 8.0f;
    // The floor the fade goes down to. Deliberately not zero: a control you cannot
    // find is worse than one that is in the way. PPSSPP's per-control equivalent is
    // SetMinimumAlpha.
    float idleOpacity = 0.30f;

    // Hold a direction even when the thumb slides off the pad, rather than releasing
    // it. PPSSPP calls this StickyTouchDPad, and for a fighting game it is the
    // difference between a dash coming out and a walk.
    bool sticky = false;
    // Eight directions from the d-pad, or four. PPSSPP: DisableDpadDiagonals.
    bool eightWay = true;
    // A finger that slides from one button onto another presses the one it lands on.
    // PPSSPP: TouchGliding.
    bool gliding = true;
    // Round positions to this many screen-fractions, or don't when it is 0. The
    // editor uses it; PPSSPP expresses the same thing in pixels (TouchSnapGridSize).
    float snapGrid = 0.0f;

    // The values a fresh install starts from, and what the editor's Reset returns to.
    // They reproduce the layout the engine shipped before it was configurable, so that
    // nothing moves for a player who never opens the editor.
    void resetToDefaults();

    // Reads `[TouchControls]` from a parsed document. Every key it does not find
    // keeps its current value, so a half-written section is a usable one.
    void read(const IniFile& document);
    // The same section, written back out. Kept as text rather than as an IniFile
    // because the engine's reader has no writer -- and a settings file is five lines.
    [[nodiscard]] std::string toIniText() const;
};

// Where the on-screen pad lives in the game tree, for the screens that have
// something to say in the band it covers.
//
// The pad is drawn in *screen* space, not in the game's own coordinate space, so a
// screen laying something out under it has to know where the game's own middle is:
// the pad's two thumb clusters leave a gap centred on the middle of the screen.
[[nodiscard]] float screenCentreX(float virtualWidth);

// A layout worked out at a particular screen size: every control's rectangle in
// device pixels, plus the d-pad's centre and radius.
//
// Exposed rather than kept private because two very different callers need it --
// the input matcher below, and the layout editor that drags these rectangles around
// -- and because a test can then pin the layout's arithmetic without a device.
struct Resolved {
    enum class Control { Attacks, Directions, Start };

    struct Button {
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        uint32_t button = 0;   // the input bit it produces
        const char* label = nullptr;  // nullptr for the d-pad arms, which draw arrows
        Control control = Control::Attacks;
    };
    // Six attacks (LP MP HP / LK MK HK), then four d-pad arms (up, down, left,
    // right), then Start.
    std::array<Button, 11> buttons{};

    float dpadCentreX = 0.0f;
    float dpadCentreY = 0.0f;
    float dpadSize = 0.0f;
    float dpadRadius = 0.0f;  // how far out a touch still counts as the d-pad
};

[[nodiscard]] Resolved resolve(const Layout& layout, float screenWidth, float screenHeight);

// The buttons a set of fingers means.
//
// Every finger is tested against every control and the results are OR'd -- holding a
// direction with one thumb while mashing an attack with the other has to work, and on
// a touchscreen there is no other way to walk forward and punch at once.
//
// `fingerHeld` is the pad's memory of what each finger was holding last tick, indexed
// by contact id and `fingerSlots` long; it is updated in place. That memory is what
// makes a sticky d-pad and a non-gliding press possible, and passing it in rather
// than keeping it in a global is what lets a test drive the pad exactly.
[[nodiscard]] uint32_t buttonsFor(const std::vector<Contact>& contacts, const Layout& layout,
                                 float screenWidth, float screenHeight, uint32_t* fingerHeld,
                                 int fingerSlots);

}  // namespace touchpad
}  // namespace af
