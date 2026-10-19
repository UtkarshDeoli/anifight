#include "engine/screens/win_screen.h"

#include <algorithm>

#include "core/log.h"
#include "engine/character.h"
#include "engine/engine.h"
#include "engine/fighter.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "gfx/font.h"

namespace af {

namespace {

// The parsed `wintext` element, or MUGEN's compiled defaults when the
// screenpack has no `[Win Screen]` section at all.
//
// The shipped motif is exactly that case: it defines `[Victory Screen]` but no
// Win section, so `SystemDef::win.winText` is an empty record while
// `win.enabled` keeps its compiled `true`. MUGEN's defaults for the omitted
// section are "Congratulations!" in font 2, centred at 159,70, persistent
// (SCREEN-FLOW-RE §2.5); without applying them here the card would show a pose
// and no words. The parser applies these defaults when the section *is*
// present, so this only fills the section-absent hole.
DisplayElement winTextElement(const SystemDef& system) {
    DisplayElement element = system.win.winText;
    if (element.drawsSomething()) return element;

    element.present = true;
    element.text = "Congratulations!";
    element.font = FontRef{2, 0, 0};
    element.offset = Vec2{159.0f, 70.0f};
    element.displaytime = -1;
    element.layerno = 2;
    return element;
}

}  // namespace

WinScreenView::WinScreenView(Game& game, Character* winner, const Fighter* winnerFighter)
    : game_(game),
      winner_(winner),
      winnerFighter_(winnerFighter),
      timing_(game.system().win.fadeInTime, game.system().win.poseTime,
              game.system().win.fadeOutTime) {}

void WinScreenView::enter() {
    const WinScreen& win = game_.system().win;
    AF_LOGI("win: %s cleared the run, %lld ticks (in %d, pose %d, out %d)",
            winner_ != nullptr ? winner_->displayName().c_str() : "(none)",
            timing_.totalTicks(), win.fadeInTime, win.poseTime, win.fadeOutTime);

    // The fight screen gives its window the device's aspect so the stage can
    // fill a phone. This card is a screenpack screen like the title or the VS
    // card, so it draws in the screenpack's own 320x240 space and is
    // letterboxed, exactly as the menus are. The fight restores its wide window
    // on the first update after this screen pops.
    if (engine() == nullptr) return;
    const Vec2 local = game_.system().localCoord;
    if (local.x > 0.0f && local.y > 0.0f) {
        engine()->renderer().setVirtualSize(local.x, local.y);
        engine()->renderer().setAspectMode(game_.config().aspect);
    }
}

void WinScreenView::update(double dt, const InputState& input) {
    (void)dt;
    (void)input;

    timing_.advance();
    // No skip. The reference's Win Screen has no input of its own, and this is
    // the one card that says the run is over; a stray press should not cut it
    // short. The engine keeps updating this screen until the clock runs out.
    if (timing_.finished()) {
        if (engine() != nullptr) engine()->popScreen();
    }
}

void WinScreenView::draw(Renderer& renderer) {
    // `[Win Screen]` names no background section in any shipped motif
    // (SCREEN-FLOW-RE §2.5), and MUGEN's clear colour for it is black.
    renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                      Color{0.0f, 0.0f, 0.0f, 1.0f});

    drawPose(renderer);
    drawWinText(renderer);

    // Fade last, so it covers the pose and the words as well as the background.
    const float alpha = timing_.fadeAlpha();
    if (alpha > 0.0f) {
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                          Color{0.0f, 0.0f, 0.0f, alpha});
    }
}

