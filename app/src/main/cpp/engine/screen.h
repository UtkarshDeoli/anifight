#pragma once

#include <vector>

#include "engine/input.h"
#include "engine/touch_layout.h"

namespace af {

class Renderer;
class Engine;

// One full-screen state: title, character select, VS, the fight itself, ...
// MUGEN's screen flow is a plain stack, so this is too.
class Screen {
public:
    virtual ~Screen() = default;

    // Called once when the screen becomes the top of the stack.
    virtual void enter() {}
    // Called once when the screen is popped.
    virtual void leave() {}

    // `dt` is in seconds and is fixed at 1/60 by the engine, matching MUGEN's
    // frame counter, so screens can count frames by incrementing.
    virtual void update(double dt, const InputState& input) = 0;

    virtual void draw(Renderer& renderer) = 0;

    // Set by the engine just before enter(). Screens use it to move along the
    // stack; the engine defers those changes to the end of the tick, so calling
    // them from update() is safe.
    void setEngine(Engine* engine) { engine_ = engine; }

    // Whether the on-screen pad is drawn over this screen.
    //
    // On by default, and deliberately so: the pad is the controller, and a menu
    // you cannot navigate without a keyboard would leave the game unplayable on
    // the device it is written for. Screens with nothing to navigate can turn it
    // off, but every screen in this game has at least a cursor to move.
    [[nodiscard]] virtual bool wantsTouchPad() const { return true; }

    // Whether the pad still reports buttons on this screen.
    //
    // Separate from drawing it, because "do not cover my layout" and "ignore my
    // fingers" are different requests and a screen can want one without the
    // other -- a timed card wants to be dismissible by a tap without having a
    // controller drawn across it. Defaults to following the drawing, since a pad
    // you can see is a pad you expect to work.
    [[nodiscard]] virtual bool acceptsTouchInput() const { return wantsTouchPad(); }

    // Whether the pad should dim itself when nothing is being touched.
    //
    // Off by default, because a menu is navigated *by* the pad and a faded
    // controller is a controller a player cannot find. The fight screen turns it
    // on: there the pad is drawn over the stage, and dimming it is the only way
    // to keep the fighters' feet visible without moving the pad out of the game's
    // coordinate space entirely.
    [[nodiscard]] virtual bool fadesTouchPad() const { return false; }

    // The raw fingers, in device pixels, delivered every tick before update().
    //
    // Every screen but one wants the *buttons* a set of fingers means, which is what
    // update() is handed. The layout editor is the exception and cannot use them:
    // it is editing where the buttons are, so asking it to work through the buttons
    // would be circular -- moving a control would press it.
    virtual void touch(const std::vector<touchpad::Contact>& contacts) { (void)contacts; }

protected:
    [[nodiscard]] Engine* engine() const { return engine_; }

private:
    Engine* engine_ = nullptr;
};

}  // namespace af
