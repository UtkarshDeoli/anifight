#include "engine/victory_flow.h"

#include <algorithm>

namespace af {

std::string formatVictoryText(std::string_view format, std::string_view winnerName, int matchNo) {
    if (format.empty()) return {};

    const std::string number = std::to_string(matchNo);
    std::string out;
    out.reserve(format.size() + winnerName.size() + number.size());

    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%' || i + 1 >= format.size()) {
            out += format[i];
            continue;
        }
        switch (format[i + 1]) {
            case 's':
                out += winnerName;
                ++i;
                break;
            case 'i':
                out += number;
                ++i;
                break;
            case '%':
                out += '%';
                ++i;
                break;
            default:
                // An unknown escape is left as written. MUGEN has no other
                // victory-screen placeholder, and eating the character would
                // silently change text the screenpack wrote.
                out += format[i];
                break;
        }
    }
    return out;
}

// ------------------------------------------------------------------ quotes

void readVictoryQuotes(const IniFile& document, VictoryQuoteSet& out) {
    // `IniFile::section` is case-insensitive, so this matches `[Quotes]` as
    // written. It deliberately does *not* match `[ja.Quotes]` and friends: the
    // section name there is "ja.quotes", a different record, and this engine has
    // no language selection to choose one.
    const IniSection* section = document.section("quotes");
    if (section == nullptr) return;

    static constexpr std::string_view kPrefix = "victory";
    for (const IniLine& line : section->lines) {
        if (line.key.size() <= kPrefix.size()) continue;
        if (line.key.compare(0, kPrefix.size(), kPrefix) != 0) continue;

        // The rest must be digits and nothing else. `tryParseInt` is lenient
        // ("12px" reads as 12) because character constants carry trailing junk;
        // a quote key does not, and accepting "victory1foo" would let a typo or
        // a differently spelled key overwrite a real quote.
        const std::string_view digits(line.key.data() + kPrefix.size(),
                                      line.key.size() - kPrefix.size());
        if (digits.size() > 3) continue;
        int index = 0;
        bool numeric = true;
        for (const char c : digits) {
            if (c < '0' || c > '9') {
                numeric = false;
                break;
            }
            index = index * 10 + (c - '0');
        }
        if (!numeric || index >= kMaxVictoryQuotes) continue;

        out[static_cast<size_t>(index)] = unquote(line.value);
    }
}

bool hasVictoryQuote(const VictoryQuoteSet& quotes, int index) {
    return index >= 0 && index < kMaxVictoryQuotes &&
           !quotes[static_cast<size_t>(index)].empty();
}

int victoryQuoteCount(const VictoryQuoteSet& quotes) {
    int count = 0;
    for (const std::string& quote : quotes) {
        if (!quote.empty()) ++count;
    }
    return count;
}

int victoryQuoteIndexForRoll(const VictoryQuoteSet& quotes, std::uint32_t roll) {
    const int count = victoryQuoteCount(quotes);
    if (count == 0) return -1;

    // `roll` is unsigned, so the modulo is well-defined even for a value a
    // caller cast from a negative. The scan runs in index order, which is what
    // makes the result reproducible for a given character and roll.
    int wanted = static_cast<int>(roll % static_cast<std::uint32_t>(count));
    for (int i = 0; i < kMaxVictoryQuotes; ++i) {
        if (quotes[static_cast<size_t>(i)].empty()) continue;
        if (wanted == 0) return i;
        --wanted;
    }
    return -1;
}

int resolveVictoryQuoteIndex(const VictoryQuoteSet& quotes, int selected,
                             std::uint32_t roll) {
    if (hasVictoryQuote(quotes, selected)) return selected;
    // The controller's -1 default, an out-of-range value, and an index the
    // author never wrote all mean the same thing here: the reference selects a
    // random quote when the requested index is not valid.
    return victoryQuoteIndexForRoll(quotes, roll);
}

std::string victoryQuoteText(std::string_view quote, std::string_view fallback,
                             std::string_view winnerName, int matchNo) {
    // A character quote is data, not a format. The reference substitutes it
    // into the screen's text slot, so `%s`, `%i` and `%%` inside a quote are
    // the author's literal characters and must not be expanded.
    if (!quote.empty()) return std::string(quote);
    if (fallback.empty()) return {};
    return formatVictoryText(fallback, winnerName, matchNo);
}

VictoryTiming::VictoryTiming(int fadeInTime, int holdTime, int fadeOutTime)
    : fadeIn_(std::max(0, fadeInTime)),
      hold_(holdTime > 0 ? holdTime : 150),
      fadeOut_(std::max(0, fadeOutTime)) {}

float VictoryTiming::fadeAlpha() const {
    if (fadeIn_ > 0 && tick_ < fadeIn_) {
        return 1.0f - static_cast<float>(tick_) / static_cast<float>(fadeIn_);
    }
    if (fadeOut_ > 0 && tick_ >= hold_) {
        const float t = static_cast<float>(tick_ - hold_) / static_cast<float>(fadeOut_);
        return t > 1.0f ? 1.0f : t;
    }
    return 0.0f;
}

}  // namespace af