void WinScreenView::drawPose(Renderer& renderer) {
    if (winner_ == nullptr) return;

    // The reference draws the character's win pose. There is no forced state
    // 180 in this engine, so the winner is whatever frame it was frozen in at
    // the end of the match -- the honest equivalent, and the same source
    // VictoryScreenView falls back to when a character has no 9000,2 portrait.
    const Animation* animation = nullptr;
    AnimFrame frame;
    int facing = 1;
    if (winnerFighter_ != nullptr) {
        const StateMachine& machine = winnerFighter_->machine();
        animation = machine.currentAnimation();
        facing = machine.facing();
        if (animation == nullptr || animation->frames.empty()) {
            animation = winner_->animation(Fighter::kStandAnim);
        }
        if (animation == nullptr || animation->frames.empty()) return;
        frame = animation->frameDrawn(machine.animTime());
    } else {
        // No live fighter (a run cleared without one, which a real arcade run
        // cannot produce): the character's stand pose is the least surprising
        // substitute, as it is on the victory card.
        animation = winner_->animation(Fighter::kStandAnim);
        if (animation == nullptr || animation->frames.empty()) return;
        frame = animation->frameDrawn(0);
    }
    if (frame.blank()) return;

    const Sprite* sprite = winner_->sprites().find(frame.group, frame.image);
    if (sprite == nullptr || sprite->width <= 0 || sprite->height <= 0) return;

    // Fit the frame into the screenpack's canvas. A win pose is a full-body
    // frame centred on the card, not a portrait in a window: 72% of the width
    // and 82% of the height leaves it reading as a screen-wide pose rather than
    // a sprite pasted at its native size.
    const Vec2 local = game_.system().localCoord;
    const float canvasWidth = local.x > 0.0f ? local.x : renderer.virtualWidth();
    const float canvasHeight = local.y > 0.0f ? local.y : renderer.virtualHeight();
    const float boxWidth = canvasWidth * 0.72f;
    const float boxHeight = canvasHeight * 0.82f;
    const float scale = std::min(boxWidth / static_cast<float>(sprite->width),
                                 boxHeight / static_cast<float>(sprite->height));

    // Centre the *art*, not the axis. A whole-body frame's axis is at its feet;
    // centring that point would push the body off the top of the card, which is
    // the same reason the victory fallback centres the art box.
    const int drawFacing = facing * (frame.flipH ? -1 : 1);
    const float frameScaleX = frame.scaleX > 0.0f ? frame.scaleX : 1.0f;
    const float frameScaleY = frame.scaleY > 0.0f ? frame.scaleY : 1.0f;
    const float scaleX = scale * frameScaleX;
    const float scaleY = scale * frameScaleY;
    const float centreX = canvasWidth * 0.5f;
    const float centreY = canvasHeight * 0.5f;
    const float left = centreX - static_cast<float>(sprite->width) * scaleX * 0.5f;
    const float top = centreY - static_cast<float>(sprite->height) * scaleY * 0.5f;
    const float axisX =
        drawFacing < 0 ? left + static_cast<float>(sprite->width - sprite->axisX) * scaleX
                       : left + static_cast<float>(sprite->axisX) * scaleX;
    const float axisY = top + static_cast<float>(sprite->axisY) * scaleY;

    drawSpriteAt(renderer, winner_->sprites(), frame.group, frame.image, axisX, axisY,
                 drawFacing, scaleX, scaleY, Color{}, frame.trans(), nullptr, frame.flipV);
}

void WinScreenView::drawWinText(Renderer& renderer) {
    DisplayElement element = winTextElement(game_.system());
    if (!element.drawsSomething()) return;
    if (!winTextVisible(element.displaytime, timing_.tick())) return;

    const FontTable fonts = game_.fontTable(false);
    // A screenpack may name a font it did not ship. font 0 means "no font" in
    // MUGEN and 1 is the every-screenpack fallback, so the banner keeps its
    // words instead of silently disappearing, as the victory card does.
    if (element.font && fonts.get(element.font->font) == nullptr) {
        element.font->font = 1;
        if (fonts.get(element.font->font) == nullptr) return;
    }

    // The element's own offset, font, bank, alignment, scale and alpha are the
    // whole of what `wintext.*` means; drawElement is the one place those are
    // interpreted. The text is drawn verbatim: every shipped motif writes a
    // plain "Congratulations!" with no placeholder, and the reference's
    // substitution rule for this section was not traced, so this engine does
    // not invent one.
    drawElement(renderer, element, game_.systemSprites(), game_.system().animations, fonts,
                timing_.tick(), 0.0f, 0.0f);
}

}  // namespace af
