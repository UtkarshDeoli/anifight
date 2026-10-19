#include "engine/screens/layout_screen.h"

#include <algorithm>
#include <cmath>

#include "engine/engine.h"
#include "engine/motif.h"
#include "engine/touch_controls.h"
#include "gfx/font.h"
#include "gfx/renderer.h"

namespace af {
namespace {

// How close a touch has to land to a control to pick it up, as a fraction of the
// screen's shorter side. Generous on purpose: this is a screen for fingers, and a
// miss here means the player has to try again.
constexpr float kGrabRadius = 0.16f;

}  // namespace

float LayoutScreen::screenWidth() const {
    return engine() != nullptr ? static_cast<float>(engine()->renderer().framebufferWidth()) : 1.0f;
}

float LayoutScreen::screenHeight() const {
    return engine() != nullptr ? static_cast<float>(engine()->renderer().framebufferHeight()) : 1.0f;
}

void LayoutScreen::enter() {
    message_ = "DRAG A CONTROL TO MOVE IT - TAP TO PICK";
    messageTicks_ = 240;
}

LayoutScreen::Centre LayoutScreen::centreOf(Target target,
                                             const touchpad::Resolved& geometry) const {
    switch (target) {
        case Target::Attacks: {
            const touchpad::Resolved::Button& first = geometry.buttons[0];
            const touchpad::Resolved::Button& last = geometry.buttons[5];
            return Centre{(first.x + last.x + last.w) * 0.5f,
                          (first.y + last.y + last.h) * 0.5f};
        }
        case Target::Directions:
            return Centre{geometry.dpadCentreX, geometry.dpadCentreY};
        case Target::Start: {
            const touchpad::Resolved::Button& button = geometry.buttons[10];
            return Centre{button.x + button.w * 0.5f, button.y + button.h * 0.5f};
        }
    }
    return Centre{0.0f, 0.0f};
}

void LayoutScreen::selectNearest(float x, float y, float screenWidth, float screenHeight) {
    if (engine() == nullptr) return;
    const touchpad::Layout& layout = engine()->pad().layout();
    const touchpad::Resolved geometry = touchpad::resolve(layout, screenWidth, screenHeight);
    const float radius = kGrabRadius * std::min(screenWidth, screenHeight);

    float best = radius;
    Target bestTarget = target_;
    const Target candidates[3] = {Target::Attacks, Target::Directions, Target::Start};
    for (Target candidate : candidates) {
        const Centre centre = centreOf(candidate, geometry);
        const float distance = std::sqrt((centre.x - x) * (centre.x - x) +
                                         (centre.y - y) * (centre.y - y));
        if (distance < best) {
            best = distance;
            bestTarget = candidate;
        }
    }
    if (best < radius) target_ = bestTarget;
}

void LayoutScreen::applyDrag(const touchpad::Resolved& geometry, float dx, float dy) {
    if (engine() == nullptr) return;
    touchpad::Layout& layout = engine()->pad().layout();
    touchpad::Placement* placement = nullptr;
    switch (target_) {
        case Target::Attacks: placement = &layout.attacks; break;
        case Target::Directions: placement = &layout.directions; break;
        case Target::Start: placement = &layout.start; break;
    }
    if (placement == nullptr) return;

    // The drag is applied to where the control was when the finger went down, so that
    // the control tracks the finger instead of accumulating its own rounding. In
    // fractions of the screen, which is the whole point of the format: the numbers
    // saved mean the same thing on a phone with a different screen.
    *placement = dragStartPlacement_;
    placement->x = std::clamp(placement->x + dx / std::max(1.0f, screenWidth()), 0.0f, 1.0f);
    placement->y = std::clamp(placement->y + dy / std::max(1.0f, screenHeight()), 0.0f, 1.0f);

    const float snap = layout.snapGrid;
    if (snap > 0.0f) {
        placement->x = std::round(placement->x / snap) * snap;
        placement->y = std::round(placement->y / snap) * snap;
    }
    (void)geometry;
}

void LayoutScreen::applyResize(float dx, float dy) {
    if (engine() == nullptr) return;
    touchpad::Layout& layout = engine()->pad().layout();
    // Vertical drag scales the control and horizontal drag widens the gap inside its
    // cluster, which is the pair of gestures PPSSPP's editor uses. Both are measured
    // against the screen's shorter side rather than in pixels, so that the same
    // movement of the thumb does the same thing on any phone.
    //
    // The d-pad has a radius rather than a spacing, since it has no neighbours to
    // spread out; it is the one control whose "wider" means "reaches further".
    const float shorter = std::min(screenWidth(), screenHeight());
    const float factor = std::clamp(dy / (shorter * 0.35f), -0.5f, 1.0f);
    switch (target_) {
        case Target::Attacks:
            layout.attackSpacing =
                std::clamp(dragStartSpacing_ * (1.0f + dx / (shorter * 0.5f)), 0.5f, 2.5f);
            layout.attacks.scale = std::clamp(dragStartPlacement_.scale * (1.0f + factor), 0.5f, 3.0f);
            break;
        case Target::Directions:
            layout.directionRadius =
                std::clamp(dragStartSpacing_ * (1.0f + dx / (shorter * 0.5f)), 0.5f, 2.5f);
            layout.directions.scale =
                std::clamp(dragStartPlacement_.scale * (1.0f + factor), 0.5f, 3.0f);
            break;
        case Target::Start:
            layout.start.scale = std::clamp(dragStartPlacement_.scale * (1.0f + factor), 0.5f, 3.0f);
            break;
    }
}

LayoutScreen::Box LayoutScreen::resetRect(float screenWidth, float screenHeight) const {
    const float shorter = std::min(screenWidth, screenHeight);
    const float height = shorter * 0.085f;
    return Box{screenWidth * 0.03f, screenHeight * 0.045f, height * 2.6f, height};
}

LayoutScreen::Box LayoutScreen::doneRect(float screenWidth, float screenHeight) const {
    const float shorter = std::min(screenWidth, screenHeight);
    const float height = shorter * 0.085f;
    const float width = height * 3.4f;
    return Box{(screenWidth - width) * 0.5f, screenHeight - height * 1.45f, width, height};
}

bool LayoutScreen::inside(const Box& box, float x, float y) {
    return x >= box.x && x < box.x + box.w && y >= box.y && y < box.y + box.h;
}

void LayoutScreen::touch(const std::vector<touchpad::Contact>& contacts) {
    if (engine() == nullptr) return;
    const float width = screenWidth();
    const float height = screenHeight();
    const touchpad::Resolved geometry = touchpad::resolve(engine()->pad().layout(), width, height);

    if (contacts.empty()) {
        if (dragging_) {
            // The finger has gone: the arrangement that was dragged to is the one to
            // keep, so it is written out now rather than on the way out of the screen.
            dragging_ = false;
            dragFinger_ = -1;
            engine()->saveSettings();
            message_ = "SAVED";
            messageTicks_ = 60;
        }
        return;
    }

    if (!dragging_) {
        // A new touch either presses one of the editor's own buttons or picks up the
        // control it landed nearest to.
        const touchpad::Contact& contact = contacts.front();
        if (inside(resetRect(width, height), contact.x, contact.y)) {
            engine()->pad().layout().resetToDefaults();
            engine()->saveSettings();
            message_ = "RESET TO THE SHIPPED LAYOUT";
            messageTicks_ = 120;
            return;
        }
        if (inside(doneRect(width, height), contact.x, contact.y)) {
            engine()->saveSettings();
            engine()->popScreen();
            return;
        }
        selectNearest(contact.x, contact.y, width, height);
        dragging_ = true;
        dragFinger_ = contact.id;
        dragStartX_ = contact.x;
        dragStartY_ = contact.y;
        const touchpad::Layout& layout = engine()->pad().layout();
        switch (target_) {
            case Target::Attacks: dragStartPlacement_ = layout.attacks; break;
            case Target::Directions: dragStartPlacement_ = layout.directions; break;
            case Target::Start: dragStartPlacement_ = layout.start; break;
        }
        dragStartSpacing_ = target_ == Target::Attacks ? layout.attackSpacing : layout.directionRadius;
        return;
    }

    // Follow the finger that started the drag, not whichever happens to be first in
    // the list -- a second thumb resting on the glass must not steer the control.
    for (const touchpad::Contact& contact : contacts) {
        if (contact.id != dragFinger_) continue;
        const float dx = contact.x - dragStartX_;
        const float dy = contact.y - dragStartY_;
        if (mode_ == Mode::Move) {
            applyDrag(geometry, dx, dy);
        } else {
            applyResize(dx, dy);
        }
        return;
    }
}

void LayoutScreen::update(double dt, const InputState& input) {
    (void)dt;
    if (messageTicks_ > 0) --messageTicks_;

    // The physical buttons still work for the things that are not a drag: Start
    // switches between moving and resizing, and the back button leaves.
    if (input.wasPressed(Button::kStart)) {
        mode_ = mode_ == Mode::Move ? Mode::Resize : Mode::Move;
        message_ = mode_ == Mode::Move ? "MOVE: DRAG TO PUT IT SOMEWHERE"
                                       : "RESIZE: DRAG UP DOWN, AND SIDE TO SIDE";
        messageTicks_ = 240;
    }
    if (input.wasPressed(Button::kB)) {
        if (engine() == nullptr) return;
        engine()->saveSettings();
        engine()->popScreen();
        return;
    }
}

void LayoutScreen::draw(Renderer& renderer) {
    if (engine() == nullptr) return;
    const float width = screenWidth();
    const float height = screenHeight();

    // A dark backdrop, so that the controls stand out against something other than a
    // fight. The pad is drawn over the game's screen in the game, and a layout chosen
    // against a bright stage is a layout chosen against the wrong thing.
    renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                      Color::rgba8(14, 16, 24));

