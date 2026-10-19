// Watching what the entity layer asks the renderer to draw.
//
// A host test has no display and the sprite-bank shim's `drawSpriteAt` is a
// no-op, so a fact about drawing is a fact about the *call*: which sprite, at
// which point, mirrored which way. That is enough to pin the parts of an
// explod's draw that the state machine decides and nothing else does --
// `vfacing`, the vertical mirror -- and it is the only place they are visible,
// because the alternative is a real GL context and a human looking at a screen.
//
// Recording is opt-in and off by default, so a test that only steps a fighter
// keeps the shim's old no-op behaviour and its memory profile exactly. Turn it
// on with record(true), clear() between cases, then read recorded().
#pragma once

#include <cstdint>
#include <vector>

#include "gfx/renderer.h"

namespace af {
namespace draw_stub {

// One `drawSpriteAt` call, in the order its arguments are written there.
struct DrawCall {
    int group = 0;
    int image = 0;
    float x = 0.0f;
    float y = 0.0f;  // the sprite's axis, not its top-left
    int facing = 1;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    Color tint{};
    // The transparency the draw was asked for, as `drawSpriteAt` received it --
    // the blend mode and both alpha factors, since a test that cares about a
    // fade needs the pair and not just the mode.
    TransParams trans{};
    bool flipV = false;
};

// Start or stop recording.
void record(bool enabled);

// Note one call. A no-op unless recording is on, which is what keeps a test
// that never looks at the recording from paying for it.
void push(const DrawCall& call);

// The calls made since the last clear(), in call order.
const std::vector<DrawCall>& recorded();

void clear();

}  // namespace draw_stub
}  // namespace af
