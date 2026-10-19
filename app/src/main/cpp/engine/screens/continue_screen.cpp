#include "engine/screens/continue_screen.h"

#include "core/log.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/motif.h"

namespace af {

namespace {

// Buttons that mean "yes" on this screen. It is the same set the title menu and
// the select screen confirm with: Start plus every attack button, because a pad
// wired for fighting has no dedicated confirm. B is an attack here (kB), not
// "back": there is nothing to go back to from a lost match.
bool confirmPressed(const InputState& input) {
    return input.wasPressed(Button::kStart) || input.wasPressed(Button::kA) ||
           input.wasPressed(Button::kB) || input.wasPressed(Button::kC) ||
           input.wasPressed(Button::kX) || input.wasPressed(Button::kY) ||
           input.wasPressed(Button::kZ);
}

// One of the five `[Continue Screen]` elements, with MUGEN's compiled defaults
// filled in when the screenpack does not define it.
//
// The shipped pack carries no `[Continue Screen]` section at all, and the
// reference's default motif is documented as "enabled but text-less"
// (SCREEN-FLOW-RE §2.6, §7). The values here are the ones §2.6 tabulates from
// the reference and its `big` motif: "CONTINUE?", "YES" and "NO", font 1 bank 0,
// with YES at -80,60 and NO at +80,60 from `pos`. An active variant is font 1
// bank 3. A screenpack that defines an element keeps every part of it --
// `drawElement` is what honours its offset, scale, alpha and alignment.
//
// The parser cannot distinguish an omitted offset from an explicit zero, so an
// element the file made drawable is taken whole; defaults only fill an element
// that would otherwise draw nothing. That is exactly the "section absent" case
// the shipped pack is in.
DisplayElement withDefault(DisplayElement element, const char* text, const Vec2& offset,
                           FontRef font) {
    if (element.drawsSomething()) return element;
    element.present = true;
    element.text = text;
    element.offset = offset;
    element.font = font;
    return element;
}

}  // namespace

ContinueScreenView::ContinueScreenView(Game& game, ContinueResult* result)
    : game_(game), result_(result) {}

void ContinueScreenView::enter() {
    AF_LOGI("continue: prompt up, YES selected, %d ticks (interpreted duration, not traced)",
            prompt_.ticksLeft());

    // The fight gives its window the device's aspect so the stage fills a phone.
    // This card, like every screenpack screen, draws in the screenpack's own
    // 320x240 space and is letterboxed. The fight clears its remembered window
    // width on the first update after this screen pops, so accepting a continue
    // restores the wide view before the next frame is drawn.
    if (engine() == nullptr) return;
    const Vec2 local = game_.system().localCoord;
    if (local.x > 0.0f && local.y > 0.0f) {
        engine()->renderer().setVirtualSize(local.x, local.y);
        engine()->renderer().setAspectMode(game_.config().aspect);
    }
}

void ContinueScreenView::update(double dt, const InputState& input) {
    (void)dt;
    ++tick_;

    // The clock runs before the input is read, so the tick that reaches zero is
    // already too late: ten seconds means ten seconds. Expiry is silent -- the
    // section has no timer element -- and follows the NO path.
    prompt_.advance();
    if (prompt_.expired()) {
        AF_LOGI("continue: countdown expired after %d ticks", kContinueCountdownTicks);
        decide(ContinuePrompt::timedOut());
        return;
    }

    // YES is the left block and NO the right one, as the default offsets say.
    if (input.wasPressed(Button::kLeft)) prompt_.moveSelection(-1);
    if (input.wasPressed(Button::kRight)) prompt_.moveSelection(+1);

    if (confirmPressed(input)) {
        decide(prompt_.chosen());
    }
}

void ContinueScreenView::decide(ContinueResult result) {
    if (decided_) return;
    decided_ = true;
    if (result_ != nullptr) *result_ = result;
    AF_LOGI("continue: %s",
            result == ContinueResult::Accepted ? "accepted -- resuming this match"
                                               : "declined -- ending the run");
    if (engine() != nullptr) engine()->popScreen();
}

void ContinueScreenView::draw(Renderer& renderer) {
    const SystemDef& system = game_.system();
    const ContinueScreen& screen = system.continueScreen;
    const FontTable fonts = game_.fontTable(false);

    // `[Continue Screen]` names no background section, and MUGEN's clear colour
    // for it is black.
    renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                      Color{0.0f, 0.0f, 0.0f, 1.0f});

    // The five blocks at the section's own `pos`. The inactive/active pair is
    // one option each: YES is active when selected and NO when not, and the
    // other way round.
    const DisplayElement prompt =
        withDefault(screen.prompt, "CONTINUE?", Vec2{0.0f, 0.0f}, FontRef{1, 0, 0});
    const DisplayElement yes =
        withDefault(screen.yes, "YES", Vec2{-80.0f, 60.0f}, FontRef{1, 0, 0});
    const DisplayElement yesActive =
        withDefault(screen.yesActive, "YES", Vec2{-80.0f, 60.0f}, FontRef{1, 3, 0});
    const DisplayElement no =
        withDefault(screen.no, "NO", Vec2{80.0f, 60.0f}, FontRef{1, 0, 0});
    const DisplayElement noActive =
        withDefault(screen.noActive, "NO", Vec2{80.0f, 60.0f}, FontRef{1, 3, 0});

    drawElement(renderer, prompt, game_.systemSprites(), system.animations, fonts, tick_,
                screen.pos.x, screen.pos.y);
    drawElement(renderer, prompt_.yesSelected() ? yesActive : yes, game_.systemSprites(),
                system.animations, fonts, tick_, screen.pos.x, screen.pos.y);
    drawElement(renderer, prompt_.yesSelected() ? no : noActive, game_.systemSprites(),
                system.animations, fonts, tick_, screen.pos.x, screen.pos.y);
}

}  // namespace af
