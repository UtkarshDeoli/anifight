// Checks the three pieces of the victory screen that a device cannot show you:
// the text substitution, the character `[Quotes]` parser and selection rules,
// and the fade/hold/fade-out clock.
//
// The drawing itself is deliberately not covered here. What is worth pinning is
// that `%s` and `%i` reach *different* values -- the screen's whole reason for
// having its own formatter rather than motif.cpp's single-value `resolveText` --
// and that the card leaves on the tick the screenpack's `time` and `fadeout.time`
// say it does. A card that never leaves would strand the arcade run.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_victory_flow \
//         tools/hosttest/test_victory_flow.cpp \
//         app/src/main/cpp/engine/victory_flow.cpp app/src/main/cpp/core/ini.cpp
//     /tmp/test_victory_flow

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

#include "core/ini.h"
#include "engine/victory_flow.h"

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

void checkText(const std::string& actual, const std::string& expected, const char* what) {
    if (actual == expected) return;
    std::fprintf(stderr, "FAIL: %s (expected \"%s\", got \"%s\")\n", what, expected.c_str(),
                 actual.c_str());
    ++failures;
}

void checkNear(float actual, float expected, const char* what) {
    if (std::fabs(actual - expected) < 1e-5f) return;
    std::fprintf(stderr, "FAIL: %s (expected %.4f, got %.4f)\n", what, expected, actual);
    ++failures;
}

}  // namespace

