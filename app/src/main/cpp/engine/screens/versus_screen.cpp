#include "engine/screens/versus_screen.h"

#include <cstdio>
#include <utility>

#include "core/log.h"
#include "engine/character.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "gfx/font.h"

namespace af {

namespace {

// The VS screen's own layout is entirely in system.def, so the only numbers
// here are the ones that are not the screenpack's business.
}  // namespace

VersusScreen::VersusScreen(Game& game, Character* p1, Character* p2, int matchNumber)
    : game_(game), matchNumber_(matchNumber) {
    fighters_[0] = p1;
    fighters_[1] = p2;
}

void VersusScreen::enter() {
    const VsScreen& versus = game_.system().versus;
    AF_LOGI("versus: match %d, %s vs %s, %d ticks",
            matchNumber_,
            fighters_[0] != nullptr ? fighters_[0]->displayName().c_str() : "(none)",
            fighters_[1] != nullptr ? fighters_[1]->displayName().c_str() : "(none)",
            versus.time);
}

void VersusScreen::update(double dt, const InputState& input) {
    (void)dt;

    const VsScreen& versus = game_.system().versus;
    ++tick_;

    // Any button, or a tap anywhere on the glass, moves on. MUGEN's VS screen is
    // skippable, and a player who has seen it does not want to wait: the card
    // exists to say who is fighting, and it has said so by the time anyone can
    // read it.
    //
    // The press cannot leak to the fight screen waiting underneath. The engine
    // records what the pad reported last tick, so a button that is still held
    // when this screen leaves reads as already-down rather than as a fresh
    // press, and the fight's own update only runs once this screen has gone.
    if (input.touched || input.pressed != 0) {
        if (engine() != nullptr) engine()->popScreen();
        return;
    }

    // A screenpack that sets no time would otherwise make this screen a trap:
    // nothing else can move the stack, so there has to be a floor under it.
    const long long hold = versus.time > 0 ? versus.time : 150;
    const long long out = versus.fadeOutTime > 0 ? versus.fadeOutTime : 0;
    if (tick_ >= hold + out) {
        if (engine() != nullptr) engine()->popScreen();
    }
}

void VersusScreen::draw(Renderer& renderer) {
    const SystemDef& system = game_.system();
    const VsScreen& versus = system.versus;
    SpriteBank& sprites = game_.systemSprites();
    const FontTable fonts = game_.fontTable(false);

    drawBackground(renderer, system.versusBg, sprites, system.animations, tick_);

    // match.text is written as "MATCH %i", so the number goes through the same
    // substitution every other text element uses.
    if (versus.match.present) {
        char match[8];
        std::snprintf(match, sizeof(match), "%d", matchNumber_);
        drawElement(renderer, versus.match, sprites, system.animations, fonts, tick_, 0.0f, 0.0f,
                    match);
    }

    // The names are the fight screen's own fallbacks, so a match with no
    // characters on one side is introduced as exactly what it is.
    drawSide(renderer, versus.p1Sprite, versus.p1Name, fighters_[0], "PLAYER 1");
    drawSide(renderer, versus.p2Sprite, versus.p2Name, fighters_[1], "PLAYER 2");

    // Fade in, hold, fade out -- the same shape every other screen in the game
    // uses, driven by the screenpack's own numbers.
    const float alpha = [&]() {
        const long long in = versus.fadeInTime > 0 ? versus.fadeInTime : 0;
        if (tick_ < in) return 1.0f - static_cast<float>(tick_) / static_cast<float>(in);

        const long long hold = versus.time > 0 ? versus.time : 150;
        const long long out = versus.fadeOutTime > 0 ? versus.fadeOutTime : 0;
        if (out > 0 && tick_ >= hold) {
            const float t = static_cast<float>(tick_ - hold) / static_cast<float>(out);
            return t > 1.0f ? 1.0f : t;
        }
        return 0.0f;
    }();
    if (alpha > 0.0f) {
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                          Color{0.0f, 0.0f, 0.0f, alpha});
    }
}

void VersusScreen::drawSide(Renderer& renderer, const DisplayElement& portrait,
                            const DisplayElement& nameElement, Character* fighter,
                            const std::string& fallbackName) {
    if (portrait.present && fighter != nullptr && portrait.sprite.has_value()) {
        // The portrait is the character's own sprite -- group 9000 is MUGEN's
        // convention for it -- so it comes out of the character's bank rather
        // than the screenpack's. The element supplies where it goes and how big,
        // which is the whole of what a `[VS Screen]` p1.spr line means.
        drawSpriteAt(renderer, fighter->sprites(), portrait.sprite->first,
                     portrait.sprite->second, portrait.offset.x, portrait.offset.y,
                     portrait.facing, portrait.scale.x > 0.0f ? portrait.scale.x : 1.0f,
                     portrait.scale.y > 0.0f ? portrait.scale.y : 1.0f);
    }

    if (!nameElement.present) return;
    drawElement(renderer, nameElement, game_.systemSprites(), game_.system().animations,
                game_.fontTable(false), tick_, 0.0f, 0.0f,
                fighter != nullptr ? fighter->displayName() : fallbackName);
}

}  // namespace af
