#include "engine/screens/victory_screen.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "core/log.h"
#include "engine/character.h"
#include "engine/engine.h"
#include "engine/fighter.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "gfx/font.h"

namespace af {

namespace {

// Splits `text` into lines no wider than `maxWidth` font pixels, measured with
// the font that will draw it. This is what `winquote.textwrap = w` means; `n`
// never calls it. A single word longer than the line is left whole rather than
// broken, because breaking it would change the screenpack's words.
std::vector<std::string> wrapTextLines(const Font& font, const std::string& text, int bank,
                                       float maxWidth) {
    std::vector<std::string> lines;
    if (maxWidth <= 0.0f) {
        lines.push_back(text);
        return lines;
    }

    std::istringstream words(text);
    std::string word;
    std::string line;
    while (words >> word) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (line.empty() ||
            static_cast<float>(font.textWidth(candidate, bank)) <= maxWidth) {
            line = candidate;
        } else {
            lines.push_back(line);
            line = word;
        }
    }
    if (!line.empty()) lines.push_back(line);
    // An all-whitespace string still counts as one (empty) line, so the caller
    // draws nothing rather than silently reporting a layout it did not do.
    if (lines.empty()) lines.push_back(std::string());
    return lines;
}

}  // namespace

VictoryScreenView::VictoryScreenView(Game& game, Character* winner,
                                     const Fighter* winnerFighter, int matchNo)
    : game_(game),
      winner_(winner),
      winnerFighter_(winnerFighter),
      matchNo_(matchNo),
      timing_(game.system().victory.fadeInTime, game.system().victory.time,
              game.system().victory.fadeOutTime) {
    if (winner_ == nullptr) return;

    // The order is the reference's (SCREEN-FLOW-RE.md §2.4, §3.4): the
    // winner's VictoryQuote selection, otherwise a random quote from the
    // character's [Quotes], otherwise the screenpack's winquote.text. A
    // character that never ran the controller has no selection, and an
    // out-of-range or missing selection is treated the same way, so both end in
    // a random present quote.
    const VictoryQuoteSet& quotes = winner_->victoryQuotes();
    int selected = StateMachine::kVictoryQuoteRandom;
    std::uint32_t roll = static_cast<std::uint32_t>(matchNo_);
    if (winnerFighter_ != nullptr) {
        selected = winnerFighter_->machine().victoryQuoteIndex();
        // The card is built after the match, so drawing from the fight's own
        // stream cannot change how the match played. With no live fighter (a
        // test card, or a winner already torn down) the match number is a
        // stable seed rather than an uninitialised one.
        roll = static_cast<std::uint32_t>(winnerFighter_->machine().nextVictoryQuoteRoll());
    }
    quoteIndex_ = resolveVictoryQuoteIndex(quotes, selected, roll);
    if (quoteIndex_ >= 0) quoteText_ = quotes[static_cast<std::size_t>(quoteIndex_)];
}

void VictoryScreenView::enter() {
    const VictoryScreen& victory = game_.system().victory;
    AF_LOGI("victory: %s won match %d, %d ticks",
            winner_ != nullptr ? winner_->displayName().c_str() : "(none)", matchNo_,
            victory.time);
}

void VictoryScreenView::update(double dt, const InputState& input) {
    (void)dt;
    timing_.advance();

    // A tap or any button skips the card, but only once it is fully visible.
    // The press cannot leak into the fight waiting underneath: the engine
    // suppresses press edges when the top of the stack changes, and the fight
    // only updates after this screen has been popped.
    if (timing_.fadeInDone() && (input.touched || input.pressed != 0)) {
        if (engine() != nullptr) engine()->popScreen();
        return;
    }

    if (timing_.finished()) {
        if (engine() != nullptr) engine()->popScreen();
    }
}

void VictoryScreenView::draw(Renderer& renderer) {
    const SystemDef& system = game_.system();

    if (!system.victoryBg.elements.empty()) {
        drawBackground(renderer, system.victoryBg, game_.systemSprites(), system.animations,
                       timing_.tick());
    } else {
        // The screenpack has no victory background. MUGEN's clear colour would
        // be black here; drawing it explicitly keeps the card self-contained
        // instead of relying on whatever the frame was cleared with.
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                          Color{0.0f, 0.0f, 0.0f, 1.0f});
    }

    drawPortrait(renderer);
    drawName(renderer);
    drawWinQuote(renderer);

    // The same fade shape VersusScreen uses. It is drawn last so it covers the
    // portrait and the text too, not just the background.
    const float alpha = timing_.fadeAlpha();
    if (alpha > 0.0f) {
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                          Color{0.0f, 0.0f, 0.0f, alpha});
    }
}

