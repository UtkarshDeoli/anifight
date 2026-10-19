#include "engine/anim.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>

#include "core/log.h"

namespace af {

namespace {

constexpr std::string_view kBeginActionPrefix = "begin action";

// MUGEN's frame tuple has blank fields in the middle (`10,0, 0,0, 1, , S`), so
// this split must keep empties. splitCommas() deliberately drops them and is
// therefore wrong here.
std::vector<std::string> splitTuple(std::string_view text) {
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;) {
        const size_t comma = text.find(',', start);
        const std::string_view piece =
            (comma == std::string_view::npos) ? text.substr(start) : text.substr(start, comma - start);
        fields.emplace_back(trim(piece));
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return fields;
}

bool parseInt(std::string_view text, int& out) { return tryParseInt(text, out); }

bool parseFloat(std::string_view text, float& out) { return tryParseFloat(text, out); }

void parseFlip(std::string_view text, bool& flipH, bool& flipV) {
    for (const char c : text) {
        if (c == 'H' || c == 'h') flipH = true;
        else if (c == 'V' || c == 'v') flipV = true;
    }
}

// Blend codes, e.g. "A" (add), "S" (subtract), "A1" (shorthand for AS256D128),
// "AS128D256", "SA256D0". The leading letter picks the mode; any S<num>/D<num>
// pairs after it set the source and destination alphas, on MUGEN's 0..256 scale.
Blend parseBlend(std::string_view text, float& srcAlpha, float& dstAlpha) {
    srcAlpha = 1.0f;
    dstAlpha = 1.0f;
    if (text.empty()) return Blend::Normal;

    Blend blend = Blend::Normal;
    size_t i = 0;
    if (text[0] == 'A' || text[0] == 'a') {
        blend = Blend::Additive;
        i = 1;
    } else if (text[0] == 'S' || text[0] == 's') {
        blend = Blend::Subtractive;
        i = 1;
    }

    if (i + 1 == text.size() && text[i] == '1') {
        srcAlpha = 256.0f / 256.0f;
        dstAlpha = 128.0f / 256.0f;
        return blend;
    }

    while (i < text.size()) {
        const char c = text[i];
        if (c == 'S' || c == 's' || c == 'D' || c == 'd') {
            ++i;
            int value = 0;
            bool sawDigit = false;
            while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
                value = value * 10 + (text[i] - '0');
                ++i;
                sawDigit = true;
            }
            if (sawDigit) {
                const float scaled = static_cast<float>(std::clamp(value, 0, 256)) / 256.0f;
                if (c == 'S' || c == 's') srcAlpha = scaled;
                else dstAlpha = scaled;
            }
        } else {
            ++i;
        }
    }
    return blend;
}

std::optional<int> actionNumberFromSectionName(std::string_view name) {
    if (name.size() <= kBeginActionPrefix.size()) return std::nullopt;
    if (name.compare(0, kBeginActionPrefix.size(), kBeginActionPrefix) != 0) return std::nullopt;
    int value = 0;
    if (!parseInt(name.substr(kBeginActionPrefix.size()), value)) return std::nullopt;
    return value;
}

bool startsWith(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

// Parses "[Begin Action N]" sections out of an already-parsed document.
struct ActionBuilder {
    Animation animation;
    std::vector<CollisionBox> defaultAttack;
    std::vector<CollisionBox> defaultHurt;
    std::vector<CollisionBox> nextAttack;
    std::vector<CollisionBox> nextHurt;
    bool nextAttackDeclared = false;
    bool nextHurtDeclared = false;
    // True between a `Clsn*Default:` header and the next per-frame `Clsn:`
    // header. Boxes arriving while it holds belong to the defaults, which
    // every frame without its own declaration then shares.
    bool inDefaultAttack = false;
    bool inDefaultHurt = false;
    bool sawInterpolate = false;
    int copyFrom = -1;
};

bool parseCollisionBox(std::string_view value, CollisionBox& out) {
    const std::vector<std::string> parts = splitCommas(value);
    float numbers[4] = {};
    if (parts.size() < 4) return false;
    for (int i = 0; i < 4; ++i) {
        if (!parseFloat(parts[static_cast<size_t>(i)], numbers[i])) return false;
    }
    out = CollisionBox{numbers[0], numbers[1], numbers[2], numbers[3]};
    return true;
}

void finishFrame(ActionBuilder& builder) {
    // A frame's boxes are the ones declared for it; if it declared none, the
    // action's defaults apply. An explicit `Clsn1: 0` means "no boxes at all",
    // which is why the declared-flags exist rather than an emptiness check.
    builder.animation.attackBoxes.push_back(
        builder.nextAttackDeclared ? builder.nextAttack : builder.defaultAttack);
    builder.animation.hurtBoxes.push_back(
        builder.nextHurtDeclared ? builder.nextHurt : builder.defaultHurt);
    builder.nextAttack.clear();
    builder.nextHurt.clear();
    builder.nextAttackDeclared = false;
    builder.nextHurtDeclared = false;
}

ActionBuilder buildAction(const IniSection& section, int number) {
    ActionBuilder builder;
    builder.animation.number = number;

    for (const IniLine& line : section.lines) {
        const std::string_view raw = line.raw;
        std::string lower = toLower(raw);

        // `Loopstart` on its own line: looping resumes at the *next* element.
        // Without one, looping resumes at the first element, so the default of
        // zero is already right -- an action loops unless its last element holds
        // forever, which is the only thing MUGEN calls "no loop".
        if (lower == "loopstart") {
            builder.animation.loopStart = builder.animation.frames.size();
            continue;
        }

        if (startsWith(lower, "clsn")) {
            bool isAttack = false;

            if (startsWith(lower, "clsn1")) {
                isAttack = true;
            } else if (startsWith(lower, "clsn2")) {
                isAttack = false;
            } else {
                continue;
            }

            if (lower.find("default") != std::string::npos) {
                // `Clsn1Default: 2` -- the boxes that follow belong to every
                // frame of the action until a per-frame declaration says
                // otherwise.
                if (isAttack) {
                    builder.defaultAttack.clear();
                    builder.inDefaultAttack = true;
                } else {
                    builder.defaultHurt.clear();
                    builder.inDefaultHurt = true;
                }
                continue;
            }

            if (line.key.empty()) {
                // A declaration header such as `Clsn1: 2`, with no '=' at all.
                // This closes Default mode for its own channel only: once a
                // frame says what its boxes are, the defaults are only for the
                // frames that do not.
                if (isAttack) {
                    builder.nextAttackDeclared = true;
                    builder.nextAttack.clear();
                    builder.inDefaultAttack = false;
                } else {
                    builder.nextHurtDeclared = true;
                    builder.nextHurt.clear();
                    builder.inDefaultHurt = false;
                }
                continue;
            }

            // A box line: `Clsn2[0] = -10, 0, 10, -79` or `Clsn1 = ...`.
            CollisionBox box;
            if (!parseCollisionBox(line.value, box)) continue;

            // A Default-mode box line joins the defaults rather than opening a
            // per-frame block.
            if (isAttack && builder.inDefaultAttack) {
                builder.defaultAttack.push_back(box);
                continue;
            }
            if (!isAttack && builder.inDefaultHurt) {
                builder.defaultHurt.push_back(box);
                continue;
            }

            // A box line also opens a Next block if no header preceded it, which
            // some files rely on.
            if (isAttack) {
                if (!builder.nextAttackDeclared && builder.defaultAttack.empty()) {
                    builder.nextAttackDeclared = true;
                }
                builder.nextAttack.push_back(box);
            } else {
                if (!builder.nextHurtDeclared && builder.defaultHurt.empty()) {
                    builder.nextHurtDeclared = true;
                }
                builder.nextHurt.push_back(box);
            }
            continue;
        }

        if (startsWith(lower, "copy action")) {
            int source = -1;
            if (parseInt(trim(raw.substr(std::string_view("copy action").size())), source)) {
                builder.copyFrom = source;
            }
            continue;
        }

        if (startsWith(lower, "interpolate")) {
            // `Interpolate Offset`, `Interpolate Scale`, `Interpolate Angle`: lerp
            // those quantities from each frame toward the next over its duration.
            if (lower.find("scale") != std::string::npos) builder.animation.interpScale = true;
            else if (lower.find("angle") != std::string::npos) builder.animation.interpAngle = true;
            else builder.animation.interpOffset = true;
            builder.sawInterpolate = true;
            continue;
        }

        // Otherwise this should be a frame tuple.
        const std::vector<std::string> fields = splitTuple(raw);
        if (fields.size() < 5) {
            AF_LOGW("action %d line %d: unrecognised, ignoring: %s", number, line.lineNumber,
                    raw.data());
            continue;
        }

        AnimFrame frame;
        if (!parseInt(fields[0], frame.group) || !parseInt(fields[1], frame.image)) {
            AF_LOGW("action %d line %d: bad sprite id, ignoring: %s", number, line.lineNumber,
                    raw.data());
            continue;
        }
        parseFloat(fields[2], frame.offsetX);
        parseFloat(fields[3], frame.offsetY);
        if (!parseInt(fields[4], frame.duration)) frame.duration = 0;
        if (fields.size() > 5) parseFlip(fields[5], frame.flipH, frame.flipV);
        if (fields.size() > 6) {
            frame.blend = parseBlend(fields[6], frame.srcAlpha, frame.dstAlpha);
        }
        if (fields.size() > 7) parseFloat(fields[7], frame.scaleX);
        if (fields.size() > 8) parseFloat(fields[8], frame.scaleY);
        if (fields.size() > 9) parseFloat(fields[9], frame.angle);

        finishFrame(builder);
        builder.animation.frames.push_back(frame);
    }

    // A trailing Clsn block with no frame after it is meaningless; drop it.
    builder.nextAttack.clear();
    builder.nextHurt.clear();

    builder.animation.infinite = std::any_of(
        builder.animation.frames.begin(), builder.animation.frames.end(),
        [](const AnimFrame& f) { return f.duration == -1; });

    return builder;
}

}  // namespace

int Animation::frameAt(long long ticks) const {
    if (frames.empty()) return 0;
    if (ticks < 0) ticks = 0;

    const size_t loopBegin = std::min(loopStart, frames.size());

    // Only `-1` holds a frame forever. MUGEN treats every other negative
    // duration as zero ticks, so the frame is skipped rather than frozen on;
    // the old `< 0` test turned any negative into a hold.
    const auto effectiveDuration = [](int duration) {
        return duration == -1 ? -1 : std::max(0, duration);
    };

    // Frames before Loopstart play exactly once, then looping takes over.
    for (size_t i = 0; i < loopBegin; ++i) {
        const int duration = effectiveDuration(frames[i].duration);
        if (duration < 0) return static_cast<int>(i);  // holds forever
        if (ticks < duration) return static_cast<int>(i);
        ticks -= duration;
    }

    if (loopBegin >= frames.size()) {
        return static_cast<int>(frames.size() - 1);
    }

    long long loopTime = 0;
    for (size_t i = loopBegin; i < frames.size(); ++i) {
        const int duration = effectiveDuration(frames[i].duration);
        if (duration < 0) {
            // The loop itself holds forever: play through to the held frame.
            for (size_t j = loopBegin; j < frames.size(); ++j) {
                const int d = effectiveDuration(frames[j].duration);
                if (d < 0) return static_cast<int>(j);
                if (ticks < d) return static_cast<int>(j);
                ticks -= d;
            }
            return static_cast<int>(frames.size() - 1);
        }
        loopTime += duration;
    }

    if (loopTime <= 0) return static_cast<int>(loopBegin);

    ticks %= loopTime;
    for (size_t i = loopBegin; i < frames.size(); ++i) {
        const int duration = frames[i].duration;
        if (duration <= 0) continue;
        if (ticks < duration) return static_cast<int>(i);
        ticks -= duration;
    }
    return static_cast<int>(frames.size() - 1);
}

AnimFrame Animation::frameDrawn(long long ticks) const {
    const int index = frameAt(ticks);
    AnimFrame frame = frames[static_cast<size_t>(index)];
    if (frames.empty() || frame.duration <= 0 ||
        (!interpOffset && !interpScale && !interpAngle)) {
        return frame;
    }

    // How far into this frame we are: recompute the frame's start the same way
    // frameAt finds the frame, prefix once and the repeating loop after.
    const size_t loopBegin = std::min(loopStart, frames.size());
    long long prefix = 0;
    for (size_t i = 0; i < loopBegin; ++i) prefix += std::max(0, frames[i].duration);

    long long frameStart;
    if (static_cast<size_t>(index) < loopBegin || ticks < prefix) {
        frameStart = 0;
        for (int i = 0; i < index; ++i) frameStart += std::max(0, frames[static_cast<size_t>(i)].duration);
    } else {
        long long loopTime = 0;
        for (size_t i = loopBegin; i < frames.size(); ++i) {
            if (frames[i].duration == -1) { loopTime = -1; break; }
            loopTime += std::max(0, frames[i].duration);
        }
        if (loopTime <= 0) return frame;  // holds forever: nowhere to lerp toward
        // Ticks into the current loop pass, minus the frames before this one.
        long long elapsedThisFrame = (ticks - prefix) % loopTime;
        for (size_t i = loopBegin; i < static_cast<size_t>(index); ++i) {
            elapsedThisFrame -= std::max(0, frames[i].duration);
        }
        const float fraction =
            static_cast<float>(elapsedThisFrame) / static_cast<float>(frame.duration);
        // Past the last frame the loop wraps, so the next is the loop's first.
        const size_t next = static_cast<size_t>(index) + 1 < frames.size()
                                ? static_cast<size_t>(index) + 1
                                : loopBegin;
        const AnimFrame& nextFrame = frames[next];
        const float into = std::clamp(fraction, 0.0f, 1.0f);
        if (interpOffset) {
            frame.offsetX += (nextFrame.offsetX - frame.offsetX) * into;
            frame.offsetY += (nextFrame.offsetY - frame.offsetY) * into;
        }
        if (interpScale) {
            frame.scaleX += (nextFrame.scaleX - frame.scaleX) * into;
            frame.scaleY += (nextFrame.scaleY - frame.scaleY) * into;
        }
        if (interpAngle) frame.angle += (nextFrame.angle - frame.angle) * into;
        return frame;
    }

    // A once-only prefix frame: lerp toward the next frame, or hold at the last.
    if (static_cast<size_t>(index) + 1 >= frames.size()) return frame;
    const float fraction =
        static_cast<float>(ticks - frameStart) / static_cast<float>(frame.duration);
    const AnimFrame& nextFrame = frames[static_cast<size_t>(index) + 1];
    const float into = std::clamp(fraction, 0.0f, 1.0f);
    if (interpOffset) {
        frame.offsetX += (nextFrame.offsetX - frame.offsetX) * into;
        frame.offsetY += (nextFrame.offsetY - frame.offsetY) * into;
    }
    if (interpScale) {
        frame.scaleX += (nextFrame.scaleX - frame.scaleX) * into;
        frame.scaleY += (nextFrame.scaleY - frame.scaleY) * into;
    }
    if (interpAngle) frame.angle += (nextFrame.angle - frame.angle) * into;
    return frame;
}

void AnimationSet::add(Animation animation) {
    const int number = animation.number;
    animations_[number] = std::move(animation);
}

const Animation* AnimationSet::find(int number) const {
    const auto it = animations_.find(number);
    return it == animations_.end() ? nullptr : &it->second;
}

void parseAnimations(const IniFile& document, AnimationSet& out) {
    struct CopyRef {
        int number;
        int source;
    };
    std::vector<CopyRef> copies;

    for (const IniSection& section : document.sections()) {
        const std::optional<int> number = actionNumberFromSectionName(section.name);
        if (!number.has_value()) continue;

        ActionBuilder builder = buildAction(section, *number);
        if (builder.copyFrom >= 0) {
            copies.push_back(CopyRef{*number, builder.copyFrom});
        }
        out.add(std::move(builder.animation));
    }

    // `Copy Action n` aliases another action. Resolve iteratively so chains work
    // regardless of the order they appeared in the file.
    for (int pass = 0; pass < 4 && !copies.empty(); ++pass) {
        std::vector<CopyRef> unresolved;
        for (const CopyRef& ref : copies) {
            const Animation* source = out.find(ref.source);
            if (source == nullptr || source->frames.empty()) {
                unresolved.push_back(ref);
                continue;
            }
            Animation copy = *source;
            copy.number = ref.number;
            out.add(std::move(copy));
        }
        if (unresolved.size() == copies.size()) {
            for (const CopyRef& ref : unresolved) {
                AF_LOGW("action %d copies action %d, which does not exist", ref.number, ref.source);
            }
            break;
        }
        copies = std::move(unresolved);
    }
}

void parseAirText(std::string_view text, AnimationSet& out) {
    parseAnimations(IniFile::parse(text), out);
}

}  // namespace af
