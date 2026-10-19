// How much of the corpus' trigger text does the expression parser actually read?
//
// Not part of run_all_tests.sh: this is an instrument, not an assertion. It walks
// every pack in local_chars, pulls the `triggerN = ...` lines and the
// expression-valued controller parameters out of each [State ...] block, and
// prints the ones the parser rejects together with their shape. It exits 0
// whether or not there are failures -- a character typo is not a regression --
// so it is safe to put in a loop.
//
// It exists because a trigger the parser cannot read is the worst kind of bug
// this engine can have: Expr::parse fails, the trigger evaluates false, and the
// controller silently never runs. Nothing throws, nothing logs, and the only
// symptom is a character that plays wrong. Counting the failures is therefore
// the cheapest way to find real parser gaps -- and the way to tell a gap apart
// from a typo in somebody's .cns.
//
// The first run of this probe is what motivated the precedence-table and
// redirect work in engine/expr.cpp: the corpus parse rate went from 87.5% to
// 99.99% (128832 expressions, 8 failures), and every one of the 8 survivors is
// an author typo rather than a syntax the engine is missing:
//
//   Velocity Y > 0       3 lines, local_chars/dbz_piccolo/piccolo.cns only.
//   velocity y > -1         The trigger is `Vel Y` -- 953 uses across 79 files
//   Position Y >= 0      2  -- and `Pos Y` has 1050 uses across 80 files. The
//                           misspellings appear in exactly one pack.
//   Power => 500         2  `=>` is not an operator; `>=`. The neighbouring
//                           controller in the same state writes `Power = 500`.
//   AnimElem = 2,        1  A trailing comma with nothing after it.
//
// Build (from the repo root):
//   clang++ -std=c++20 -O1 -Wall -Wextra -I app/src/main/cpp \
//     -o /tmp/probe_corpus_parse tools/hosttest/probe_corpus_parse.cpp \
//     app/src/main/cpp/engine/expr.cpp
//   /tmp/probe_corpus_parse [root]      # default: local_chars

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

#include "engine/expr.h"

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string trim(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Controller parameters whose right-hand side is an expression. A name-valued
// parameter is deliberately absent: `value = 5030, time` and `anim = F7` read a
// *name* in their first slot, not an expression, so counting them here would
// report failures the engine does not have.
bool isExpressionParam(const std::string& key) {
    static const char* const kKnown[] = {
        "x",      "y",      "zx",     "xy",     "vx",     "vy",     "time",   "value2",
        "p1x",    "p1y",    "p2x",    "p2y",    "freq",   "ampl",   "phase",  "decay",
        "dir",    "id",     "index",  "first",  "last",   "number", "size.x", "size.y",
        "pos",    "offset", "spacing", "bindtime", "velset", "facing", "sprpriority",
        "elem",   "persistent_count", "looptime", "xaccel", "yaccel", "zaccel",
        "xvel",   "yvel",   "xveladd", "yveladd", "damage", "pausetime",
    };
    return std::find_if(std::begin(kKnown), std::end(kKnown),
                        [&](const char* k) { return key == k; }) != std::end(kKnown);
}

std::vector<std::string> cnsFiles(const std::string& root) {
    std::vector<std::string> out;
    DIR* dir = opendir(root.c_str());
    if (dir == nullptr) return out;
    while (dirent* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (name.size() > 4 && name.compare(name.size() - 4, 4, ".cns") == 0) {
            out.push_back(root + "/" + name);
        }
    }
    closedir(dir);
    return out;
}

// Splits on commas that are not inside parentheses or brackets.
std::vector<std::string> splitTop(const std::string& text) {
    std::vector<std::string> out;
    int depth = 0;
    std::string current;
    for (char c : text) {
        if (c == '(' || c == '[') ++depth;
        if (c == ')' || c == ']') --depth;
        if (c == ',' && depth == 0) {
            out.push_back(trim(current));
            current.clear();
            continue;
        }
        current.push_back(c);
    }
    out.push_back(trim(current));
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "local_chars";

    std::vector<std::string> packs;
    DIR* dir = opendir(root.c_str());
    if (dir == nullptr) {
        // Nothing to measure. local_chars is gitignored, so a fresh clone has no
        // corpus at all and this is the normal case, not an error.
        std::printf("no corpus at %s -- nothing to measure\n", root.c_str());
        return 0;
    }
    while (dirent* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        struct stat info {};
        const std::string path = root + "/" + name;
        if (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) packs.push_back(path);
    }
    closedir(dir);
    std::sort(packs.begin(), packs.end());

    int expressions = 0, failed = 0;
    std::map<std::string, int> failures;
    for (const std::string& pack : packs) {
        for (const std::string& path : cnsFiles(pack)) {
            std::istringstream stream(readFile(path));
            std::string line;
            bool inState = false;
            while (std::getline(stream, line)) {
                const std::string stripped = trim(line);
                if (stripped.size() > 1 && stripped[0] == '[') {
                    inState = stripped.rfind("[State ", 0) == 0 ||
                              stripped.rfind("[state ", 0) == 0;
                    continue;
                }
                if (!inState) continue;
                const size_t eq = stripped.find('=');
                if (eq == std::string::npos) continue;
                const std::string key = trim(stripped.substr(0, eq));
                if (key.rfind("trigger", 0) != 0 && !isExpressionParam(key)) continue;
                std::string value = trim(stripped.substr(eq + 1));

                // The scanner treats `;` as a comment to end of line, so the probe
                // has to as well -- otherwise a comment containing commas reads as
                // a parameter list and is counted as a failure.
                const size_t semi = value.find(';');
                if (semi != std::string::npos) value = trim(value.substr(0, semi));
                if (value.empty()) continue;
                ++expressions;

                // A parameter that takes a *list* -- `pos = 0, 0`,
                // `velset = -2.4, 0` -- is several expressions separated by
                // commas, not one expression. Try the whole value first, then the
                // split, so the count reflects failures rather than the list
                // syntax the .cns format is written in.
                bool bad = !af::Expr::parse(value).valid();
                if (bad) {
                    bad = false;
                    for (const std::string& part : splitTop(value)) {
                        if (!af::Expr::parse(part).valid()) bad = true;
                    }
                }
                if (!bad) continue;
                ++failed;
                // Normalise the shape so the same mistake in twenty files counts
                // once: digits collapse, and the line is clipped.
                std::string shape = std::regex_replace(value, std::regex("[0-9]+"), "N");
                if (shape.size() > 70) shape = shape.substr(0, 70) + "...";
                ++failures[shape];
            }
        }
    }

    std::printf("%zu packs, %d expressions, %d failed to parse (%.2f%%)\n", packs.size(),
                expressions, failed,
                expressions == 0 ? 0.0 : 100.0 * failed / expressions);
    std::vector<std::pair<int, std::string>> ranked;
    for (const auto& entry : failures) ranked.emplace_back(entry.second, entry.first);
    std::sort(ranked.rbegin(), ranked.rend());
    for (size_t i = 0; i < ranked.size() && i < 40; ++i) {
        std::printf("%6d  %s\n", ranked[i].first, ranked[i].second.c_str());
    }
    return 0;
}