void VictoryScreenView::drawPortrait(Renderer& renderer) {
    if (winner_ == nullptr) return;

    const DisplayElement& element = game_.system().victory.portrait;
    // `p1.window` is a clipping rectangle in the reference, not just a size
    // hint: it is what keeps a character's full-body fallback inside the card
    // and a screenpack's too-large portrait off the text.
    const bool clipped = element.window.has_value();
    if (clipped) {
        const Rect4& w = *element.window;
        renderer.pushClip(w[0], w[1], w[2], w[3]);
    }

    // The portrait is the character's own sprite, group 9000 image 2 (the
    // convention SCREEN-FLOW-RE.md records), so it comes out of the character's
    // bank rather than the screenpack's. The element says where and how big.
    const Sprite* portrait =
        element.sprite.has_value()
            ? winner_->sprites().find(element.sprite->first, element.sprite->second)
            : nullptr;
    const bool hasPortrait = portrait != nullptr && portrait->valid();
    if (hasPortrait) {
        drawSpriteAt(renderer, winner_->sprites(), element.sprite->first,
                     element.sprite->second, element.offset.x, element.offset.y,
                     element.facing, element.scale.x > 0.0f ? element.scale.x : 1.0f,
                     element.scale.y > 0.0f ? element.scale.y : 1.0f, Color{},
                     element.trans);
    } else {
        // A character packaged without the 9000,2 portrait must still show
        // something: an empty card next to a name reads as a broken screen.
        drawAnimationFallback(renderer, element);
    }

    if (clipped) renderer.popClip();
}

void VictoryScreenView::drawAnimationFallback(Renderer& renderer,
                                              const DisplayElement& element) {
    if (winnerFighter_ == nullptr) return;

    const StateMachine& machine = winnerFighter_->machine();
    const Animation* animation = machine.currentAnimation();
    if (animation == nullptr || animation->frames.empty()) {
        // No live frame (a fighter between states, or a match that never
        // started): the character's stand animation is the least surprising
        // substitute for a portrait.
        animation = winner_->animation(Fighter::kStandAnim);
    }
    if (animation == nullptr || animation->frames.empty()) return;

    const AnimFrame frame = animation->frameDrawn(machine.animTime());
    if (frame.blank()) return;
    const Sprite* sprite = winner_->sprites().find(frame.group, frame.image);
    if (sprite == nullptr || sprite->width <= 0 || sprite->height <= 0) return;

    // Fit the whole frame into the card. The screenpack's `p1.scale` was
    // authored for a 9000,2 portrait, not for a full-body frame, so using it
    // here would draw the fighter far outside the window.
    Rect4 box{};
    if (element.window) {
        box = *element.window;
    } else {
        const Vec2 local = game_.system().localCoord;
        const float width = local.x * 0.5f;
        const float height = local.y * 0.55f;
        box = Rect4{local.x * 0.5f - width * 0.5f, 0.0f, local.x * 0.5f + width * 0.5f,
                    height};
    }
    const float boxWidth = std::max(1.0f, box[2] - box[0]);
    const float boxHeight = std::max(1.0f, box[3] - box[1]);
    const float scale = std::min(boxWidth / static_cast<float>(sprite->width),
                                 boxHeight / static_cast<float>(sprite->height));

    // Centre the *art*, not the axis: a whole-body frame's axis is at its feet,
    // and centring the axis would push the body off the top of the card.
    // Facing is the fighter's, with the frame's own horizontal flip folded in,
    // exactly as Fighter::draw composes them; the frame's scale and vertical
    // flip are part of the pose too.
    const int facing = machine.facing() * (frame.flipH ? -1 : 1);
    const float frameScaleX = frame.scaleX > 0.0f ? frame.scaleX : 1.0f;
    const float frameScaleY = frame.scaleY > 0.0f ? frame.scaleY : 1.0f;
    const float scaleX = scale * frameScaleX;
    const float scaleY = scale * frameScaleY;
    const float centreX = (box[0] + box[2]) * 0.5f;
    const float centreY = (box[1] + box[3]) * 0.5f;
    const float left = centreX - static_cast<float>(sprite->width) * scaleX * 0.5f;
    const float top = centreY - static_cast<float>(sprite->height) * scaleY * 0.5f;
    const float axisX =
        facing < 0 ? left + static_cast<float>(sprite->width - sprite->axisX) * scaleX
                   : left + static_cast<float>(sprite->axisX) * scaleX;
    const float axisY = top + static_cast<float>(sprite->axisY) * scaleY;

    drawSpriteAt(renderer, winner_->sprites(), frame.group, frame.image, axisX, axisY, facing,
                 scaleX, scaleY, Color{}, frame.trans(), nullptr, frame.flipV);
}

