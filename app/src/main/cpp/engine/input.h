// Input, in the shape a fighting game wants it.
#pragma once

#include <cstdint>

namespace af {

// Buttons are named the way MUGEN names them so that .cmd files, which are
// written against these names, translate one-to-one later.
namespace Button {
constexpr uint32_t kUp = 1u << 0;
constexpr uint32_t kDown = 1u << 1;
constexpr uint32_t kLeft = 1u << 2;
constexpr uint32_t kRight = 1u << 3;
constexpr uint32_t kA = 1u << 4;
constexpr uint32_t kB = 1u << 5;
constexpr uint32_t kC = 1u << 6;
constexpr uint32_t kX = 1u << 7;
constexpr uint32_t kY = 1u << 8;
constexpr uint32_t kZ = 1u << 9;
constexpr uint32_t kStart = 1u << 10;
}  // namespace Button

// MUGEN's eight attack strengths, in the canonical order used by .cmd files.
constexpr uint32_t kAttackButtons[] = {
    Button::kX, Button::kY, Button::kZ, Button::kA, Button::kB, Button::kC,
};

struct InputState {
    uint32_t held = 0;
    uint32_t pressed = 0;   // transitioned to down this frame
    uint32_t released = 0;  // transitioned to up this frame

    // A finger is on the screen, anywhere. Set only for a screen that accepts
    // touch input, and for the screens that want "tap anywhere" -- a button mask
    // cannot express it, because the pad's zones cover only part of the glass.
    bool touched = false;

    [[nodiscard]] bool down(uint32_t button) const { return (held & button) != 0; }
    [[nodiscard]] bool wasPressed(uint32_t button) const { return (pressed & button) != 0; }
    [[nodiscard]] bool wasReleased(uint32_t button) const { return (released & button) != 0; }

    [[nodiscard]] int horizontalAxis() const {
        return (down(Button::kRight) ? 1 : 0) - (down(Button::kLeft) ? 1 : 0);
    }
    [[nodiscard]] int verticalAxis() const {
        return (down(Button::kDown) ? 1 : 0) - (down(Button::kUp) ? 1 : 0);
    }
};

}  // namespace af
