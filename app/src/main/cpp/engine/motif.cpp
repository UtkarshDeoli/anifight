#include "engine/motif.h"

#include <algorithm>
#include <cmath>

#include "gfx/font.h"

namespace af {

namespace {

// A tiled background on a small screen can want a lot of copies, but never an
// unbounded number. If a sprite somehow reports a zero width this is what stops
// the loop from never terminating.
constexpr int kMaxTiles = 128;

// Resolves an element's alpha modifier for text.
//
// Text is the one element that does not go through `TransParams`: a font draws
// its own quads with a tint and has no blend pair of its own, so the source
// half of a screenpack element's `alpha` is the only part it can honour. The
// sprite and animation paths pass the whole pair through `element.trans`, which
// is where the destination half matters.
Color applyAlpha(Color color, const std::optional<Vec2>& alpha) {
    if (alpha) color.a *= std::clamp(alpha->x, 0.0f, 1.0f);
    return color;
}

// The string an element actually draws, given the value its caller wants shown.
//
// MUGEN's text elements come in two shapes. A banner is a format -- "%s WINS",
// "ROUND %i", "MATCH %i" -- and the value goes into the placeholder. Everything
// else (the timer's digits, the powerbar counters, a fighter's name) has no text
// of its own in the .def, and takes the value as the whole string. Since both
// shapes arrive through the same call, the element's own text is what decides
// which one applies.
std::string resolveText(const DisplayElement& element, std::string_view value) {
    if (value.empty()) return element.text;
    if (element.text.empty()) return std::string(value);

    std::string text = element.text;
    for (const std::string_view placeholder : {"%s", "%i"}) {
        for (size_t at = text.find(placeholder); at != std::string::npos;
             at = text.find(placeholder, at + value.size())) {
            text.replace(at, placeholder.size(), value);
        }
    }
    return text;
}

void drawBgElement(Renderer& renderer, const BgElement& element, SpriteBank& bank,
                   const AnimationSet& animations, long long tick, Vec2 camera,
                   const BackgroundPlacement& placement) {
    // An animated background layer names an action instead of a sprite. MUGEN
    // animates it as a whole, so the frame's own offset is added to `start`.
    if (element.type == BgElement::Type::Anim) {
        if (element.actionNo < 0) return;
        const Animation* anim = animations.find(element.actionNo);
        if (anim == nullptr || anim->empty()) return;

        const AnimFrame& frame = anim->frames[static_cast<size_t>(anim->frameAt(tick))];
        if (frame.blank()) return;

        const float ticks = static_cast<float>(tick);
        // An animated layer's frame can carry its own trans; where it does, it
        // wins over the element's, exactly as a HUD animation's frame does.
        const TransParams trans = frame.blend != Blend::Normal ? frame.trans() : element.trans;
        // `start` is measured from the stage's top centre, which is the
        // placement's anchor, so it cancels against it. The frame's own AIR
        // offsets are stage pixels and take the same scale.
        drawSpriteAt(renderer, bank, frame.group, frame.image,
                     placement.origin.x +
                         (element.start.x + frame.offsetX) * placement.scale.x -
                         camera.x * element.delta.x +
                         element.velocity.x * ticks * placement.scale.x,
                     placement.origin.y +
                         (element.start.y + frame.offsetY) * placement.scale.y -
                         camera.y * element.delta.y +
                         element.velocity.y * ticks * placement.scale.y,
                     1, element.scaleStart.x * placement.scale.x,
                     element.scaleStart.y * placement.scale.y, Color{}, trans);
        return;
    }

    const Sprite* sprite = bank.find(element.sprite.first, element.sprite.second);
    if (sprite == nullptr) return;
    const Texture* texture = bank.texture(element.sprite.first, element.sprite.second);
    if (texture == nullptr) return;

    const float scaleX = element.scaleStart.x * placement.scale.x;
    const float scaleY = element.scaleStart.y * placement.scale.y;
    const float width = static_cast<float>(sprite->width) * scaleX;
    const float height = static_cast<float>(sprite->height) * scaleY;
    if (width <= 0.0f || height <= 0.0f) return;

    // `start` is where the layer sits when the camera is at the origin, and
    // `delta` is how much of the camera's movement the layer takes with it: 1
    // is pinned to the stage, 0 is pinned to the screen, and anything between
    // is a layer at some distance. A title or select background has no camera,
    // so it passes the origin and its layers simply sit where they are put.
    //
    // `camera` itself is already in screen units (see drawBackground), so only
    // the element's own stage-unit quantities take the placement scale.
    const float ticks = static_cast<float>(tick);
    float left = placement.origin.x + element.start.x * placement.scale.x -
                 camera.x * element.delta.x + element.velocity.x * ticks * placement.scale.x -
                 static_cast<float>(sprite->axisX) * scaleX;
    float top = placement.origin.y + element.start.y * placement.scale.y -
                camera.y * element.delta.y + element.velocity.y * ticks * placement.scale.y -
                static_cast<float>(sprite->axisY) * scaleY;

    // A tiled layer scrolls forever, so fold its position back into one tile's
    // worth of space before repeating; otherwise the offset would grow without
    // bound and the tiling would stop covering the screen.
    if (element.tile.x > 0.0f) {
        left = std::fmod(left, width);
        if (left > 0.0f) left -= width;
    }
    if (element.tile.y > 0.0f) {
        top = std::fmod(top, height);
        if (top > 0.0f) top -= height;
    }

    int columns = 1;
    int rows = 1;
    if (element.tile.x > 0.0f) {
        columns = static_cast<int>(std::ceil((renderer.virtualWidth() - left) / width));
    }
    if (element.tile.y > 0.0f) {
        rows = static_cast<int>(std::ceil((renderer.virtualHeight() - top) / height));
    }
    columns = std::clamp(columns, 1, kMaxTiles);
    rows = std::clamp(rows, 1, kMaxTiles);

    const Color tint = Color{};
    const Rect source{0.0f, 0.0f, static_cast<float>(sprite->width),
                      static_cast<float>(sprite->height)};

    const bool clipped = element.window.has_value();
    if (clipped) {
        // `window` is a rectangle in the stage's own screen coordinates, whose
        // (0,0) is the top-left of the authored screen -- hence `anchor`, not
        // `start`'s top-centre convention.
        const Rect4& w = *element.window;
        renderer.pushClip(placement.origin.x + (w[0] - placement.anchor.x) * placement.scale.x,
                          placement.origin.y + (w[1] - placement.anchor.y) * placement.scale.y,
                          placement.origin.x + (w[2] - placement.anchor.x) * placement.scale.x,
                          placement.origin.y + (w[3] - placement.anchor.y) * placement.scale.y);
    }

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            renderer.drawSprite(*texture, left + static_cast<float>(column) * width,
                                top + static_cast<float>(row) * height, width, height, source,
                                tint, false, false, element.trans);
        }
    }