int main() {
    // ------------------------------------------------------------ substitution

    checkText(af::formatVictoryText("%s", "Kung Fu Man", 1), "Kung Fu Man",
              "%s is the winner's name");
    checkText(af::formatVictoryText("%i", "Kung Fu Man", 3), "3", "%i is the match number");
    checkText(af::formatVictoryText("%s - MATCH %i", "Kung Fu Man", 3),
              "Kung Fu Man - MATCH 3",
              "the two placeholders take their own values from one format");
    checkText(af::formatVictoryText("MATCH %i: %s wins", "Ryu", 12), "MATCH 12: Ryu wins",
              "placeholders work in any order and more than once");
    checkText(af::formatVictoryText("", "Ryu", 1), "",
              "an empty format stays empty, so a missing quote draws nothing");
    checkText(af::formatVictoryText("100%% %s", "Ryu", 1), "100% Ryu",
              "%% is a literal percent sign");
    checkText(af::formatVictoryText("%q", "Ryu", 1), "%q",
              "an unknown escape is left as written");
    checkText(af::formatVictoryText("ends with %", "Ryu", 1), "ends with %",
              "a trailing percent is left as written");
    checkText(af::formatVictoryText("no placeholders", "Ryu", 1), "no placeholders",
              "text with no placeholders is unchanged");

    // -------------------------------------------------------- [Quotes] parsing
    //
    // `[Quotes]` holds `victory0`..`victory99`, and characters use sparse
    // indices: KFM writes victory1..victory7 with no victory0. The parser is
    // therefore indexed, not a list, and every non-quote line in the section is
    // ignored rather than turning into a quote.
    {
        const af::IniFile document = af::IniFile::parse(R"(
[Quotes]
victory0 = "Zero, and 100% literal"
victory1 = One
victory3 = "Three"
victory99 = "Ninety-nine"
victory100 = "Past the last legal index"
victoryX = "Not a number"
victory = "No index at all"
[ja.Quotes]
victory2 = "Alternate language"
)");
        af::VictoryQuoteSet quotes{};
        af::readVictoryQuotes(document, quotes);

        check(af::victoryQuoteCount(quotes) == 4,
              "exactly the four legal victory0..victory99 keys are quotes");
        check(af::hasVictoryQuote(quotes, 0), "victory0 is quote 0");
        checkText(quotes[0], "Zero, and 100% literal", "quoted values lose one quote layer");
        checkText(quotes[1], "One", "unquoted values are read as written");
        checkText(quotes[3], "Three", "a sparse index keeps its own slot");
        checkText(quotes[99], "Ninety-nine", "victory99 is the last legal index");
        check(!af::hasVictoryQuote(quotes, 2),
              "a missing index is absent, so selection can skip it");
        check(!af::hasVictoryQuote(quotes, 100), "victory100 is past the last legal index");
        check(!af::hasVictoryQuote(quotes, -1), "no quote has a negative index");

        // `[ja.Quotes]` is a different section. Only the English `[Quotes]` is
        // read; the localization is documented as ignored.
        checkText(quotes[2], "", "an alternate-language section is not merged in");

        // Character::loadCns calls this for every state file that carries
        // [Data], so a later file without [Quotes] must not erase what an
        // earlier one loaded.
        af::readVictoryQuotes(af::IniFile::parse("[Data]\nlife = 1000\n"), quotes);
        check(af::victoryQuoteCount(quotes) == 4,
              "a document without [Quotes] must not clear existing quotes");
    }

    {
        af::VictoryQuoteSet empty{};
        af::readVictoryQuotes(af::IniFile::parse("[Quotes]\n"), empty);
        check(af::victoryQuoteCount(empty) == 0, "an empty [Quotes] section has no quotes");
        check(!af::hasVictoryQuote(empty, 0), "an empty section has no quote 0");
        check(af::victoryQuoteIndexForRoll(empty, 0) == -1,
              "no quote to roll means no index, not index 0");
        check(af::resolveVictoryQuoteIndex(empty, 3, 0) == -1,
              "an explicit selection cannot name a quote that does not exist");
    }

    // ------------------------------------------------- quote selection rules
    //
    // The controller stores an explicit index only when that quote exists;
    // everything else -- absent, -1, or out of range -- is the random marker.
    // The screen then rolls among the *present* indices, in index order, so the
    // random pick is uniform over quotes rather than over the 0..99 slots.
    {
        af::VictoryQuoteSet quotes{};
        af::readVictoryQuotes(af::IniFile::parse(
                                  "[Quotes]\nvictory0 = A\nvictory1 = B\n"
                                  "victory3 = C\nvictory99 = D\n"),
                              quotes);

        check(af::victoryQuoteIndexForRoll(quotes, 0) == 0,
              "roll 0 picks the first present quote");
        check(af::victoryQuoteIndexForRoll(quotes, 1) == 1,
              "roll 1 picks the second present quote");
        check(af::victoryQuoteIndexForRoll(quotes, 2) == 3,
              "the roll skips the absent index 2");
        check(af::victoryQuoteIndexForRoll(quotes, 3) == 99,
              "the last present index is reachable");
        check(af::victoryQuoteIndexForRoll(quotes, 4) == 0,
              "the roll wraps modulo the number of present quotes");
        check(af::victoryQuoteIndexForRoll(quotes, 1000) == 0,
              "a large roll is reduced modulo the quote count");

        // Every present quote must be reachable: a random pick that can never
        // choose one of them is not a random pick among existing quotes.
        bool reached[af::kMaxVictoryQuotes] = {};
        for (std::uint32_t roll = 0; roll < 4; ++roll) {
            reached[af::victoryQuoteIndexForRoll(quotes, roll)] = true;
        }
        check(reached[0] && reached[1] && reached[3] && reached[99],
              "rolling across one full period reaches every present quote");

        check(af::resolveVictoryQuoteIndex(quotes, 3, 1) == 3,
              "an explicit present index is kept, whatever the roll says");
        check(af::resolveVictoryQuoteIndex(quotes, 1, 0) == 1,
              "any present explicit index is kept");
        check(af::resolveVictoryQuoteIndex(quotes, 2, 2) == 3,
              "an explicit but missing index falls back to a random present quote");
        check(af::resolveVictoryQuoteIndex(quotes, 120, 1) == 1,
              "an out-of-range explicit index falls back to a random present quote");
        check(af::resolveVictoryQuoteIndex(quotes, -1, 3) == 99,
              "the -1 random marker falls back to a random present quote");
    }

    // ------------------------------------------------------------ quote text
    //
    // A character quote is data, not a format string: its `%` is literal. Only
    // the screenpack's `winquote.text` fallback goes through the substitution.
    checkText(af::victoryQuoteText("I win 100%!", "Winner!", "Ryu", 2), "I win 100%!",
              "a quote is drawn verbatim, so % inside it is literal");
    checkText(af::victoryQuoteText("", "%s wins match %i", "Ryu", 2), "Ryu wins match 2",
              "the screenpack fallback is formatted as before");
    checkText(af::victoryQuoteText("", "Winner!", "Ryu", 2), "Winner!",
              "a fallback with no placeholders is unchanged");
    checkText(af::victoryQuoteText("", "", "Ryu", 2), "",
              "no quote and no fallback draws nothing");

    // ------------------------------------------------------------------- clock
    //
    // The shipped card's shape: 8 ticks in, a 150-tick hold, 15 ticks out.
    constexpr int kIn = 8;
    constexpr int kHold = 150;
    constexpr int kOut = 15;

    af::VictoryTiming timing(kIn, kHold, kOut);
    check(timing.tick() == 0, "a fresh card starts at tick 0");
    check(!timing.fadeInDone(), "a confirm cannot skip the card before the first tick");
    check(!timing.finished(), "a fresh card is not finished");
    checkNear(timing.fadeAlpha(), 1.0f, "the card starts fully black");

    timing.advance();
    checkNear(timing.fadeAlpha(), 1.0f - 1.0f / static_cast<float>(kIn),
              "the fade-in lifts one step per tick");
    check(!timing.fadeInDone(), "the card is not visible until the fade-in ends");

    for (int i = 1; i < kIn; ++i) timing.advance();
    check(timing.tick() == kIn, "the fade-in ends on fadein.time");
    check(timing.fadeInDone(), "the card may be skipped once the fade-in is done");
    checkNear(timing.fadeAlpha(), 0.0f, "the card is fully visible after the fade-in");

    // Held, fully visible, for the whole hold.
    while (timing.tick() < kHold) timing.advance();
    check(!timing.finished(), "the card has not finished when the hold ends");
    checkNear(timing.fadeAlpha(), 0.0f, "the fade-out starts from fully visible");

    timing.advance();
    checkNear(timing.fadeAlpha(), 1.0f / static_cast<float>(kOut),
              "the fade-out darkens one step per tick");

    while (timing.tick() < kHold + kOut - 1) timing.advance();
    check(!timing.finished(), "the card is still up one tick before the end");
    checkNear(timing.fadeAlpha(), 1.0f - 1.0f / static_cast<float>(kOut),
              "the card is almost black on its last visible tick");

    timing.advance();
    check(timing.tick() == kHold + kOut, "the card finishes on hold + fadeout");
    check(timing.finished(), "the card finishes on hold + fadeout");
    checkNear(timing.fadeAlpha(), 1.0f, "the card is black when it finishes");

    // It must not overshoot if a slow frame calls advance again before the pop
    // is applied; finishing has to stay stable.
    timing.advance();
    check(timing.finished(), "the card stays finished after its end tick");
    checkNear(timing.fadeAlpha(), 1.0f, "the overlay does not go past fully black");

    // MUGEN's floor: `time = 0` means the documented 150-tick hold, not a card
    // that can never leave. Negative fades collapse to zero rather than invert.
    af::VictoryTiming floored(0, 0, 0);
    check(floored.fadeInDone(), "a zero-tick fade-in is done immediately");
    checkNear(floored.fadeAlpha(), 0.0f, "a zero-tick fade-in starts visible");
    for (int i = 0; i < 149; ++i) floored.advance();
    check(!floored.finished(), "the default hold is 150 ticks");
    floored.advance();
    check(floored.finished(), "the default hold ends at tick 150");

    af::VictoryTiming negative(-4, 10, -2);
    check(negative.fadeInDone(), "a negative fade-in is treated as zero");
    checkNear(negative.fadeAlpha(), 0.0f, "a negative fade-in leaves the card visible");

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}