    // The real pad, at full brightness and without the idle fade -- the player is
    // looking straight at it -- and with no buttons held, so nothing is lit.
    engine()->pad().draw(renderer, engine()->game().fontTable(true), 0u, width, height, 0.0,
                         false);

    // The selected control, picked out. Drawn in the pad's own space so that it lands
    // exactly on the thing it is marking.
    const touchpad::Resolved geometry =
        touchpad::resolve(engine()->pad().layout(), width, height);
    const Centre centre = centreOf(target_, geometry);
    const float radius = kGrabRadius * std::min(width, height) * 0.55f;
    renderer.pushPixelSpace(width, height);
    const Texture& disc = renderer.disc();
    const Rect whole{static_cast<float>(disc.width()), static_cast<float>(disc.height()), 0, 0};
    const Color ring = mode_ == Mode::Move ? Color::rgba8(120, 220, 255, 70)
                                           : Color::rgba8(255, 190, 90, 70);
    renderer.drawSprite(disc, centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f,
                        whole, ring);
    renderer.popSpace();

    // The editor's own two buttons. Drawn in the pad's space, because that is where
    // they are hit-tested and they have to look like something you can press.
    renderer.pushPixelSpace(width, height);
    {
        const Texture& buttonDisc = renderer.disc();
        const Rect whole{static_cast<float>(buttonDisc.width()),
                         static_cast<float>(buttonDisc.height()), 0, 0};
        const auto plate = [&](const Box& rect, Color face, Color rim) {
            renderer.fillRect(rect.x, rect.y, rect.w, rect.h, face);
            const float edge = std::max(1.0f, rect.h * 0.06f);
            renderer.fillRect(rect.x, rect.y, rect.w, edge, rim);
            renderer.fillRect(rect.x, rect.y + rect.h - edge, rect.w, edge, rim);
            renderer.fillRect(rect.x, rect.y, edge, rect.h, rim);
            renderer.fillRect(rect.x + rect.w - edge, rect.y, edge, rect.h, rim);
        };
        plate(resetRect(width, height), Color::rgba8(52, 40, 44, 210), Color::rgba8(220, 130, 140));
        plate(doneRect(width, height), Color::rgba8(30, 52, 44, 210), Color::rgba8(130, 220, 170));

        // The labels go in the same space as the plates. They have to: the reset button
        // sits in the letterbox bar, which is *outside* the game's own screen, so the
        // font -- which is a screenpack font, drawn in the game's coordinates -- cannot
        // reach it unless it is scaled up into this space, which is what the pad does
        // for its own labels for the same reason.
        if (Font* font = engine()->game().fontTable(false).get(1)) {
            const auto label = [&](const Box& box, const char* text, Color colour) {
                font->draw(renderer, box.x + box.w * 0.5f, box.y + box.h * 0.5f, text, 0,
                           TextAlign::Center, colour, std::max(1.0f, box.h / 70.0f));
            };
            label(resetRect(width, height), "RESET", Color::rgba8(240, 190, 195));
            label(doneRect(width, height), "DONE", Color::rgba8(200, 245, 220));
        }
        (void)buttonDisc;
        (void)whole;
    }
    renderer.popSpace();

