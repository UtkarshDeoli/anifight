// The drawing vocabulary every screenpack screen is written in.
//
// system.def and fight.def use one visual language: a background is a list of
// BgElements, and everything else is a DisplayElement that is either an
// animation, a sprite or a run of text. The title screen, the select screen and
// the fight HUD all come through here, so "how does MUGEN position a sprite" is
// answered in exactly one place.
#pragma once

#include <array>
#include <string_view>

#include "engine/anim.h"
#include "engine/defs.h"
#include "gfx/renderer.h"
#include "gfx/sprite_bank.h"

namespace af {

class Font;

// A file's font table, numbered the way MUGEN numbers it: 1..10. Slot 0 is
// unused, matching MUGEN, where font 0 means "no font".
struct FontTable {
    std::array<Font*, 11> slots{};

    [[nodiscard]] Font* get(int index) const {
        if (index < 1 || index > 10) return nullptr;
        return slots[static_cast<size_t>(index)];
    }
};

// Draws a sprite with its axis at (x, y).
//
// MUGEN positions every sprite by its axis rather than by a corner, which is
// what makes a character's feet land on the floor regardless of how its art is
// framed, and what lets a lifebar hang off the centre of the screen. With
// `facing` -1 the sprite mirrors *about its axis*, so the axis stays put and the
// art flips around it -- not around the sprite's centre.
//
// `palette` is the caller's palette opinion (a fighter's palno/PalFX); null
// leaves the sprite in the palette it was decoded with, which is right for
// everything except the fighters themselves.
//
// `flipV` mirrors the sprite *about its axis* the way `facing` mirrors it
// horizontally: the axis stays put and the art turns over around it, so the
// point the offsets are measured from does not move. It is MUGEN's
// `vfacing = -1` on an Explod, where the vertical mirror is an independent
// choice from which way the effect faces.
void drawSpriteAt(Renderer& renderer, SpriteBank& bank, int group, int image, float x, float y,
                  int facing = 1, float scaleX = 1.0f, float scaleY = 1.0f,
                  Color tint = Color{}, TransParams trans = {},
                  const PaletteBind* palette = nullptr, bool flipV = false);

// As above, but showing only `fraction` of the sprite, measured outward from the
// axis in the direction `signedSpan` points (a negative span means "leftward" in
// the sprite's own frame, before `facing` mirrors it).
//
// This is how a lifebar empties. The art covers the whole bar and the current
// amount is a crop of it, which is why `range.x` in fight.def is a pair of
// offsets from the bar's anchor rather than a width.
void drawSpriteFraction(Renderer& renderer, SpriteBank& bank, int group, int image, float x,
                        float y, float signedSpan, float fraction, int facing, float scaleX,
                        float scaleY, Color tint, TransParams trans = {});

// How a background's own coordinates land on the screen.
//
// A title or select background is drawn 1:1 with its top-left at the screen's
// top-left, and the default is exactly that. A stage is not: its coordinates are
// authored in the stage's `localcoord` space, in which (0,0) is the top-left of
// an often much larger screen, and `start` is measured from the top *centre* of
// it. `anchor` is the stage-space point `origin` stands for, so a point `p` is
// drawn at `origin + (p - anchor) * scale`. For a fight that means
// `anchor = (localcoord.x / 2, 0)` -- the top centre -- placed at the middle of
// the fight window, with `scale` from stageTranslation().
struct BackgroundPlacement {
    Vec2 origin;  // screen point that `anchor` maps to
    Vec2 anchor;  // stage-space point that maps to `origin`
    Vec2 scale{1.0f, 1.0f};  // stage units -> screen units
};

// Draws a whole background definition. `tick` drives `velocity` scrolling, so a
// background drifts at the same rate whatever the frame rate is.
//
// Layers are drawn in definition order, then layerno order, which is MUGEN's
// rule for stages; for a title or select background, where everything sits
// behind the menu, the two coincide.
//
// `camera` is the stage position the view is centred on, in screen units. A
// title or select background has no camera and leaves it at the origin, which is
// what makes the default the right one for every screen that is not the fight.
void drawBackground(Renderer& renderer, const BgDef& bg, SpriteBank& bank,
                    const AnimationSet& animations, long long tick, Vec2 camera = Vec2{},
                    const BackgroundPlacement& placement = {});

// Draws one DisplayElement with its offset origin at (baseX, baseY).
//
// `textOverride` is the value the caller wants the element to show, and how it
// is applied depends on the element's own text: MUGEN writes a banner as
// `win.text = "%s WINS"` and `round.default.text = "ROUND %i"`, so a placeholder
// is substituted in place, while an element with no text takes the value whole.
// `tint` multiplies whatever colour the element draws in.
void drawElement(Renderer& renderer, const DisplayElement& element, SpriteBank& bank,
                 const AnimationSet& animations, const FontTable& fonts, long long tick,
                 float baseX, float baseY, std::string_view textOverride = {},
                 Color tint = Color{});

// The size a DisplayElement's text would occupy, so a caller can lay something
// out relative to it. `text` is the same override drawElement() would be given,
// and is substituted the same way. Returns false when the element is not text or
// its font is missing.
bool measureText(const DisplayElement& element, const FontTable& fonts, std::string_view text,
                 int* outWidth, int* outHeight);

}  // namespace af
