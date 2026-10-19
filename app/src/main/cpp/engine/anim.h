// Animations.
//
// MUGEN stores animations in `.air` files, but `system.def` and `fight.def` also
// embed `[Begin Action N]` blocks verbatim, using the identical syntax. So this
// one parser serves the screenpack, and later the characters too.
//
// The frame tuple is:
//   Group, Image, OffsetX, OffsetY, Duration[, Flip][, Blend][, XScale, YScale[, Angle]]
#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/ini.h"
#include "gfx/renderer.h"

namespace af {

struct AnimFrame {
    int group = -1;
    int image = -1;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    // Duration in game ticks. -1 means "hold this frame forever", which is how
    // an animation is made non-looping. Any other negative value is treated as
    // zero ticks, matching MUGEN.
    int duration = 1;
    bool flipH = false;
    bool flipV = false;
    Blend blend = Blend::Normal;
    // The frame's colour-add code, e.g. `AS128D128`, as the pair MUGEN's
    // documentation describes it. Both are 1 for a frame that names no code,
    // which is what "draw the sprite as it is" means; `srcAlpha` is the `S`
    // number over 256 and `dstAlpha` the `D` number, and `A1` -- the format's
    // own shorthand for `AS256D128` -- is the one everybody writes.
    float srcAlpha = 1.0f;
    float dstAlpha = 1.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float angle = 0.0f;

    // The three fields above as the draw path wants them.
    [[nodiscard]] TransParams trans() const { return {blend, srcAlpha, dstAlpha}; }

    // Group or image of -1 draws nothing; the frame exists only to consume time.
    [[nodiscard]] bool blank() const { return group < 0 || image < 0; }
};

// A collision box in the owning file's coordinate space. MUGEN's axes differ
// between collision space and screen space: here +Y is up, so y2 is normally the
// more negative of the pair.
struct CollisionBox {
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
};

class Animation {
public:
    int number = 0;
    // Index of the frame that looping returns to. An action loops unless its
    // last element holds forever, so zero -- loop from the first element -- is
    // the right default rather than a placeholder: `LoopStart` only moves the
    // point the action resumes from, for an action whose opening elements play
    // once on entry.
    size_t loopStart = 0;
    // True when some frame has duration -1, i.e. the animation holds forever.
    bool infinite = false;
    std::vector<AnimFrame> frames;

    // Parallel to `frames`.
    std::vector<std::vector<CollisionBox>> attackBoxes;  // Clsn1
    std::vector<std::vector<CollisionBox>> hurtBoxes;    // Clsn2

    // `Interpolate Offset/Scale/Angle` on the action: between a frame and the
    // one after it, the drawn values are lerped over the frame's duration rather
    // than stepping -- the reason an interpolation-written action reads as a
    // smooth swing instead of a flipbook.
    bool interpOffset = false;
    bool interpScale = false;
    bool interpAngle = false;

    [[nodiscard]] bool empty() const { return frames.empty(); }

    // Which frame is showing after `ticks` ticks. Always a valid index.
    [[nodiscard]] int frameAt(long long ticks) const;

    // The frame at `ticks`, with the Interpolate flags applied. Callers that draw
    // want this; callers that test timing (`animelem`) want the raw index.
    [[nodiscard]] AnimFrame frameDrawn(long long ticks) const;
};

class AnimationSet {
public:
    void add(Animation animation);

    // nullptr when the screenpack does not define that action.
    [[nodiscard]] const Animation* find(int number) const;

    [[nodiscard]] size_t size() const { return animations_.size(); }
    [[nodiscard]] bool empty() const { return animations_.empty(); }

    [[nodiscard]] const std::map<int, Animation>& all() const { return animations_; }

private:
    std::map<int, Animation> animations_;
};

// Extracts every `[Begin Action N]` section from a parsed .def document.
// `Copy Action` references are resolved against the same document.
void parseAnimations(const IniFile& document, AnimationSet& out);

// Parses a standalone `.air` file (identical syntax, no [Begin Action] wrapper
// differences).
void parseAirText(std::string_view text, AnimationSet& out);

}  // namespace af