    // The words, in the game's own space where the screenpack font lives.
    const FontTable fonts = engine()->game().fontTable(false);
    Font* font = fonts.get(1);
    if (font == nullptr) return;
    const float middleX = renderer.virtualWidth() * 0.5f;
    const float top = renderer.virtualHeight() * 0.14f;

    font->draw(renderer, middleX, top, "CONTROL LAYOUT", 0, TextAlign::Center,
               Color::rgba8(255, 255, 255));
    font->draw(renderer, middleX, top + 18.0f, mode_ == Mode::Move ? "MOVE" : "RESIZE", 0,
               TextAlign::Center, mode_ == Mode::Move ? Color::rgba8(120, 220, 255)
                                                      : Color::rgba8(255, 190, 90));
    if (messageTicks_ > 0) {
        font->draw(renderer, middleX, top + 34.0f, message_.c_str(), 0, TextAlign::Center,
                   Color::rgba8(150, 160, 180));
    }
    font->draw(renderer, middleX, renderer.virtualHeight() - 18.0f,
               "START: MOVE/RESIZE - B: SAVE AND BACK", 0, TextAlign::Center,
               Color::rgba8(150, 160, 180));
    font->draw(renderer, middleX, renderer.virtualHeight() - 34.0f,
               "DRAG WITH ONE FINGER - THE REST OF THE PAD IS YOURS", 0, TextAlign::Center,
               Color::rgba8(120, 130, 150));
}

}  // namespace af
