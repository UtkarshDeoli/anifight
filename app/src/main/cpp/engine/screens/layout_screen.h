// The pad's layout editor: move, resize and reset the on-screen controls.
//
// PPSSPP ships the same screen for its own Android pad, and it is the half of that
// design that matters most. A configurable layout kept in a settings file is only
// usable by someone who will edit a text file; letting the player drag the controls
// where their thumbs want them is what makes the format a feature rather than an
// implementation detail.
//
// Everything here works on the layout that is already live -- the editor moves the
// real controls, so the preview is not a picture of the arrangement but the
// arrangement itself.
#pragma once

#include <string>
#include <vector>

#include "engine/screen.h"

namespace af {

class LayoutScreen : public Screen {
public:
    void enter() override;
    void update(double dt, const InputState& input) override;
    void touch(const std::vector<touchpad::Contact>& contacts) override;
    void draw(Renderer& renderer) override;

    // The pad is drawn by this screen rather than by the engine, at full brightness
    // and with the selected control picked out -- two things the engine's own pad
    // drawing has no reason to do.
    [[nodiscard]] bool wantsTouchPad() const override { return false; }
    [[nodiscard]] bool acceptsTouchInput() const override { return true; }

private:
    enum class Mode { Move, Resize };
    enum class Target { Attacks, Directions, Start };

    // The screen-space centre of a control, which is what a drag moves.
    struct Centre {
        float x;
        float y;
    };
    [[nodiscard]] Centre centreOf(Target target, const touchpad::Resolved& geometry) const;
    void applyDrag(const touchpad::Resolved& geometry, float dx, float dy);
    void applyResize(float dx, float dy);

    void selectNearest(float x, float y, float screenWidth, float screenHeight);

    // The editor's own two buttons, as fractions of the screen: the pad cannot be used
    // to drive the screen that is editing the pad, so the screen brings its own.
    // Named `Box` rather than `Rect` because the renderer already has a `Rect`, and it
    // means something quite different: a source rectangle within a texture.
    struct Box {
        float x, y, w, h;
    };
    [[nodiscard]] Box resetRect(float screenWidth, float screenHeight) const;
    [[nodiscard]] Box doneRect(float screenWidth, float screenHeight) const;
    [[nodiscard]] static bool inside(const Box& box, float x, float y);

    Mode mode_ = Mode::Move;
    Target target_ = Target::Attacks;
    bool dragging_ = false;
    int dragFinger_ = -1;
    float dragStartX_ = 0.0f;
    float dragStartY_ = 0.0f;
    // What the control was when the drag began, so a drag is applied to the starting
    // arrangement rather than accumulating its own rounding.
    touchpad::Placement dragStartPlacement_{};
    float dragStartSpacing_ = 1.0f;
    std::string message_;
    int messageTicks_ = 0;

    [[nodiscard]] float screenWidth() const;
    [[nodiscard]] float screenHeight() const;
};

}  // namespace af