void VictoryScreenView::drawName(Renderer& renderer) {
    const SystemDef& system = game_.system();
    const DisplayElement& element = system.victory.name;
    const FontTable fonts = game_.fontTable(false);
    const std::string winnerName =
        winner_ != nullptr ? winner_->displayName() : std::string("WINNER");

    // An element with neither text nor a usable font has no geometry of its
    // own. The reference's compiled default is font 3 right-aligned at 20,180,
    // but a screenpack this engine ships may only carry fonts 1 and 2, and a
    // font that failed to load draws nothing at all. Fall back to font 1,
    // centred and at the reference's documented name height, so the card always
    // names the winner it has.
    Font* font = element.font ? fonts.get(element.font->font) : nullptr;
    if (!element.present || font == nullptr) {
        Font* fallback = fonts.get(1);
        if (fallback == nullptr) return;
        const Vec2 local = system.localCoord;
        const std::string text =
            element.text.empty() ? winnerName
                                 : formatVictoryText(element.text, winnerName, matchNo_);
        fallback->draw(renderer, local.x * 0.5f, local.y * 0.75f, text, 0, TextAlign::Center,
                       Color::rgba8(255, 255, 255), 1.0f);
        return;
    }

    // The element is drawn through drawElement so its font, bank, alignment,
    // offset, scale and alpha keep the screenpack's meaning. The substitution
    // happens first because this screen has two different values; drawElement's
    // own override would write one value into both placeholders.
    DisplayElement resolved = element;
    resolved.text = element.text.empty() ? winnerName
                                         : formatVictoryText(element.text, winnerName, matchNo_);
    drawElement(renderer, resolved, game_.systemSprites(), system.animations, fonts,
                timing_.tick(), 0.0f, 0.0f);
}

void VictoryScreenView::drawWinQuote(Renderer& renderer) {
    const SystemDef& system = game_.system();
    const VictoryScreen& victory = system.victory;
    const DisplayElement& element = victory.winQuote;

    const FontTable fonts = game_.fontTable(false);
    const std::string winnerName =
        winner_ != nullptr ? winner_->displayName() : std::string("WINNER");
    // The character's quote is the primary source, and it is drawn verbatim:
    // the reference substitutes it into the screen's text slot as data, so a
    // `%` inside a quote is literal. `winquote.text` is used -- and formatted
    // with `%s`/`%i`/`%%` exactly as before -- only when the character wrote no
    // quote at all (SCREEN-FLOW-RE.md §2.4).
    const std::string text = victoryQuoteText(quoteText_, element.text, winnerName, matchNo_);
    if (text.empty()) return;

    Color tint{1.0f, 1.0f, 1.0f, 1.0f};
    if (element.alpha) tint.a *= std::clamp(element.alpha->x, 0.0f, 1.0f);

    Font* font = element.font ? fonts.get(element.font->font) : nullptr;
    if (font == nullptr) {
        // Same fallback as the name, at the quote's documented height.
        Font* fallback = fonts.get(1);
        if (fallback == nullptr) return;
        const Vec2 local = system.localCoord;
        fallback->draw(renderer, local.x * 0.5f, local.y * 0.80f, text, 0, TextAlign::Center,
                       tint, 1.0f);
        return;
    }

    const float scale = element.scale.x > 0.0f ? element.scale.x : 1.0f;
    const int bank = element.font->bank;
    const TextAlign align = textAlignFromInt(element.font->align);

    // `winquote.textwrap` is `w` (word wrap) or `n` (no wrap). The window is
    // the only width the file gives, so `w` wraps against it; `n` stays one
    // line even when it overflows, and `w` with no window has no boundary to
    // wrap against and is left whole rather than wrapped at an invented width.
    const bool wrap = !victory.winQuoteTextwrap.empty() &&
                      (victory.winQuoteTextwrap[0] == 'w' || victory.winQuoteTextwrap[0] == 'W');
    std::vector<std::string> lines;
    if (wrap && element.window && scale > 0.0f) {
        const Rect4& w = *element.window;
        const float maxWidth = (w[2] - w[0]) / scale;
        lines = wrapTextLines(*font, text, bank, maxWidth);
    } else {
        lines.push_back(text);
    }

    const bool clipped = element.window.has_value();
    if (clipped) {
        const Rect4& w = *element.window;
        renderer.pushClip(w[0], w[1], w[2], w[3]);
    }

    const float lineHeight = static_cast<float>(std::max(1, font->lineHeight())) * scale;
    for (size_t i = 0; i < lines.size(); ++i) {
        font->draw(renderer, element.offset.x,
                   element.offset.y + static_cast<float>(i) * lineHeight, lines[i], bank, align,
                   tint, scale);
    }

    if (clipped) renderer.popClip();
}

}  // namespace af
