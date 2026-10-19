// MUGEN text-file parsing.
//
// .def, .cfg, .cns, .cmd and .air are all the same dialect: an INI file with
// three section flavours. This parser is deliberately lossless-ish: it keeps
// every key in order and keeps duplicates, because MUGEN is order- and
// duplicate-sensitive in several places (e.g. repeated `[Begin Action]`
// blocks, repeated `anim` lines in select.def, and the `Clsn1:`/`Clsn2:`
// groups inside an action).
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace af {

struct IniLine {
    // The line as written, with any trailing comment removed and surrounding
    // space trimmed. AIR action frames (`0,0, 0,0, 4`) and collision lines
    // (`Clsn2[0] = -10, 0, 10, -79`) have no `key = value` shape at all, so the
    // raw text is the only thing that can represent them faithfully.
    std::string raw;
    std::string key;    // lower-cased; empty for a bare data line
    std::string value;  // everything after '=' with surrounding space trimmed
    int lineNumber = 0; // 1-based, for error messages
};

struct IniSection {
    std::string name;  // lower-cased, e.g. "begin action 0", "files", "info"
    std::vector<IniLine> lines;
    int lineNumber = 0;
};

// MUGEN's dialect is genuinely irregular, so the model is simply:
// an ordered list of sections, each holding ordered key/value lines.
class IniFile {
public:
    // Never throws; on unreadable input returns an empty document.
    static IniFile parse(std::string_view text);

    [[nodiscard]] bool empty() const { return sections_.empty(); }
    [[nodiscard]] const std::vector<IniSection>& sections() const { return sections_; }

    // First section whose name equals `name` (case-insensitive), or nullptr.
    [[nodiscard]] const IniSection* section(std::string_view name) const;

    // Every section whose name equals `name`, in file order.
    [[nodiscard]] std::vector<const IniSection*> sectionsNamed(std::string_view name) const;

    // First value for `key` within `sec`. Returns `fallback` if absent or blank.
    [[nodiscard]] static std::string_view get(
        const IniSection& sec, std::string_view key, std::string_view fallback = {});

    // Typed convenience readers built on get().
    [[nodiscard]] static int getInt(const IniSection& sec, std::string_view key, int fallback = 0);
    [[nodiscard]] static float getFloat(const IniSection& sec, std::string_view key, float fallback = 0.0f);
    [[nodiscard]] static bool getBool(const IniSection& sec, std::string_view key, bool fallback = false);

private:
    std::vector<IniSection> sections_;
};

// Splits "x = -1, 2, 3" style comma lists, trimming each element.
[[nodiscard]] std::vector<std::string> splitCommas(std::string_view value);

// Splits on runs of whitespace, dropping empties.
[[nodiscard]] std::vector<std::string> splitWhitespace(std::string_view value);

[[nodiscard]] std::string toLower(std::string_view s);
[[nodiscard]] std::string_view trim(std::string_view s);

// Lenient numeric parsing. MUGEN values routinely carry trailing junk
// ("0,1", "30 ", "+2"), so these accept a leading number and ignore the rest.
// They return false only when there is no number at all.
bool tryParseInt(std::string_view text, int& out);
bool tryParseFloat(std::string_view text, float& out);

// Removes one layer of surrounding double quotes, if present.
[[nodiscard]] std::string unquote(std::string_view text);

}  // namespace af
