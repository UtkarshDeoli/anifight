// The pure half of the Victory screen: its text substitution, its quote
// selection, and its clock.
//
// All three are separated from the screen itself because none needs a renderer,
// a character or a GL context. That is what lets the host suite pin the things
// a device test cannot see directly -- that `%s` and `%i` substitute to
// different values, that the character's `[Quotes]` are indexed the way MUGEN
// spells them, and that the fade/hold/fade-out arithmetic lands on the right
// tick -- without pretending to test drawing.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "core/ini.h"

namespace af {

// Formats one `[Victory Screen]` text element.
//
// The two placeholders are the ones SCREEN-FLOW-RE.md records for this screen:
// `%s` is the winning character's display name and `%i` the match number. They
// are substituted separately, which is why this is not motif.cpp's resolveText:
// that helper takes a single value and writes it into both placeholders, and
// `"%s - MATCH %i"` has to put a name in one and a number in the other.
//
// An empty format yields an empty string. The caller treats that as "draw
// nothing" for the win quote, and as "use the bare name" for the name element.
// `%%` is a literal percent sign.
std::string formatVictoryText(std::string_view format, std::string_view winnerName, int matchNo);

// ------------------------------------------------------------- win quotes
//
// The character-side half of the Victory screen. MUGEN keeps up to one hundred
// quotes per character in the constants file's `[Quotes]` section, spelled
// `victory0` .. `victory99` (sctrls.html's VictoryQuote notes; SCREEN-FLOW-RE.md
// §3.4). The section is indexed densely even when the author starts at
// `victory1`, as Kung Fu Man does, so a missing key is a missing index and not a
// shift.
//
// Only the English `[Quotes]` is read. MUGEN also accepts localised sections
// (`[ja.Quotes]`, `[es.Quotes]`, ...) selected by `language` in mugen.cfg; this
// engine has no language setting, so those sections are deliberately ignored
// rather than guessed at.
inline constexpr int kMaxVictoryQuotes = 100;
using VictoryQuoteSet = std::array<std::string, kMaxVictoryQuotes>;

// Reads every `victoryN` key of `[Quotes]` into `out`. Entries already in `out`
// survive, which is what lets a character's constants be split across the .cns
// and a state file that also carries `[Data]`: the later document adds what it
// has rather than erasing the earlier one. Out-of-range and non-numeric keys
// are skipped, and the stored value is unquoted exactly as the .def reader
// unquotes `name` and `displayname`.
void readVictoryQuotes(const IniFile& document, VictoryQuoteSet& out);

// Whether `index` names a quote the character actually wrote. A key written as
// an empty string is treated as absent, because there is nothing to display and
// falling back is the only useful answer.
[[nodiscard]] bool hasVictoryQuote(const VictoryQuoteSet& quotes, int index);
[[nodiscard]] int victoryQuoteCount(const VictoryQuoteSet& quotes);

// Maps a non-negative roll onto the (roll mod count)-th present quote, in
// ascending index order, and answers -1 when the character has no quotes.
// Taking the roll as a parameter keeps the selection seedable: the controller
// and the screen feed it their own generator, and the host tests can ask for an
// exact quote without owning a generator.
[[nodiscard]] int victoryQuoteIndexForRoll(const VictoryQuoteSet& quotes, std::uint32_t roll);

// The index the Victory screen should display. `selected` is the fighter's
// recorded VictoryQuote value: a present index is used as written; anything
// else -- the controller's -1 default, an out-of-range value, or an index the
// author left blank -- selects a random present quote. -1 when there is no
// quote at all, which sends the screen to `winquote.text`.
[[nodiscard]] int resolveVictoryQuoteIndex(const VictoryQuoteSet& quotes, int selected,
                                           std::uint32_t roll);

// The text the Victory screen draws for its quote line. A character quote is
// returned verbatim, never run through formatVictoryText: the reference hands
// the quote to the screen as the *value* of its text element, so a `%` inside
// it is the character author's literal character. Only when the character has
// no quote is the screenpack's `winquote.text` formatted, exactly as the card
// did before `[Quotes]` existed.
[[nodiscard]] std::string victoryQuoteText(std::string_view quote, std::string_view fallback,
                                           std::string_view winnerName, int matchNo);

// The Victory screen's fade-in / hold / fade-out clock.
//
// This mirrors VersusScreen's timing exactly (SCREEN-FLOW-RE.md's `[Victory
// Screen]` row), so the two cards behave alike: the screen fades in over
// `fadeInTime`, holds until `time`, fades out over `fadeOutTime`, and is
// finished on the tick `time + fadeOutTime`. A screenpack that sets no `time`
// gets MUGEN's 150-tick hold rather than a card that can never leave.
//
// `advance()` is called once per update, before anything reads the state, which
// is the same order VersusScreen uses. The initial state is "tick 0, fully
// black", so the card starts invisible and fades in.
class VictoryTiming {
public:
    VictoryTiming(int fadeInTime, int holdTime, int fadeOutTime);

    void advance() { ++tick_; }

    [[nodiscard]] long long tick() const { return tick_; }
    // A confirm may skip the card only once it is fully visible: skipping during
    // the fade-in would make the first tick of input dismiss something the
    // player has not seen yet.
    [[nodiscard]] bool fadeInDone() const { return tick_ >= fadeIn_; }
    [[nodiscard]] bool finished() const { return tick_ >= hold_ + fadeOut_; }

    // The alpha of the black overlay drawn over the card: 1 at the start of the
    // fade-in, 0 while the card is held, and 1 again at the end of the fade-out.
    [[nodiscard]] float fadeAlpha() const;

private:
    long long tick_ = 0;
    int fadeIn_ = 0;
    // The tick at which the fade-out starts. Normalised from the screenpack's
    // `time`; see the class comment.
    int hold_ = 150;
    int fadeOut_ = 0;
};

}  // namespace af
