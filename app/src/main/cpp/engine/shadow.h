// The shadow alpha rule, shared by the fighter and object draw paths.
//
// Kept apart from both draw sites because it is the one part of MUGEN's shadow
// model that is pure arithmetic rather than geometry, and because the reference
// applies exactly the same chain wherever a shadow is drawn (SHADOW-RE.md §3.3
// for players, §4.2 for objects):
//
//   base  = 256                     normally
//         = 256 - dst_alpha         when this tick's Trans is `addalpha`
//   ramp  = 0                     depth at/above `fade.range`'s top
//         = 1                     depth at/below its bottom
//         = linear in between
//   alpha = base * ramp * intensity / 256 / 256
//
// The disassembly settles the two points the notes originally left open
// (0x470F08-0x470FDE):
//
//   * intensity multiplies the alpha, at 0x470FBA (`imull 0x34(%esp),%ecx`,
//     `sarl $8`), after the addalpha substitution and the fade ramp; it is not
//     baked into the tint instead.
//   * the fade ramp *multiplies* the base alpha rather than replacing it: the
//     intermediate is computed as `(depth-top) * base / (bottom-top)` from the
//     saved base at 0x470F79-0x470F8B. So `addalpha` and `fade.range` compose
//     multiplicatively, which is what this function does.
//
// `depth` is in MUGEN's stage-y direction, where larger is lower on screen and
// the floor is 0; callers pass negated engine heights (see the draw sites).
// `addAlphaDst` is MUGEN's 0..256 destination alpha, or a negative value when
// this tick's Trans is anything other than `addalpha`.
#pragma once

#include <algorithm>
#include <cmath>

#include "engine/defs.h"

namespace af {

inline float shadowAlpha(float intensity, float fadeTop, float fadeBottom, float depth,
                         float addAlphaDst = -1.0f) {
    // The reference stores intensity unclamped and treats >= 256 as full in the
    // draw path; clamping the result reproduces that at both ends (a negative
    // intensity cannot brighten anything).
    const float base = addAlphaDst >= 0.0f ? std::clamp(256.0f - addAlphaDst, 0.0f, 256.0f)
                                           : 256.0f;

    float ramp = 1.0f;
    if (fadeTop < fadeBottom) {
        if (depth <= fadeTop) {
            ramp = 0.0f;
        } else if (depth >= fadeBottom) {
            ramp = 1.0f;
        } else {
            ramp = (depth - fadeTop) / (fadeBottom - fadeTop);
        }
    }

    const float alpha = base * ramp * intensity / 256.0f / 256.0f;
    return std::clamp(alpha, 0.0f, 1.0f);
}

inline float shadowAlpha(const ShadowStyle& style, float depth, float addAlphaDst = -1.0f) {
    return shadowAlpha(style.intensity, style.fadeTop, style.fadeBottom, depth, addAlphaDst);
}

// `[Reflection] intensity` is an alpha on MUGEN's 0..256 scale, not a shadow
// darkness: the reference draws the reflection only when it is > 0 and uses it
// directly for the mirrored blit (SHADOW-RE.md §0 fact 7, §3.2). It is a
// separate value from `[Shadow] intensity` and does not go through the fade or
// `addalpha` chain. Clamped at both ends for the same reason `shadowAlpha` is:
// the parser stores what the stage says, and a draw cannot use more than full.
inline float reflectionAlpha(const ShadowStyle& style) {
    return std::clamp(style.reflectionIntensity / 256.0f, 0.0f, 1.0f);
}

inline bool reflectionActive(const ShadowStyle& style) {
    return style.reflectionIntensity > 0.0f;
}

// Mirrors a sprite's axis y about the ground plane. Keeping this separate from
// the draw site makes the one geometric claim of the reflection checkable in a
// host test: a body axis `h` above the floor (`bodyY = floorY - h`) reflects to
// `floorY + h`.
inline float reflectionY(float floorY, float bodyY) { return floorY + (floorY - bodyY); }

// Whether an object's raw `shadow` triple asks for a shadow at all. `0,0,0` is
// the documented default and means none; every other value -- including the -1
// stage-colour sentinel -- means yes.
inline bool objectShadowEnabled(const ShadowRgb& rgb) { return !rgb.none(); }

// Resolves an object's raw triple to the colour to draw. `anyComponentSelectsStage`
// is the reference's asymmetry between Explod and Projectile: only Explod's
// `r == -1` selects the stage colour, while any Projectile component does
// (SHADOW-RE §4.4, §4.5). Components left at -1 without selecting the stage
// colour are clamped to 0, the closest reading of SDL_MapRGB's unsigned byte.
inline Color resolveObjectShadowColor(const ShadowRgb& rgb, const ShadowStyle& style,
                                      bool anyComponentSelectsStage) {
    const bool stage = anyComponentSelectsStage
                           ? (rgb.r == -1 || rgb.g == -1 || rgb.b == -1)
                           : (rgb.r == -1);
    if (stage) return style.color;
    return Color{static_cast<float>(std::clamp(rgb.r, 0, 255)) / 255.0f,
                 static_cast<float>(std::clamp(rgb.g, 0, 255)) / 255.0f,
                 static_cast<float>(std::clamp(rgb.b, 0, 255)) / 255.0f, 1.0f};
}

}  // namespace af