    if (clipped) renderer.popClip();
}

}  // namespace

void drawSpriteAt(Renderer& renderer, SpriteBank& bank, int group, int image, float x, float y,
                  int facing, float scaleX, float scaleY, Color tint, TransParams trans,
                  const PaletteBind* palette, bool flipV) {
    const Sprite* sprite = bank.find(group, image);
    if (sprite == nullptr) return;
    const Texture* texture = bank.texture(group, image);
    if (texture == nullptr) return;

    const float width = static_cast<float>(sprite->width);
    const float height = static_cast<float>(sprite->height);

    // The axis stays at (x, y) whichever way the sprite faces, so mirroring
    // moves the top-left corner to the far side of the axis rather than to the
    // far side of the sprite. The vertical mirror is the same arithmetic on the
    // other axis: a sprite whose axis is not in its middle has to be moved by
    // the *rest* of its height when it turns over, not by its axis.
    float left = 0.0f;
    if (facing < 0) {
        left = x - (width - static_cast<float>(sprite->axisX)) * scaleX;
    } else {
        left = x - static_cast<float>(sprite->axisX) * scaleX;
    }
    const float top = flipV ? y - (height - static_cast<float>(sprite->axisY)) * scaleY
                            : y - static_cast<float>(sprite->axisY) * scaleY;

    renderer.drawSprite(*texture, left, top, width * scaleX, height * scaleY,
                        Rect{0.0f, 0.0f, width, height}, tint, facing < 0, flipV, trans, palette);
}

