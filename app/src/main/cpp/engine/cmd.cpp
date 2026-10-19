#include "engine/cmd.h"

#include <cctype>
#include <cstdlib>

#include "core/ini.h"

namespace af {

namespace {

// A tick of input, as symbols. Only the directions and buttons a command can
// name are kept. Start is addressable as the symbol `s`: the button bit
// already travels end to end (touch zone -> JNI bridge -> InputState), so the
// parser only had to be told the letter.
uint32_t symbolsFromHeld(uint32_t held, int facing) {
    uint32_t out = 0;
    if ((held & Button::kUp) != 0) out |= CmdKey::kUp;
    if ((held & Button::kDown) != 0) out |= CmdKey::kDown;

    // Back and forward are the same two physical directions, swapped by which
    // way the fighter is looking.
    const bool backIsLeft = facing >= 0;
    if ((held & Button::kLeft) != 0) out |= backIsLeft ? CmdKey::kBack : CmdKey::kForward;
    if ((held & Button::kRight) != 0) out |= backIsLeft ? CmdKey::kForward : CmdKey::kBack;

    const uint32_t attacks[6] = {Button::kX, Button::kY, Button::kZ,
                                 Button::kA, Button::kB, Button::kC};
    // MUGEN's button letters are fixed: x, y, z are the punches and a, b, c the
    // kicks, in that order. The engine's own masks are named the other way
    // round for the on-screen pad, so this is the one place the two orderings
    // have to be reconciled.
    const char letters[6] = {'x', 'y', 'z', 'a', 'b', 'c'};
    for (int i = 0; i < 6; ++i) {
        if ((held & attacks[i]) != 0) out |= CmdKey::button(letters[i]);
    }

    // `s` is Start. It sits in the same letter space as the attacks (bit 22),
    // so a .cmd may write `s`, `a+s`, `~s` and so on like any other button.
    if ((held & Button::kStart) != 0) out |= CmdKey::button('s');
    return out;
}

// A direction written as one or two upper-case letters: U, D, B, F, UB, UF,
// DB, DF. Returns 0 when the text is not a direction.
uint32_t directionSymbols(std::string_view text) {
    uint32_t out = 0;
    for (const char c : text) {
        switch (c) {
            case 'U': out |= CmdKey::kUp; break;
            case 'D': out |= CmdKey::kDown; break;
            case 'B': out |= CmdKey::kBack; break;
            case 'F': out |= CmdKey::kForward; break;
            default: return 0;
        }
    }
    return out;
}

}  // namespace

uint32_t commandSymbols(const InputState& input, int facing) {
    return symbolsFromHeld(input.held, facing);
}

std::vector<CmdStep> parseCommandString(std::string_view text) {
    std::vector<CmdStep> steps;

    // Elements are comma-separated. A trailing comma or an empty element is
    // tolerated: it is common in hand-written .cmd files and means nothing.
    size_t pos = 0;
    while (pos <= text.size()) {
        const size_t comma = text.find(',', pos);
        const size_t end = comma == std::string_view::npos ? text.size() : comma;
        const std::string_view element = trim(text.substr(pos, end - pos));
        pos = end + 1;

        if (!element.empty()) {
            CmdStep step;
            size_t i = 0;

            // Prefixes, in the order they appear in real files: `/`, then `~`,
            // then an optional hold count, then `$`, with `>` allowed anywhere
            // in the run. `/` and `~` can repeat and the count can appear
            // before or after the tilde, so these are peeled in a loop rather
            // than as a fixed sequence.
            //
            // `/` is the one that decides what the element *is*: it means "held
            // down", and an element without it has to be freshly pressed on the
            // tick it is tested.
            bool sawPrefix = true;
            while (i < element.size() && sawPrefix) {
                switch (element[i]) {
                    case '/':
                        step.hold = true;
                        ++i;
                        break;
                    case '~':
                        step.release = true;
                        ++i;
                        break;
                    case '>':
                        // Silence between this element and the previous one:
                        // any symbol at all in the gap breaks the command.
                        step.chained = true;
                        ++i;
                        break;
                    case '$':
                        // "Detect as 4-way": in a strict engine `$D` matches
                        // exactly D, DB or DF, while a plain `D` would need
                        // 8-way diagonals listed to catch the diagonals too.
                        // Here there is nothing to tighten: this matcher is
                        // inclusive by construction (an element matches when
                        // its bits are a subset of what is held), so the plain
                        // `D` already matches the diagonals and `$D` lands on
                        // the same set a strict `$D` would. Kept loose rather
                        // than strict on purpose: the strict form rejects
                        // nothing this parser would accept, and the subset
                        // reading is what every plain-direction command in the
                        // wild already relies on. Recorded rather than dropped
                        // so a reader can see it was considered.
                        ++i;
                        break;
                    default: {
                        if (std::isdigit(static_cast<unsigned char>(element[i]))) {
                            int value = 0;
                            while (i < element.size() &&
                                   std::isdigit(static_cast<unsigned char>(element[i]))) {
                                value = value * 10 + (element[i] - '0');
                                ++i;
                            }
                            step.holdTicks = value;
                            // A bare number is a hold duration, so it implies a
                            // hold even when the `/` is left off.
                            step.hold = true;
                        } else {
                            sawPrefix = false;
                        }
                        break;
                    }
                }
            }

            // The symbols, joined by `+` for simultaneous presses.
            const std::string_view symbols = trim(element.substr(i));
            size_t start = 0;
            while (start <= symbols.size()) {
                const size_t plus = symbols.find('+', start);
                const size_t stop = plus == std::string_view::npos ? symbols.size() : plus;
                const std::string_view one = trim(symbols.substr(start, stop - start));
                start = stop + 1;
                if (one.empty()) continue;

                if (one.size() <= 2 && std::isupper(static_cast<unsigned char>(one[0]))) {
                    const uint32_t dir = directionSymbols(one);
                    if (dir != 0) {
                        step.symbols |= dir;
                        continue;
                    }
                }
                if (one.size() == 1 && std::islower(static_cast<unsigned char>(one[0]))) {
                    step.symbols |= CmdKey::button(one[0]);
                    continue;
                }
                // An element this parser does not understand. Dropping the
                // whole step rather than the symbol keeps the command from
                // matching on a subsequence of itself, which would fire moves
                // at the wrong time -- worse than not firing them.
                step.symbols = 0;
                break;
            }

            if (step.symbols != 0) {
                steps.push_back(step);
            } else {
                return {};
            }
        }

        if (comma == std::string_view::npos) break;
    }

    return steps;
}

// ---------------------------------------------------------------- the buffer

void CommandBuffer::clear() {
    head_ = 0;
    count_ = 0;
    for (uint32_t& entry : history_) entry = 0;
}

void CommandBuffer::push(const InputState& input, int facing) {
    head_ = (head_ + 1) % kCapacity;
    history_[head_] = commandSymbols(input, facing);
    if (count_ < kCapacity) ++count_;
}

uint32_t CommandBuffer::at(int ticksBack) const {
    if (ticksBack < 0 || ticksBack >= count_ || ticksBack >= kCapacity) return 0;
    int index = head_ - ticksBack;
    while (index < 0) index += kCapacity;
    return history_[index];
}

bool CommandBuffer::heldFor(uint32_t symbols, int ticks) const {
    if (ticks <= 0) return true;
    if (ticks > count_) return false;
    for (int i = 0; i < ticks; ++i) {
        if ((at(i) & symbols) != symbols) return false;
    }
    return true;
}

bool CommandBuffer::heldForEndingAt(uint32_t symbols, int ticks, int endTicksBack) const {
    // The hold window closes at `endTicksBack`: the element satisfied by
    // counting held time *ends* there, and the ticks before it are the window.
    if (ticks <= 0) return true;
    if (endTicksBack + ticks > count_) return false;
    for (int i = 0; i < ticks; ++i) {
        if ((at(endTicksBack + i) & symbols) != symbols) return false;
    }
    return true;
}

namespace {

// Whether one step is satisfied by the tick `ticksBack` ago.
//
// The three hold styles are the whole of the matching rule:
//
//   release  the element must NOT be held. A motion opens with one of these
//            (`~D, DF, F, x`), and it is satisfied by the neutral tick before
//            the motion starts -- which is what "the stick was not already
//            down that way" means.
//   hold     the element must be held (`$D`, `60$D`).
//   press    the element must have just gone down: held now, not held on the
//            tick before. This is what makes a sequence a sequence, and it is
//            also how a diagonal registers when D is already held.
bool stepSatisfied(const CmdStep& step, const CommandBuffer& buffer, int ticksBack) {
    const uint32_t mask = buffer.at(ticksBack);

    if (step.release) {
        // `~N$D`: the direction was held for N ticks and let go -- the charge
        // move's charge. The count is checked first: it closes the tick *before*
        // the release tick, and plain "element up" would pass it with no charge
        // at all, which is how a one-frame tap used to fire a charged move.
        if (step.holdTicks > 0) {
            return (mask & step.symbols) == 0 &&
                   buffer.heldForEndingAt(step.symbols, step.holdTicks, ticksBack + 1);
        }
        // A release is satisfied both by the element being up and by it having
        // just come up, so a command may be written either way round.
        return (mask & step.symbols) == 0;
    }
    if (step.hold) {
        // The counted hold's window closes on the tick the element matched, not
        // on the newest tick -- the release ticks *after* the window are the
        // motion's tail and are not part of the count.
        if (step.holdTicks > 0) {
            return buffer.heldForEndingAt(step.symbols, step.holdTicks, ticksBack);
        }
        return (mask & step.symbols) == step.symbols;
    }
    // A press, which is also what a tap starts as.
    if ((mask & step.symbols) != step.symbols) return false;

    const uint32_t previous = buffer.at(ticksBack + 1);
    if ((step.symbols & ~CmdKey::kDirections) != 0) {
        // A button (or a direction+button element): the symbols must be new.
        return (previous & step.symbols) != step.symbols;
    }
    // A direction-only element is "pressed" when the held *direction set* changed
    // on the way in: that is the only reading under which the canonical roll
    // `D, DF, F, x` matches (DF is new when F joins, F is new when D leaves),
    // a double tap `F, F` still needs a real second tap (the set must change),
    // and a direction held since before the window never matches (the set has
    // not changed since it was pressed).
    return (previous & CmdKey::kDirections) != (mask & CmdKey::kDirections);
}

}  // namespace

bool CommandBuffer::match(const CommandDef& command) const {
    const int steps = static_cast<int>(command.steps.size());
    if (steps == 0) return false;

    // The last step has to land on this tick. Relaxing that to "within the last
    // few ticks" is the obvious way to add input buffering and the wrong one:
    // the same press would then match again on every following tick, firing the
    // move repeatedly, and characters guard against that with state checks that
    // half of them do not have.
    if (!stepSatisfied(command.steps[static_cast<size_t>(steps - 1)], *this, 0)) {
        return false;
    }

    // Then walk backwards looking for the earlier steps, most recent first.
    int wanted = steps - 2;
    if (wanted < 0) return true;

    const int limit = command.time > 0 ? command.time : 15;
    for (int back = 1; back <= limit && back < count_; ++back) {
        const CmdStep& step = command.steps[static_cast<size_t>(wanted)];
        if (stepSatisfied(step, *this, back)) {
            if (--wanted < 0) return true;
            continue;
        }

        // A `>` on the step just satisfied makes the gap sacred: the history
        // between it and the step before it may hold nothing at all. The tick
        // being tested here did not satisfy the wanted step, so any symbol in
        // it is an intervening input and the chain is broken -- fail at once,
        // because every older candidate only puts more of the gap behind it.
        //
        // Tested after stepSatisfied because the satisfying tick is part of
        // the element, not junk: a press step is satisfied precisely when its
        // symbols are held, so that tick is never empty by definition.
        //
        // Reads steps[wanted + 1], the step already matched against a newer
        // tick: `>` is written on the *later* element (`D, >DF`), and the
        // constraint is on what lies before it.
        if (command.steps[static_cast<size_t>(wanted + 1)].chained && at(back) != 0) {
            return false;
        }
    }
    return false;
}

// ------------------------------------------------------------- declarations

void CommandList::parse(std::string_view text) {
    commands_.clear();
    const IniFile document = IniFile::parse(text);

    // [Command] repeats once per move, so every one of them has to be read --
    // sectionsNamed() rather than section(), which would find only the first.
    for (const IniSection* section : document.sectionsNamed("command")) {
        CommandDef command;
        command.name = std::string(unquote(IniFile::get(*section, "name")));
        const std::string body = std::string(IniFile::get(*section, "command"));
        if (command.name.empty() || body.empty()) continue;

        const int time = IniFile::getInt(*section, "time", 15);
        command.time = time > 0 ? time : 15;
        command.steps = parseCommandString(body);
        if (command.steps.empty()) continue;

        commands_.push_back(std::move(command));
    }
}

const CommandDef* CommandList::find(std::string_view name) const {
    for (const CommandDef& command : commands_) {
        if (command.name == name) return &command;
    }
    return nullptr;
}

bool CommandList::triggered(std::string_view name, const CommandBuffer& buffer) const {
    // Every command sharing the name is tried, not just the first: a character
    // that binds one move to two different motions declares the name twice, and
    // taking only the first would make the second input dead.
    for (const CommandDef& command : commands_) {
        if (command.name == name && buffer.match(command)) return true;
    }
    return false;
}

}  // namespace af
