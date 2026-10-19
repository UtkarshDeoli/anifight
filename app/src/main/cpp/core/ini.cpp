#include "core/ini.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>

namespace af {

namespace {

// MUGEN files are ASCII/Latin-1 in practice. Strip a UTF-8 BOM if present so the
// first section header still parses.
std::string_view stripBom(std::string_view text) {
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        return text.substr(3);
    }
    return text;
}

bool isCommentStart(char c) {
    // MUGEN treats ';' as a comment.
    return c == ';';
}

// Removes a trailing comment, but not a ';' that sits inside a quoted string --
// `text = "K.O.; nice"` is a real value, not a value plus a comment.
std::string_view stripComment(std::string_view line) {
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (!inQuotes && isCommentStart(c)) {
            return trim(line.substr(0, i));
        }
    }
    return trim(line);
}

}  // namespace

std::string_view trim(std::string_view s) {
    size_t b = 0;
    size_t e = s.size();
    auto ws = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
    };
    while (b < e && ws(s[b])) ++b;
    while (e > b && ws(s[e - 1])) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

std::vector<std::string> splitCommas(std::string_view value) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= value.size()) {
        size_t comma = value.find(',', start);
        std::string_view piece =
            (comma == std::string_view::npos) ? value.substr(start) : value.substr(start, comma - start);
        std::string_view t = trim(piece);
        if (!t.empty()) out.emplace_back(t);
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return out;
}

std::vector<std::string> splitWhitespace(std::string_view value) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < value.size()) {
        while (i < value.size() && std::isspace(static_cast<unsigned char>(value[i]))) ++i;
        size_t start = i;
        while (i < value.size() && !std::isspace(static_cast<unsigned char>(value[i]))) ++i;
        if (i > start) out.emplace_back(value.substr(start, i - start));
    }
    return out;
}

IniFile IniFile::parse(std::string_view text) {
    IniFile doc;
    text = stripBom(text);

    IniSection* current = nullptr;
    int lineNumber = 0;
    size_t pos = 0;

    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string_view raw =
            (nl == std::string_view::npos) ? text.substr(pos) : text.substr(pos, nl - pos);
        pos = (nl == std::string_view::npos) ? text.size() + 1 : nl + 1;
        ++lineNumber;

        std::string_view line = stripComment(raw);
        if (line.empty()) continue;

        // A section header both opens and closes. Requiring the closing bracket
        // is what keeps a font's `[map]` readable: its records are keyed by the
        // literal character they define, so one of them is the line `[ 354 6`,
        // and reading that as a header would silently end the map at `Z` and
        // drop every glyph after it.
        if (line.front() == '[' && line.back() == ']') {
            doc.sections_.push_back(
                IniSection{toLower(trim(line.substr(1, line.size() - 2))), {}, lineNumber});
            current = &doc.sections_.back();
            continue;
        }

        // A key/value line outside any section: keep it in a synthetic section
        // so nothing is silently dropped.
        if (current == nullptr) {
            doc.sections_.push_back(IniSection{"", {}, lineNumber});
            current = &doc.sections_.back();
        }

        const size_t eq = line.find('=');
        if (eq == std::string_view::npos) {
            // A bare data line: an AIR action frame, or `Loopstart`.
            current->lines.push_back(IniLine{std::string(line), {}, {}, lineNumber});
        } else {
            std::string key = toLower(trim(line.substr(0, eq)));
            std::string value(trim(line.substr(eq + 1)));
            current->lines.push_back(
                IniLine{std::string(line), std::move(key), std::move(value), lineNumber});
        }
    }

    return doc;
}

const IniSection* IniFile::section(std::string_view name) const {
    for (const auto& s : sections_) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

std::vector<const IniSection*> IniFile::sectionsNamed(std::string_view name) const {
    std::vector<const IniSection*> out;
    for (const auto& s : sections_) {
        if (s.name == name) out.push_back(&s);
    }
    return out;
}

std::string_view IniFile::get(const IniSection& sec, std::string_view key, std::string_view fallback) {
    for (const auto& l : sec.lines) {
        if (l.key == key) {
            if (l.value.empty()) continue;  // MUGEN skips blank assignments
            return l.value;
        }
    }
    return fallback;
}

int IniFile::getInt(const IniSection& sec, std::string_view key, int fallback) {
    std::string_view v = get(sec, key);
    if (v.empty()) return fallback;
    // Values occasionally carry a trailing comment or unit; parse the leading integer.
    const char* begin = v.data();
    const char* end = v.data() + v.size();
    int result = 0;
    auto [ptr, ec] = std::from_chars(begin, end, result);
    if (ec != std::errc{}) return fallback;
    return result;
}

float IniFile::getFloat(const IniSection& sec, std::string_view key, float fallback) {
    std::string_view v = get(sec, key);
    if (v.empty()) return fallback;
    // from_chars for float is available on libc++ 20; fall back to strtof for safety.
    std::string tmp(v);
    char* endPtr = nullptr;
    float result = std::strtof(tmp.c_str(), &endPtr);
    if (endPtr == tmp.c_str()) return fallback;
    return result;
}

bool IniFile::getBool(const IniSection& sec, std::string_view key, bool fallback) {
    std::string_view v = get(sec, key);
    if (v.empty()) return fallback;
    std::string lower = toLower(v);
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") return true;
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") return false;
    return fallback;
}

bool tryParseInt(std::string_view text, int& out) {
    text = trim(text);
    if (text.empty()) return false;
    std::string tmp(text);
    char* end = nullptr;
    const long value = std::strtol(tmp.c_str(), &end, 10);
    if (end == tmp.c_str()) return false;
    out = static_cast<int>(value);
    return true;
}

bool tryParseFloat(std::string_view text, float& out) {
    text = trim(text);
    if (text.empty()) return false;
    std::string tmp(text);
    char* end = nullptr;
    const float value = std::strtof(tmp.c_str(), &end);
    if (end == tmp.c_str()) return false;
    out = value;
    return true;
}

std::string unquote(std::string_view text) {
    text = trim(text);
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        return std::string(text.substr(1, text.size() - 2));
    }
    return std::string(text);
}

}  // namespace af