void drawSpriteFraction(Renderer& renderer, SpriteBank& bank, int group, int image, float x,
                        float y, float signedSpan, float fraction, int facing, float scaleX,
                        float scaleY, Color tint, TransParams trans) {
    if (fraction <= 0.0f) return;
    fraction = std::fmin(fraction, 1.0f);

    const Sprite* sprite = bank.find(group, image);
    if (sprite == nullptr || sprite->width <= 0) return;
    const Texture* texture = bank.texture(group, image);
    if (texture == nullptr) return;

    // The bar's far end is `signedSpan` away from the axis in the sprite's own
    // frame, and `facing` mirrors that direction along with the art.
    const float span = signedSpan * static_cast<float>(facing);
    const float anchor = static_cast<float>(sprite->axisX);
    const float visibleFar = anchor + span * fraction;
    const float lo = std::fmin(anchor, visibleFar);
    const float hi = std::fmax(anchor, visibleFar);
    if (hi - lo <= 0.0f) return;

    // Local pixel `p` is drawn at x - axisX + p when facing right and at
    // x + axisX - p when mirrored, so the two edges map differently.
    float edgeA = 0.0f;
    float edgeB = 0.0f;
    if (facing < 0) {
        edgeA = x + (anchor - lo) * scaleX;
        edgeB = x + (anchor - hi) * scaleX;
    } else {
        edgeA = x + (lo - anchor) * scaleX;
        edgeB = x + (hi - anchor) * scaleX;
    }

    const float left = std::fmin(edgeA, edgeB);
    const float width = std::fabs(edgeB - edgeA);
    const float top = y - static_cast<float>(sprite->axisY) * scaleY;
    const float height = static_cast<float>(sprite->height) * scaleY;

    renderer.drawSprite(*texture, left, top, width, height,
                        Rect{lo, 0.0f, hi - lo, static_cast<float>(sprite->height)}, tint,
                        facing < 0, false, trans);
}

void drawBackground(Renderer& renderer, const BgDef& bg, SpriteBank& bank,
                    const AnimationSet& animations, long long tick, Vec2 camera,
                    const BackgroundPlacement& placement) {
    // layerno 0 sits behind the screen's own content and 1 in front of it. For a
    // title or select background nothing is drawn between the two, so this is
    // just "definition order, low layers first"; for a stage it is what puts the
    // crowd in front of the fighters.
    for (int layer = 0; layer <= 1; ++layer) {
        for (const BgElement& element : bg.elements) {
            if (std::clamp(element.layerno, 0, 1) != layer) continue;
            drawBgElement(renderer, element, bank, animations, tick, camera, placement);
        }
    }
}

void drawElement(Renderer& renderer, const DisplayElement& element, SpriteBank& bank,
                 const AnimationSet& animations, const FontTable& fonts, long long tick,
                 float baseX, float baseY, std::string_view textOverride, Color tint) {
    if (!element.drawsSomething()) return;

    const float x = baseX + element.offset.x;
    const float y = baseY + element.offset.y;

    if (element.anim >= 0) {
        const Animation* anim = animations.find(element.anim);
        if (anim == nullptr || anim->empty()) return;

        const AnimFrame& frame = anim->frames[static_cast<size_t>(anim->frameAt(tick))];
        if (frame.blank()) return;

        // A frame can carry its own blend and scale; where it does, they combine
        // with the element's. The frame's pair is taken whole when it has one,
        // because its destination half belongs to the same decision as its mode.
        const TransParams trans = frame.blend != Blend::Normal ? frame.trans() : element.trans;
        drawSpriteAt(renderer, bank, frame.group, frame.image, x + frame.offsetX,
                     y + frame.offsetY, element.facing, element.scale.x * frame.scaleX,
                     element.scale.y * frame.scaleY, tint, trans);
        return;
    }

    if (element.sprite) {
        drawSpriteAt(renderer, bank, element.sprite->first, element.sprite->second, x, y,
                     element.facing, element.scale.x, element.scale.y, tint, element.trans);
        return;
    }

    if (element.font) {
        Font* font = fonts.get(element.font->font);
        if (font == nullptr) return;

        font->draw(renderer, x, y, resolveText(element, textOverride), element.font->bank,
                   textAlignFromInt(element.font->align), applyAlpha(tint, element.alpha),
                   element.scale.x);
    }
}

bool measureText(const DisplayElement& element, const FontTable& fonts, std::string_view text,
                 int* outWidth, int* outHeight) {
    if (!element.font) return false;
    Font* font = fonts.get(element.font->font);
    if (font == nullptr) return false;

    const std::string resolved = resolveText(element, text);
    if (outWidth != nullptr) *outWidth = font->textWidth(resolved, element.font->bank);
    if (outHeight != nullptr) *outHeight = font->lineHeight();
    return true;
}

}  // namespace af
