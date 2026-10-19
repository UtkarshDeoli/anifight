#include "engine/touch_controls.h"

#include <algorithm>
#include <cmath>

#include "core/ini.h"
#include "engine/input.h"
#include "gfx/font.h"

namespace af {
namespace touchpad {
namespace {

// How bright the pad is while it is being used and while it is not, and how fast it
// moves between the two. The fade is quicker to appear than to leave, because a thumb
// arriving wants the button lit now and a thumb leaving does not.
constexpr float kFadeInPerSecond = 9.0f;
constexpr float kFadeOutPerSecond = 2.2f;

}  // namespace

Pad::Pad() {
    layout_.resetToDefaults();
    fingerHeld_.fill(0u);
    fingerDown_.fill(false);
}

void Pad::load(const IniFile& document) { layout_.read(document); }

uint32_t Pad::buttonsFor(const std::vector<Contact>& contacts, float screenWidth,
                         float screenHeight) {
    // Forget the fingers that have lifted before matching, so a button held by a
    // finger that is gone does not stay down. Doing it here rather than asking the
    // caller to means there is no way to forget.
    std::array<bool, kMaxFingers> down{};
    for (const Contact& contact : contacts) {
        if (contact.id >= 0 && contact.id < kMaxFingers) down[static_cast<size_t>(contact.id)] = true;
    }
    for (int id = 0; id < kMaxFingers; ++id) {
        if (!down[static_cast<size_t>(id)] && fingerDown_[static_cast<size_t>(id)]) {
            fingerHeld_[static_cast<size_t>(id)] = 0u;
        }
    }
    fingerDown_ = down;

    // Qualified: an unqualified call here would find this same member function.
    return touchpad::buttonsFor(contacts, layout_, screenWidth, screenHeight, fingerHeld_.data(),
                                kMaxFingers);
}

void Pad::draw(Renderer& renderer, const FontTable& fonts, uint32_t held, float screenWidth,
               float screenHeight, double dt, bool fade) {
    // The pad dims itself while it is being ignored. A pad that is always at full
    // brightness sits on top of the fighters' feet for the whole match; a pad that
    // vanishes outright cannot be aimed at. The compromise is a pad that is bright
    // while a thumb is near it and a ghost while it is not.
    idleSeconds_ = held != 0 ? 0.0 : idleSeconds_ + dt;
    const bool idle = fade && idleSeconds_ >= layout_.hideSeconds;
    const float target =
        idle ? layout_.idleOpacity : static_cast<float>(layout_.opacityPercent) / 100.0f;
    const float rate = target < opacity_ ? kFadeOutPerSecond : kFadeInPerSecond;
    const float step = static_cast<float>(dt) * rate;
    if (opacity_ < target) {
        opacity_ = std::min(target, opacity_ + step);
    } else if (opacity_ > target) {
        opacity_ = std::max(target, opacity_ - step);
    }
    const float a = opacity_;

    const Color face{1.0f, 1.0f, 1.0f, 0.20f * a};
    const Color rim{1.0f, 1.0f, 1.0f, 0.42f * a};
    const Color lit{0.98f, 0.84f, 0.36f, 0.70f * a};
    const Color litRim{1.0f, 0.97f, 0.80f, 0.98f * a};
    const Color ink{1.0f, 1.0f, 1.0f, 0.82f * a};

    const Resolved geometry = resolve(layout_, screenWidth, screenHeight);

    // Everything below is in device pixels, one unit to one pixel, so that the pad
    // neither moves nor changes size when the game's own screen is letterboxed
    // differently. The renderer's own space is restored on the way out whatever
    // happens, which is why this is a scope rather than a pair of calls a caller
    // could get wrong.
    renderer.pushPixelSpace(screenWidth, screenHeight);
    const Texture& disc = renderer.disc();

    auto plate = [&](const Resolved::Button& button, bool round) {
        const bool down = (held & button.button) != 0;
        // A pressed button grows a little as well as brightening: on glass, where
        // there is no travel to feel, the size change is what says "that one".
        const float grow = down ? 1.06f : 1.0f;
        const float width = button.w * grow;
        const float height = button.h * grow;
        const float x = button.x - (width - button.w) * 0.5f;
        const float y = button.y - (height - button.h) * 0.5f;

        if (round) {
            renderer.drawSprite(disc, x, y, width, height,
                                Rect{static_cast<float>(disc.width()),
                                     static_cast<float>(disc.height()), 0, 0},
                                down ? litRim : rim);
            const float inset = std::max(1.5f, button.w * 0.055f);
            renderer.drawSprite(disc, x + inset, y + inset, width - inset * 2.0f,
                                height - inset * 2.0f,
                                Rect{static_cast<float>(disc.width()),
                                     static_cast<float>(disc.height()), 0, 0},
                                down ? lit : face);
        } else {
            renderer.fillRect(x, y, width, height, down ? lit : face);
            const float edge = std::max(1.0f, button.h * 0.05f);
            renderer.fillRect(x, y, width, edge, down ? litRim : rim);
            renderer.fillRect(x, y + height - edge, width, edge, down ? litRim : rim);
            renderer.fillRect(x, y, edge, height, down ? litRim : rim);
            renderer.fillRect(x + width - edge, y, edge, height, down ? litRim : rim);
        }
    };

    bool startVisible = false;
    for (const Resolved::Button& button : geometry.buttons) {
        if (button.button == 0) continue;
        if (button.control == Resolved::Control::Start) {
            startVisible = true;
            continue;
        }
        if (button.control == Resolved::Control::Directions && !layout_.directions.show) continue;
        if (button.control == Resolved::Control::Attacks && !layout_.attacks.show) continue;
        // The attack buttons are squares and the d-pad round; Start is a bar. Reading
        // the shape off the control rather than storing it keeps the layout to what it
        // needs to be: where things are.
        const bool round = button.control == Resolved::Control::Directions;
        plate(button, round);
    }
    if (startVisible && layout_.start.show) plate(geometry.buttons[10], false);  // NOLINT

    // Arrows on the d-pad arms, built one column at a time: the renderer draws quads,
    // and at this size a stepped triangle reads as a triangle.
    if (layout_.directions.show) {
        auto arrow = [&](const Resolved::Button& button, float dx, float dy) {
            const float cx = button.x + button.w * 0.5f;
            const float cy = button.y + button.h * 0.5f;
            const float unit = std::max(1.0f, button.w * 0.075f);
            constexpr int kSteps = 7;
            for (int i = 0; i < kSteps; ++i) {
                const float wide = static_cast<float>(kSteps - i) * unit;
                const float along = (static_cast<float>(i) - (kSteps - 1) * 0.5f) * unit;
                if (dx != 0.0f) {
                    renderer.fillRect(cx + along * dx, cy - wide * 0.5f, unit, wide, ink);
                } else {
                    renderer.fillRect(cx - wide * 0.5f, cy + along * dy, wide, unit, ink);
                }
            }
        };
        arrow(geometry.buttons[6], 0.0f, -1.0f);   // up     NOLINT
        arrow(geometry.buttons[7], 0.0f, 1.0f);    // down   NOLINT
        arrow(geometry.buttons[8], -1.0f, 0.0f);   // left   NOLINT
        arrow(geometry.buttons[9], 1.0f, 0.0f);    // right  NOLINT
    }

    // Labels last, so they read on top of the highlight. The font is a screenpack
    // font measured in the game's own units, so it has to be scaled up to the pixel
    // space the buttons are drawn in -- which is the one place the two spaces meet.
    Font* font = fonts.get(1);
    if (font != nullptr) {
        for (const Resolved::Button& button : geometry.buttons) {
            if (button.label == nullptr) continue;
            if (button.control == Resolved::Control::Attacks && !layout_.attacks.show) continue;
            if (button.control == Resolved::Control::Start && !layout_.start.show) continue;
            const float scale = std::max(1.0f, button.h / 72.0f);
            font->draw(renderer, button.x + button.w * 0.5f, button.y + button.h * 0.5f,
                       button.label, 0, TextAlign::Center,
                       Color{1.0f, 1.0f, 1.0f, 0.90f * a}, scale);
        }
    }

    renderer.popSpace();
}

}  // namespace touchpad
}  // namespace af
