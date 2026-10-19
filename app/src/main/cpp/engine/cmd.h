// The .cmd file: named input commands, and the history they are matched
// against.
//
// This is the half of the format that decides whether a fighter has two moves
// or thirty. A character's .cns defines every state it can be in, but almost
// none of them are reachable without a command to enter them: the .cmd is what
// says that `~D, DF, F, x` is a "QCF_x", and the .cns is what says that
// "QCF_x", from the standing state, means state 1000.
//
// The command strings are MUGEN's own notation:
//
//     B F U D        directions, B and F relative to the way the fighter faces
//     UB UF DB DF    diagonals, written as the two directions together
//     a .. z         buttons, lower case
//     +              pressed together, as in `a+b`
//     ,              pressed in order, as in `~D, DF, F, x`
//     ~              that element must be *released* at that point
//     $              that element must be *held* at that point
//     /              a tap: pressed and released
//     N$             held for at least N ticks, as in `60$D`
//     >              no other symbols between this element and the one before
//                    it, as in `D, >DF`
//
// Directions are held relative to facing, so the same command means "forward"
// whichever way the fighter is looking -- which is the whole reason a mirrored
// input works without the character knowing.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "engine/input.h"

namespace af {

// One symbol of a command, as a bit. Directions and buttons share the space.
namespace CmdKey {
constexpr uint32_t kUp = 1u << 0;
constexpr uint32_t kDown = 1u << 1;
constexpr uint32_t kBack = 1u << 2;
constexpr uint32_t kForward = 1u << 3;

// `a` is bit 4, `b` bit 5, ... `z` bit 29. Lower case only: MUGEN's rule is
// that upper case is a direction and lower case is a button, which is what
// makes `DB` and `b` different symbols rather than a case bug.
constexpr uint32_t kButtonBase = 1u << 4;
[[nodiscard]] constexpr uint32_t button(char c) {
    return (c >= 'a' && c <= 'z') ? (kButtonBase << (c - 'a')) : 0u;
}

constexpr uint32_t kDirections = kUp | kDown | kBack | kForward;
}  // namespace CmdKey

// One element of a command: the symbols in it, and how they must be held.
struct CmdStep {
    uint32_t symbols = 0;  // one or more, joined by `+` in the file
    bool release = false;  // `~`
    bool hold = false;     // `/`, or a count: the element must be held down
    int holdTicks = 0;     // the N in `N/D` or `N$D`
    // `>` before the element, as in `D, >DF`: between the tick that satisfies
    // the previous element and the tick that satisfies this one, the input
    // history must be silent -- any unrelated symbol in the gap breaks the
    // chain. Meaningless on the first element, where there is no predecessor.
    bool chained = false;
};

struct CommandDef {
    std::string name;
    std::vector<CmdStep> steps;
    // How many ticks the whole sequence may span. MUGEN's default is 15, which
    // is a quarter second at 60Hz -- long enough to roll a stick and short
    // enough that a stale motion does not fire much later.
    int time = 15;
};

// The per-tick history a command is matched against, newest last.
//
// The buffer holds raw screen directions and not back/forward, because a
// fighter can turn around mid-motion and the same stored tick then means the
// opposite. Facing is applied at match time instead.
class CommandBuffer {
public:
    // Ticks of history kept. Comfortably past any command `time` a real
    // character uses, and small enough to be a fixed cost.
    static constexpr int kCapacity = 120;

    void push(const InputState& input, int facing);
    void clear();

    // True when every step of `command` appears, in order, with the last one
    // satisfied on this tick.
    [[nodiscard]] bool match(const CommandDef& command) const;

    [[nodiscard]] int ticksRecorded() const { return count_; }

    // Raw symbol mask `ticksBack` ticks ago, 0 being the most recent push.
    // Out-of-range reads are zero, so a command with a long `time` against a
    // short history does not match rather than reading something stale.
    [[nodiscard]] uint32_t at(int ticksBack) const;

    // Whether `symbols` have been held on each of the last `ticks` ticks.
    [[nodiscard]] bool heldFor(uint32_t symbols, int ticks) const;
    // The same window closing on an older tick: the counted-hold of a mid-
    // sequence element is measured against where that element matched, not
    // against now.
    [[nodiscard]] bool heldForEndingAt(uint32_t symbols, int ticks, int endTicksBack) const;

private:
    uint32_t history_[kCapacity] = {};
    int head_ = 0;   // index of the most recent entry
    int count_ = 0;  // entries recorded, capped at kCapacity
};

// Turns one tick of input into a command symbol mask, resolving back and
// forward against the way the fighter is facing.
[[nodiscard]] uint32_t commandSymbols(const InputState& input, int facing);

// Parses one command string, as it appears to the right of `command =`.
// Returns an empty vector for a string this parser cannot make sense of, which
// leaves the command declared but never matching rather than matching wrongly.
[[nodiscard]] std::vector<CmdStep> parseCommandString(std::string_view text);

// Every command a character declares.
class CommandList {
public:
    // Reads the [Command] sections of a .cmd document. The document is parsed
    // here rather than passed in as text so the caller can hand the same
    // document to the state parser afterwards: a .cmd's [Statedef -1] is a
    // state definition like any other.
    void parse(std::string_view text);

    [[nodiscard]] const std::vector<CommandDef>& commands() const { return commands_; }
    [[nodiscard]] bool empty() const { return commands_.empty(); }

    // First command with this name, or nullptr. MUGEN lets a .cmd declare the
    // same name twice (the same move on two different inputs), and the first
    // is the one that defines the name.
    [[nodiscard]] const CommandDef* find(std::string_view name) const;

    // Whether any command with this name matches the buffer right now. This is
    // what `command = "name"` evaluates to.
    [[nodiscard]] bool triggered(std::string_view name, const CommandBuffer& buffer) const;

    [[nodiscard]] size_t size() const { return commands_.size(); }

private:
    std::vector<CommandDef> commands_;
};

}  // namespace af
