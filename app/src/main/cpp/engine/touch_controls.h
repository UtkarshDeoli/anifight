// The on-screen pad: the only way to play on a device with no keyboard.
//
// The pad is a pure function of a set of contact points plus a remembered layout,
// which keeps it out of the engine's state machine: the engine collects whatever
// fingers are down, asks this class which buttons that means, and merges the answer
// with the keyboard.
//
// It is laid out as *fractions of the screen* rather than in the game's own
// coordinate space, which is the format PPSSPP uses for its Android pad and the
// reason a saved layout survives a rotation or a phone with a different aspect
// ratio. See `touch_layout.h` for the layout itself and why it is shaped that way.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/motif.h"
#include "engine/touch_layout.h"
#include "gfx/renderer.h"

namespace af {

class IniFile;

namespace touchpad {

// Fingers the pad will remember between ticks. Ten is what Android reports and then
// some; a finger that has never been seen still presses, it just does not get the
// memory that makes a sticky d-pad work.
constexpr int kMaxFingers = 10;

class Pad {
public:
    Pad();

    [[nodiscard]] const Layout& layout() const { return layout_; }
    [[nodiscard]] Layout& layout() { return layout_; }

    // Reads the layout out of a parsed settings document, falling back to the
    // shipped arrangement for anything the document does not mention.
    void load(const IniFile& document);
    // The layout as settings text, ready to be written to a file.
    [[nodiscard]] std::string settingsText() const { return layout_.toIniText(); }

    // Which buttons the fingers in `contacts` mean, in device pixels.
    //
    // Fingers that have gone are forgotten here rather than by the caller, so a
    // lifted finger cannot leave a button stuck on.
    uint32_t buttonsFor(const std::vector<Contact>& contacts, float screenWidth,
                        float screenHeight);

    // Draws the pad, lighting the buttons in `held` so a press is visible even when
    // the game gives no other feedback.
    //
    // `fade` asks for the pad to dim itself once nothing has been touched for
    // `hideSeconds`. A menu wants `fade` off -- the pad is how a player finds their
    // way around it -- and a fight wants it on, because the pad is drawn over the
    // fighters there and the alternative is a thumb over a pair of ankles.
    void draw(Renderer& renderer, const FontTable& fonts, uint32_t held, float screenWidth,
              float screenHeight, double dt, bool fade);

    // The pad's current brightness, 0..1. Exposed so a screen that lays something out
    // in the pad's band can tell whether the pad has faded down right now.
    [[nodiscard]] float opacity() const { return opacity_; }

private:
    Layout layout_;
    // What each finger was holding last tick. Indexed by contact id.
    std::array<uint32_t, kMaxFingers> fingerHeld_{};
    // Which fingers were down last tick, so that the ones that left can be forgotten.
    std::array<bool, kMaxFingers> fingerDown_{};

    float opacity_ = 1.0f;
    double idleSeconds_ = 0.0;
};

}  // namespace touchpad
}  // namespace af
