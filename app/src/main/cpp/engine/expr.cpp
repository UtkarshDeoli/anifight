#include "engine/expr.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <unordered_set>

namespace af {

namespace {

// ------------------------------------------------------------------ scanner

struct Token {
    enum class Type { End, Number, String, Ident, Op, LParen, RParen, Comma, LBracket, RBracket };
    Type type = Token::Type::End;
    std::string text;
    // The source spelling of an identifier, before case folding. Only the
    // `command = <name>` rule needs it, and it needs it badly: command names are
    // case-sensitive, and `command = QCF_x` written without quotes has to reach
    // the character's command list as `QCF_x` and not as `qcf_x`.
    std::string raw;
    float number = 0.0f;
};

bool isIdentStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool isIdentBody(char c) {
    // `.` is part of a name because MUGEN writes constants as `data.life` and
    // `Size.attack.dist`. It is never a decimal point in that position, since a
    // number can only start with a digit or a leading `.`.
    return isIdentStart(c) || (c >= '0' && c <= '9') || c == '.';
}
bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Values whose name is two words. The scanner glues the axis word onto one of
// these so that `pos y` arrives as a single name. Nothing else in the language
// puts two identifiers side by side, so the glue cannot misfire on an
// expression that means something else.
const std::unordered_set<std::string>& axisPrefixes() {
    static const std::unordered_set<std::string> kPrefixes = {
        "pos", "vel", "bodydist", "p2bodydist", "p2dist", "p1dist", "screenpos", "campos",
        "hitvel", "hitdist", "p2p1dist", "parentdist", "rootdist",
    };
    return kPrefixes;
}

bool isAxisWord(const std::string& s) {
    return s == "x" || s == "y" || s == "z" || s == "u" || s == "v";
}

std::string normaliseName(std::string_view raw) {
    std::string out;
    out.reserve(raw.size());
    bool lastSpace = false;
    for (char c : raw) {
        const char lowered = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        const bool space = (c == ' ' || c == '\t' || c == '_');
        if (space) {
            if (!out.empty() && !lastSpace) out.push_back(' ');
            lastSpace = true;
        } else {
            out.push_back(lowered);
            lastSpace = false;
        }
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

class Scanner {
public:
    explicit Scanner(std::string_view text) : text_(text) {}

    // Scans everything up front so the parser can look two tokens ahead (the
    // redirect rule needs exactly that much).
    bool run(std::vector<Token>& out) {
        while (true) {
            skipSpace();
            if (pos_ >= text_.size()) break;
            const char c = text_[pos_];

            if (isDigit(c) || (c == '.' && pos_ + 1 < text_.size() && isDigit(text_[pos_ + 1]))) {
                out.push_back(scanNumber());
                continue;
            }
            if (c == '"') {
                if (!scanString(out)) return false;
                continue;
            }
            if (isIdentStart(c)) {
                out.push_back(scanIdent());
                continue;
            }
            if (!scanOp(out)) return false;
        }
        out.push_back(Token{Token::Type::End, {}, {}, 0.0f});

        glueAxisWords(out);
        return true;
    }

private:
    void skipSpace() {
        // MUGEN comments run to end of line in every file that carries these
        // expressions, and a trailing comment is common on a trigger line.
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else if (c == ';') {
                while (pos_ < text_.size() && text_[pos_] != '\n') ++pos_;
            } else {
                break;
            }
        }
    }

    Token scanNumber() {
        const size_t start = pos_;
        while (pos_ < text_.size() && (isDigit(text_[pos_]) || text_[pos_] == '.')) ++pos_;
        Token t;
        t.type = Token::Type::Number;
        t.text = std::string(text_.substr(start, pos_ - start));
        t.number = std::strtof(t.text.c_str(), nullptr);
        return t;
    }

    bool scanString(std::vector<Token>& out) {
        ++pos_;  // opening quote
        std::string value;
        while (pos_ < text_.size() && text_[pos_] != '"') {
            value.push_back(text_[pos_]);
            ++pos_;
        }
        if (pos_ >= text_.size()) return false;  // unterminated
        ++pos_;                                  // closing quote
        Token t;
        t.type = Token::Type::String;
        t.text = std::move(value);
        out.push_back(std::move(t));
        return true;
    }

    Token scanIdent() {
        const size_t start = pos_;
        while (pos_ < text_.size() && isIdentBody(text_[pos_])) ++pos_;
        Token t;
        t.type = Token::Type::Ident;
        t.raw = std::string(text_.substr(start, pos_ - start));
        t.text = normaliseName(text_.substr(start, pos_ - start));
        return t;
    }

    bool scanOp(std::vector<Token>& out) {
        const char c = text_[pos_];
        const char next = pos_ + 1 < text_.size() ? text_[pos_ + 1] : '\0';

        auto two = [&](const char* s) {
            Token t;
            t.type = Token::Type::Op;
            t.text = s;
            out.push_back(std::move(t));
            pos_ += 2;
        };
        auto one = [&](char s) {
            Token t;
            t.type = Token::Type::Op;
            t.text = std::string(1, s);
            out.push_back(std::move(t));
            ++pos_;
        };
        // A one-character spelling of a two-character operator: `&` for the
        // bitwise and, `|` for the bitwise or, `^` for the bitwise xor. The
        // documentation lists all three alongside `&&`/`||`/`^^`, and character
        // files lean on them as if they were the logical ones --
        // `animelem = 3 | animelem = 5`, `statetype != A & ctrl` -- which is the
        // same answer whenever both operands are 0/1. Left unread they took the
        // whole trigger down, so the controller they guard never ran.
        auto alias = [&](const char* canonical) {
            Token t;
            t.type = Token::Type::Op;
            t.text = canonical;
            out.push_back(std::move(t));
            ++pos_;
        };

        switch (c) {
            case '&':
                if (next == '&') return two("&&"), true;
                return alias("&"), true;
            case '|':
                if (next == '|') return two("||"), true;
                return alias("|"), true;
            case '^':
                if (next == '^') return two("^^"), true;
                return alias("^"), true;
            case '~':
                return alias("~"), true;
            case '=':
                if (next == '=') return two("="), true;  // `==` is accepted as `=`
                return one('='), true;
            case '!':
                if (next == '=') return two("!="), true;
                return one('!'), true;
            case '<':
                if (next == '=') return two("<="), true;
                return one('<'), true;
            case '>':
                if (next == '=') return two(">="), true;
                return one('>'), true;
            case '+':
            case '-':
            case '/':
            case '%':
                return one(c), true;
            case '*':
                if (next == '*') return two("**"), true;  // power, above `*`
                return one('*'), true;
            case ':':
                if (next == '=') return two(":="), true;
                return false;
            case '(':
                one('(');
                out.back().type = Token::Type::LParen;
                return true;
            case ')':
                one(')');
                out.back().type = Token::Type::RParen;
                return true;
            case ',':
                one(',');
                out.back().type = Token::Type::Comma;
                return true;
            // `[` and `]` appear only as the ends of an interval, which is a
            // comparison spelling rather than a group: `PrevStateNo = [200,650]`.
            // They are tokens of their own because `(` and `)` are already a
            // group and the interval parser has to be able to tell the two apart.
            case '[':
                one('[');
                out.back().type = Token::Type::LBracket;
                return true;
            case ']':
                one(']');
                out.back().type = Token::Type::RBracket;
                return true;
            default:
                return false;
        }
    }

    // Folds `pos` + `x` into `pos x`. Done as a pass over the finished token
    // list rather than in the scanner so the lookahead is trivial.
    static void glueAxisWords(std::vector<Token>& tokens) {
        std::vector<Token> merged;
        merged.reserve(tokens.size());
        for (size_t i = 0; i < tokens.size(); ++i) {
            Token& cur = tokens[i];
            if (cur.type == Token::Type::Ident && axisPrefixes().count(cur.text) != 0 &&
                i + 1 < tokens.size() && tokens[i + 1].type == Token::Type::Ident &&
                isAxisWord(tokens[i + 1].text)) {
                cur.text = cur.text + " " + tokens[i + 1].text;
                merged.push_back(cur);
                ++i;
                continue;
            }
            merged.push_back(cur);
        }
        merged.push_back(Token{Token::Type::End, {}, {}, 0.0f});
        tokens = std::move(merged);
    }

    std::string_view text_;
    size_t pos_ = 0;
};

// ------------------------------------------------------------------- parser

// The entities a `redirect, trigger` can name. Anything else is treated as an
// ordinary value followed by an argument comma.
// The names that may stand on the left of `target, trigger`. The list is the
// one the cns documentation calls "the complete list of redirection keywords":
// `parent`, `root`, `helper`, `helper(ID)`, `target`, `target(ID)`, `partner`,
// `enemy`, `enemy(n)`, `enemyNear`, `enemyNear(n)` and `playerID(ID)`. Missing
// from it for a long time were `enemy` and `partner` -- two of the most common
// redirects there are, `triggerall = enemy, stateno != 5050` being the usual way
// a character asks what the opponent is doing. Unrecognised, `enemy` parsed as a
// bare value and left its comma for the caller to trip over, so the whole line
// failed to parse and read as false: 898 lines in the corpus carry a comparison
// behind one of these two.
//
// `self` and `p1`/`p2` are not in the documented list but are accepted here
// because files write them; a redirect the engine cannot resolve evaluates to
// zero, which is what an unknown target should do.
const std::unordered_set<std::string>& redirectTargets() {
    static const std::unordered_set<std::string> kTargets = {
        "p2", "enemy", "enemynear", "root", "helper", "target", "parent",
        "self", "p1", "partner", "playerid",
    };
    return kTargets;
}

// Precedence, loosest first. This is the documentation's own table, read from
// the bottom up -- `cns.html`, "Precedence and associativity of operators":
//
//     ! ~ - (unary)          highest
//     **
//     * / %
//     + -
//     > >= < <=
//     = != intervals
//     :=
//     &
//     ^
//     |
//     &&
//     ^^
//     ||                     lowest
//
// Two things about it are worth stating because they are easy to get wrong:
//
//   * `:=` is not the loosest operator -- it is bound *tighter* than every
//     logical and bitwise operator, and only looser than comparison. So
//     `life >= 800 && var(32) := 0` assigns 0 to var(32) and then ands that
//     with the comparison, which is exactly how the corpus uses it, and the
//     docs' own worked example writes `(var(0) := (command="abc" &&
//     command != "holddown") && power >= 1000)` on the same reading.
//   * `&`, `^` and `|` are *bitwise*, not one-character spellings of `&&` and
//     `||` -- which does not matter when the operands are 0/1, as they are in
//     every corpus use, but it does decide where they sit relative to `:=`.
//
// The comparison operators are likewise two levels, not one: relational binds
// tighter than equality, as in C. So `a = b < c` is `a = (b < c)`.
//
// Only the equality level takes interval operands: MUGEN allows an interval on
// the right of `=` or `!=` and nowhere else.
struct Level {
    const char* const* ops;
    size_t count;
    bool intervals = false;
    bool assign = false;
};

const char* const kOr[] = {"||"};
const char* const kXor[] = {"^^"};
const char* const kAnd[] = {"&&"};
const char* const kBitOr[] = {"|"};
const char* const kBitXor[] = {"^"};
const char* const kBitAnd[] = {"&"};
const char* const kAssign[] = {":="};
const char* const kEquality[] = {"=", "!="};
const char* const kRelational[] = {">", ">=", "<", "<="};
const char* const kAdd[] = {"+", "-"};
const char* const kMul[] = {"*", "/", "%"};
const char* const kPow[] = {"**"};

constexpr Level kLevels[] = {
    {kOr, 1},           {kXor, 1},          {kAnd, 1},
    {kBitOr, 1},        {kBitXor, 1},        {kBitAnd, 1},
    {kAssign, 1, false, true},
    {kEquality, 2, true},
    {kRelational, 4},   {kAdd, 2},         {kMul, 3},        {kPow, 1},
};

Value truthValue(bool b) { return Value::boolean(b); }

// The six operators that spell a comparison. `TimeMod` is written with one of
// these between the name and its two operands, where everywhere else an
// operator takes a single right-hand side.
bool isComparison(const std::string& op) {
    return op == "=" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=";
}

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

    bool failed() const { return failed_; }
    std::vector<Expr::Node> takeNodes() { return std::move(nodes_); }

    int parseAll() {
        const int root = parseExpression(0);
        if (failed_ || peek().type != Token::Type::End) {
            failed_ = true;
            return -1;
        }
        return root;
    }

private:
    const Token& peek(size_t ahead = 0) const {
        const size_t i = pos_ + ahead;
        return i < tokens_.size() ? tokens_[i] : tokens_.back();
    }
    void advance() {
        if (pos_ + 1 < tokens_.size()) ++pos_;
    }

    int addNode() {
        nodes_.push_back(Expr::Node{});
        return static_cast<int>(nodes_.size()) - 1;
    }

    int parseExpression(size_t level) {
        if (level >= std::size(kLevels)) return parseUnary();
        int left = parseExpression(level + 1);
        const Level& l = kLevels[level];
        while (!failed_ && peek().type == Token::Type::Op) {
            bool matched = false;
            for (size_t i = 0; i < l.count; ++i) {
                if (peek().text == l.ops[i]) {
                    matched = true;
                    break;
                }
            }
            if (!matched) break;
            const std::string op = peek().text;
            if (l.assign) {
                // `var(0) := expr`, right-associative, and the only operator
                // whose left side is a place rather than a value. Anything but
                // a var-family write on the left is a parse failure: quietly
                // reading `stateno := 5` as a comparison would hide the typo.
                const Expr::Node& target = nodes_[static_cast<size_t>(left)];
                const bool writable =
                    !failed_ && target.kind == Expr::Node::Kind::Trigger &&
                    (target.text == "var" || target.text == "fvar" ||
                     target.text == "sysvar" || target.text == "sysfvar") &&
                    target.args.size() == 1;
                if (!writable) {
                    failed_ = true;
                    return -1;
                }
                advance();
                const int right = parseExpression(level);  // right-associative
                const int node = addNode();
                nodes_[node].kind = Expr::Node::Kind::Assign;
                nodes_[node].args = {left, right};
                left = node;
                continue;
            }
            advance();
            // `stateno = [200,650]` is a comparison whose right arm is a range
            // and not an expression, and it is everywhere in the corpus and in
            // Elecbyte's own common1.cns -- the fall states hand a victim to the
            // ground with `Pos Y >= [..]`-shaped tests and a character tests
            // `prevstateno = [200,650]` to ask "was I in an attack". Handed to
            // the ordinary parse, `[` was not even a token, so the whole trigger
            // failed to parse and read as false: every controller gated on one
            // was inert. `interval()` claims the right arm here, before the
            // level below can see it.
            if (l.intervals && (op == "=" || op == "!=")) {
                const int interval = parseInterval(left, op);
                if (interval >= 0) {
                    left = interval;
                    continue;
                }
            }
            const int right = parseExpression(level + 1);
            const int node = addNode();
            nodes_[node].kind = Expr::Node::Kind::Binary;
            nodes_[node].text = op;
            nodes_[node].args = {left, right};
            left = node;
        }
        return left;
    }

    // The right arm of `x = [y,z]`. MUGEN documents the four spellings as the
    // obvious open/closed readings of the two brackets, and the negated forms by
    // De Morgan: `x != [y,z]` is `(x < y) || (x > z)`. The node keeps the ends
    // and the operator rather than expanding into `&&`/`||`, so that `x` is
    // evaluated once -- it may be `random`.
    //
    // A `(` after `=` is ambiguous: `x = (1+2)` is a group and `x = (0,1)` is an
    // interval. The two are told apart by looking for a comma at the top level of
    // the group, which is the one thing a group never contains and an interval
    // always does. Returns -1, consuming nothing, when this is not an interval.
    int parseInterval(int left, const std::string& op) {
        bool lowClosed = false;
        if (peek().type == Token::Type::LBracket) {
            lowClosed = true;
        } else if (peek().type == Token::Type::LParen) {
            if (!groupHoldsRange()) return -1;
        } else {
            return -1;
        }
        advance();  // the opening bracket

        const int low = parseExpression(0);
        if (failed_ || peek().type != Token::Type::Comma) {
            failed_ = true;
            return -1;
        }
        advance();  // the comma
        const int high = parseExpression(0);

        bool highClosed = false;
        if (peek().type == Token::Type::RBracket) {
            highClosed = true;
        } else if (peek().type != Token::Type::RParen) {
            failed_ = true;
            return -1;
        }
        advance();  // the closing bracket

        const int node = addNode();
        nodes_[node].kind = Expr::Node::Kind::Interval;
        nodes_[node].text = op;
        nodes_[node].args = {left, low, high};
        nodes_[node].lowClosed = lowClosed;
        nodes_[node].highClosed = highClosed;
        return node;
    }

    // True when the group starting at the cursor is really an interval: it must
    // hold a comma at its own level before it closes.
    bool groupHoldsRange() const {
        int depth = 0;
        for (size_t i = 0;; ++i) {
            const Token& t = peek(i);
            if (t.type == Token::Type::End) return false;
            if (t.type == Token::Type::LParen || t.type == Token::Type::LBracket) {
                ++depth;
                continue;
            }
            if (t.type == Token::Type::RParen || t.type == Token::Type::RBracket) {
                if (--depth == 0) return false;
                continue;
            }
            if (t.type == Token::Type::Comma && depth == 1) return true;
        }
    }

    int parseUnary() {
        // `!`, `~` and unary `-`, which the documentation puts at the top of
        // the precedence table and which associate right to left. `~` is the
        // bitwise complement; like the other bitwise operators it truncates its
        // operand to an integer first.
        if (peek().type == Token::Type::Op &&
            (peek().text == "-" || peek().text == "!" || peek().text == "~")) {
            const std::string op = peek().text;
            advance();
            const int operand = parseUnary();
            const int node = addNode();
            nodes_[node].kind = Expr::Node::Kind::Unary;
            nodes_[node].text = op;
            nodes_[node].args = {operand};
            return node;
        }
        return parsePrimary();
    }

    int parsePrimary() {
        // `command = "QCF_x"` is not a comparison. MUGEN spells its command
        // trigger that way, with the command's name on the right of the `=`, and
        // the string is the trigger's *argument* rather than something to
        // compare against. Read as an ordinary comparison it would call the
        // trigger with no argument at all and every move in every character
        // would be unreachable -- which is exactly what it did.
        //
        // The test is narrow on purpose: only the name `command`, only `=` or
        // `!=`, and only with a string literal after it. `command != "x"` is the
        // negation of the same trigger, which is how a character asks whether a
        // command did *not* just fire.
        // The name may be quoted or bare: MUGEN documents `command = "QCF_x"`
        // and accepts `command = QCF_x`, and character files use both -- about
        // one reference in six is bare. A bare name is taken from the source
        // spelling rather than the folded one, because command names are
        // case-sensitive and `QCF_x` is not `qcf_x`.
        const bool namedCommand =
            peek().type == Token::Type::Ident && peek().text == "command" &&
            peek(1).type == Token::Type::Op && (peek(1).text == "=" || peek(1).text == "!=") &&
            (peek(2).type == Token::Type::String || peek(2).type == Token::Type::Ident);
        if (namedCommand) {
            const bool negated = peek(1).text == "!=";
            advance();  // command
            advance();  // = / !=
            const int node = addNode();
            nodes_[node].kind = Expr::Node::Kind::Trigger;
            nodes_[node].text = "command";
            const int arg = addNode();
            nodes_[arg].kind = Expr::Node::Kind::String;
            nodes_[arg].text =
                peek().type == Token::Type::String ? peek().text : peek().raw;
            advance();  // the name
            nodes_[node].args = {arg};
            if (!negated) return node;

            const int notNode = addNode();
            nodes_[notNode].kind = Expr::Node::Kind::Unary;
            nodes_[notNode].text = "!";
            nodes_[notNode].args = {node};
            return notNode;
        }

        // `hitdefattr = SCA, NA` is the other trigger whose right side is not an
        // expression: an attr *pair*, two bare words with a comma between them.
        // Read as a plain expression the comma strands the NA and the whole line
        // fails to parse -- which makes every controller gated on one inert.
        // Folded here into one string comparison, spelled the way the engine's
        // attr normaliser spells it: "sca, na".
        const bool namedAttr =
            peek().type == Token::Type::Ident && peek().text == "hitdefattr" &&
            peek(1).type == Token::Type::Op && (peek(1).text == "=" || peek(1).text == "!=") &&
            peek(2).type == Token::Type::Ident && peek(3).type == Token::Type::Comma &&
            peek(4).type == Token::Type::Ident && peek(5).type == Token::Type::End;
        if (namedAttr) {
            const bool negated = peek(1).text == "!=";
            advance();  // hitdefattr
            advance();  // = / !=
            // Two string arguments (class probe, type probe) so the engine can
            // apply MUGEN's semantics: class ⊂ probe, type == probe.
            const int node = addNode();
            nodes_[node].kind = Expr::Node::Kind::Trigger;
            nodes_[node].text = "hitdefattr";
            const int cls = addNode();
            nodes_[cls].kind = Expr::Node::Kind::String;
            nodes_[cls].text = peek().text;
            advance();  // SCA
            advance();  // ,
            const int typ = addNode();
            nodes_[typ].kind = Expr::Node::Kind::String;
            nodes_[typ].text = peek().text;
            advance();  // NA
            nodes_[node].args = {cls, typ};
            if (!negated) return node;
            const int notNode = addNode();
            nodes_[notNode].kind = Expr::Node::Kind::Unary;
            nodes_[notNode].text = "!";
            nodes_[notNode].args = {node};
            return notNode;
        }

        // `TimeMod = 4, 3` is the third trigger whose right side is not a single
        // expression. The documentation spells its format as
        //
        //     TimeMod [oper] divisor, value1
        //
        // -- a divisor and a value compared against the state-time's remainder,
        // so the pair is one operator with two operands and not a comma list.
        // Read as an ordinary comparison, `TimeMod = 4` parsed and then stranded
        // the `, 3`, which took the whole line down: 984 of them in the corpus,
        // gating loops, palette flickers and idle animations. The documentation
        // also says the `%` operator "subsumes the functionality of TimeMod", so
        // it is expanded here into exactly that -- `(time % divisor) oper value`
        // -- rather than taught to the evaluator as a trigger of its own.
        const bool namedTimeMod =
            peek().type == Token::Type::Ident && peek().text == "timemod" &&
            peek(1).type == Token::Type::Op && isComparison(peek(1).text);
        if (namedTimeMod) {
            const std::string op = peek(1).text;
            advance();  // timemod
            advance();  // the operator
            const int divisor = parseExpression(0);
            if (peek().type != Token::Type::Comma) {
                failed_ = true;
                return divisor;
            }
            advance();  // ,
            const int value = parseExpression(0);
            const int time = addNode();
            nodes_[time].kind = Expr::Node::Kind::Trigger;
            nodes_[time].text = "time";
            const int mod = addNode();
            nodes_[mod].kind = Expr::Node::Kind::Binary;
            nodes_[mod].text = "%";
            nodes_[mod].args = {time, divisor};
            const int node = addNode();
            nodes_[node].kind = Expr::Node::Kind::Binary;
            nodes_[node].text = op;
            nodes_[node].args = {mod, value};
            return node;
        }

        // `AnimElem = 3` asks whether the animation is *at* element 3, and that
        // is an ordinary comparison the evaluator already reads. The documented
        // second form is not:
        //
        //     AnimElem = value1, [oper] value2
        //
        // which asks the same question `AnimElemTime(value1) [oper] value2` asks
        // -- how long the animation has been inside element `value1`, compared
        // against something. The comma stranded the `[oper] value2` and the line
        // failed to parse, which is how `animelem = 5,>=5` and
        // `animelem = 2, >=0 && animelem = 11,<=0` came to be silently false.
        // Expanded here into the AnimElemTime form, so the two spellings of one
        // question share a single implementation.
        const bool namedAnimElem =
            peek().type == Token::Type::Ident && peek().text == "animelem" &&
            peek(1).type == Token::Type::Op && peek(1).text == "=" &&
            peek(2).type == Token::Type::Number && peek(3).type == Token::Type::Comma &&
            (peek(4).type == Token::Type::Op || peek(4).type == Token::Type::LBracket);
        if (namedAnimElem) {
            advance();  // animelem
            advance();  // =
            const int elem = addNode();
            nodes_[elem].kind = Expr::Node::Kind::Constant;
            nodes_[elem].number = peek().number;
            advance();  // the element number
            advance();  // ,
            const int time = addNode();
            nodes_[time].kind = Expr::Node::Kind::Trigger;
            nodes_[time].text = "animelemtime";
            nodes_[time].args = {elem};
            // `AnimElem = 3, [0,5]` is the same question with a range on the
            // right, which is the interval rule and not a second rule.
            const int interval = parseInterval(time, "=");
            if (interval >= 0) return interval;
            if (failed_ || peek().type != Token::Type::Op) {
                failed_ = true;
                return time;
            }
            const std::string op = peek().text;
            advance();
            const int right = parseExpression(0);
            const int node = addNode();
            nodes_[node].kind = Expr::Node::Kind::Binary;
            nodes_[node].text = op;
            nodes_[node].args = {time, right};
            return node;
        }

        // `redirect, trigger` -- checked before anything else, because the
        // target is an identifier that would otherwise parse as a bare value
        // and leave the comma for whoever asked for this expression.
        //
        // The forms that take a selector -- `helper(4000), var(0)`,
        // `playerid(34), stateno`, `enemy(1), life` -- are the same thing with a
        // parenthesised argument between the target and the comma.
        const Token& first = peek();
        size_t look = 1;
        if (first.type == Token::Type::Ident && redirectTargets().count(first.text) != 0) {
            if (peek(look).type == Token::Type::LParen) {
                // Scan ahead for the matching close: a redirect selector is a
                // single parenthesised argument, and MUGEN writes no commas in
                // one, so the first RParen at this depth ends it.
                int depth = 1;
                ++look;
                while (peek(look).type != Token::Type::End && depth > 0) {
                    if (peek(look).type == Token::Type::LParen) ++depth;
                    if (peek(look).type == Token::Type::RParen) --depth;
                    ++look;
                }
                if (depth == 0 && peek(look).type == Token::Type::Comma) {
                    const std::string target = first.text;
                    advance();  // target
                    advance();  // (
                    const int selector = parseExpression(0);
                    if (peek().type == Token::Type::RParen) advance();
                    if (peek().type == Token::Type::Comma) advance();
                    const int inner = parseUnary();
                    const int node = addNode();
                    nodes_[node].kind = Expr::Node::Kind::Redirect;
                    nodes_[node].text = target;
                    nodes_[node].args = {selector, inner};
                    return node;
                }
                // Not a redirect after all: fall through and let the identifier
                // parse as an ordinary call below.
            } else if (peek(1).type == Token::Type::Comma) {
                const std::string target = first.text;
                advance();  // target
                advance();  // comma
                const int inner = parseUnary();
                const int node = addNode();
                nodes_[node].kind = Expr::Node::Kind::Redirect;
                nodes_[node].text = target;
                nodes_[node].args = {inner};
                return node;
            }
        }

        const Token& tok = peek();
        switch (tok.type) {
            case Token::Type::Number: {
                const int node = addNode();
                nodes_[node].kind = Expr::Node::Kind::Constant;
                nodes_[node].number = tok.number;
                advance();
                return node;
            }
            case Token::Type::String: {
                const int node = addNode();
                nodes_[node].kind = Expr::Node::Kind::String;
                nodes_[node].text = tok.text;
                advance();
                return node;
            }
            case Token::Type::Ident: {
                // `TeamMode` is comparison-only, and its right-hand side is a
                // keyword rather than a trigger: `single`, `simul` and `turns`
                // compile to 0, 1 and 3 in the reference (`TEAM-MODES-RE.md`
                // §2.3, opcode 0x79). Read as bare identifiers they fell through
                // to the unknown-trigger fallback, where a word of more than four
                // letters is numeric zero -- so `teammode = single` was 1 = 0 and
                // was false for every character that gated a state on it. The
                // three words are reserved for this comparison in MUGEN 1.0;
                // outside it they name no trigger either, so compiling them here
                // changes only expressions that were malformed or inert.
                if (tok.text == "single" || tok.text == "simul" || tok.text == "turns") {
                    const int node = addNode();
                    nodes_[node].kind = Expr::Node::Kind::Constant;
                    nodes_[node].number = tok.text == "single" ? 0.0f
                                          : tok.text == "simul" ? 1.0f
                                                                : 3.0f;
                    advance();
                    return node;
                }
                const int node = addNode();
                nodes_[node].kind = Expr::Node::Kind::Trigger;
                nodes_[node].text = tok.text;
                advance();
                // Three triggers take a *name* and not an expression as their
                // first argument: `Const(param_name)`, `GetHitVar(param_name)`
                // and `StageVar(param_name)`, where the parameter is written
                // bare -- `Const(data.life)`, `GetHitVar(yaccel)`,
                // `StageVar(info.name)` -- and names no trigger at all. Handed
                // to the ordinary argument parser a bare name becomes a call to
                // a trigger of that name, and an unknown trigger reads as zero:
                // so `GetHitVar(yaccel)` arrived at the gethitvar handler as the
                // *empty* name and answered zero, and `Const(data.life)` did the
                // same at the const handler. The result was that every one of the
                // corpus' ~3800 uses of these three evaluated to zero, silently,
                // wherever they appeared in a controller parameter -- which is
                // where a character keeps its gravity, its constants and its
                // hit-parameter arithmetic.
                //
                // The name is taken from the folded spelling, which is what the
                // constant table is keyed by (the ini reader lower-cases its
                // keys), and the guard on `(` keeps a hypothetical nested call
                // -- `GetHitVar(var(3))` -- parsing the old way rather than
                // silently becoming the string "var". No file in the corpus
                // writes one: both take a name and nothing else.
                const bool nameArgument = tok.text == "const" || tok.text == "gethitvar" ||
                                          tok.text == "stagevar";
                if (peek().type == Token::Type::LParen) {
                    advance();
                    if (nameArgument && peek().type == Token::Type::Ident &&
                        peek(1).type != Token::Type::LParen) {
                        const int arg = addNode();
                        nodes_[arg].kind = Expr::Node::Kind::String;
                        nodes_[arg].text = peek().text;
                        advance();
                        nodes_[node].args.push_back(arg);
                    } else if (peek().type != Token::Type::RParen) {
                        while (true) {
                            // An argument may be a write: `ifelse(var(0) := 3,
                            // 1, 0)` and `abs(var(0) := var(0) + 1)` are how a
                            // character side-effects a var in the middle of an
                            // expression. When the assignment was handled only at
                            // the top of a line, the `:=` here was simply left
                            // unread and the whole controller -- trigger *and*
                            // parameter -- failed to parse and read as zero.
                            const int arg = parseExpression(0);
                            nodes_[node].args.push_back(arg);
                            if (peek().type == Token::Type::Comma) {
                                advance();
                                continue;
                            }
                            break;
                        }
                    }
                    if (peek().type != Token::Type::RParen) {
                        failed_ = true;
                        return node;
                    }
                    advance();
                }
                return node;
            }
            case Token::Type::LParen: {
                advance();
                // A group may hold a write, the same as an argument may:
                // `(var(0) := var(0) + 1) * 2 || !selfanimexist(var(0))` is the
                // commonest way the corpus increments a counter, 2136 times over.
                const int inner = parseExpression(0);
                if (peek().type != Token::Type::RParen) {
                    failed_ = true;
                    return inner;
                }
                advance();
                return inner;
            }
            default:
                failed_ = true;
                return addNode();
        }
    }

    const std::vector<Token>& tokens_;
    size_t pos_ = 0;
    bool failed_ = false;
    std::vector<Expr::Node> nodes_;
};

// ---------------------------------------------------------------- evaluation

Value evalNode(const Expr::Node& node, const std::vector<Expr::Node>& nodes,
               const TriggerContext& context);

Value evalBinary(const std::string& op, const Expr::Node& node,
                 const std::vector<Expr::Node>& nodes, const TriggerContext& context) {
    // MUGEN evaluates both arms and only then combines (its evaluator is a
    // postfix stack machine with no short-circuit paths -- decompile pass 6).
    // With `:=` in the language that matters: `x || (var(0) := 1)` writes the
    // var in MUGEN even when x won. Order is still left-to-right.
    if (op == "&&" || op == "||" || op == "^^") {
        const bool a = evalNode(nodes[static_cast<size_t>(node.args[0])], nodes, context).truthy();
        const bool b = evalNode(nodes[static_cast<size_t>(node.args[1])], nodes, context).truthy();
        if (op == "&&") return truthValue(a && b);
        if (op == "||") return truthValue(a || b);
        return truthValue(a != b);
    }

    const Value a = evalNode(nodes[static_cast<size_t>(node.args[0])], nodes, context);
    const Value b = evalNode(nodes[static_cast<size_t>(node.args[1])], nodes, context);

    // String comparison only when both sides are strings. A string against a
    // number falls back to numbers, where a non-empty string counts as 1 --
    // which is how `command = 1` behaves in MUGEN and is harmless either way.
    if (a.isString && b.isString) {
        if (op == "=") return truthValue(a.str == b.str);
        if (op == "!=") return truthValue(a.str != b.str);
        if (op == "+") return Value::text(a.str + b.str);
    }

    if (op == "+" && (a.isString || b.isString)) {
        return Value::text((a.isString ? a.str : std::to_string(a.toInt())) +
                           (b.isString ? b.str : std::to_string(b.toInt())));
    }

    const float x = a.toFloat();
    const float y = b.toFloat();
    if (op == "=") return truthValue(x == y);
    if (op == "!=") return truthValue(x != y);
    if (op == "<") return truthValue(x < y);
    if (op == "<=") return truthValue(x <= y);
    if (op == ">") return truthValue(x > y);
    if (op == ">=") return truthValue(x >= y);
    if (op == "+") return Value::number(x + y);
    if (op == "-") return Value::number(x - y);
    if (op == "*") return Value::number(x * y);
    if (op == "/") return Value::number(y == 0.0f ? 0.0f : x / y);
    if (op == "%") return Value::number(y == 0.0f ? 0.0f : std::fmod(x, y));
    if (op == "**") return Value::number(std::pow(x, y));
    // The bitwise operators, which the documentation lists as operators in their
    // own right and which files write as if they were the logical ones. MUGEN
    // requires integer operands here; this engine truncates, which is the same
    // thing for the 0/1 operands every corpus use has.
    if (op == "&" || op == "|" || op == "^") {
        const long long i = static_cast<long long>(x);
        const long long j = static_cast<long long>(y);
        const long long r = op == "&" ? (i & j) : (op == "|" ? (i | j) : (i ^ j));
        return Value::number(static_cast<float>(r));
    }
    return Value::number(0.0f);
}

Value evalNode(const Expr::Node& node, const std::vector<Expr::Node>& nodes,
               const TriggerContext& context) {
    using Kind = Expr::Node::Kind;
    switch (node.kind) {
        case Kind::Constant:
            return Value::number(node.number);
        case Kind::String:
            return Value::text(node.text);
        case Kind::Unary: {
            const Value v = evalNode(nodes[static_cast<size_t>(node.args[0])], nodes, context);
            if (node.text == "!") return truthValue(!v.truthy());
            // `~` inverts the bits of the operand's integer reading. MUGEN
            // documents it as producing bottom for a float operand; this engine
            // carries no bottom, so a float is truncated and complemented, which
            // is what MUGEN's own integer conversion would have produced next.
            if (node.text == "~") {
                return Value::number(static_cast<float>(~static_cast<long long>(v.toFloat())));
            }
            return Value::number(-v.toFloat());
        }
        case Kind::Binary:
            return evalBinary(node.text, node, nodes, context);
        case Kind::Redirect: {
            // With two arguments the first is the selector -- `helper(4000)`
            // -- evaluated in the *current* context, and the second the inner
            // trigger, evaluated in the target's.
            int selector = -1;
            size_t inner = 0;
            if (node.args.size() == 2) {
                selector = evalNode(nodes[static_cast<size_t>(node.args[0])], nodes, context).toInt();
                inner = 1;
            }
            const TriggerContext* other = context.redirect(node.text, selector);
            if (other == nullptr) return Value::number(0.0f);
            return evalNode(nodes[static_cast<size_t>(node.args[inner])], nodes, *other);
        }
        case Kind::Assign: {
            // The left arm names the slot; its (only) argument is the index.
            const Expr::Node& target = nodes[static_cast<size_t>(node.args[0])];
            const Value indexValue =
                evalNode(nodes[static_cast<size_t>(target.args[0])], nodes, context);
            const Value value =
                evalNode(nodes[static_cast<size_t>(node.args[1])], nodes, context);
            const char kind = target.text == "var"     ? 'v'
                              : target.text == "fvar"   ? 'f'
                              : target.text == "sysvar" ? 's'
                                                        : 'S';
            context.assignVar(kind, indexValue.toInt(), value.toFloat());
            return value;
        }
        case Kind::Interval: {
            // `x = [y,z]` is `(x >= y) && (x <= z)`, and the negated form is its
            // complement rather than the negation of that, which comes to the
            // same thing but is spelled out in the docs: `x != [y,z]` is
            // `(x < y) || (x > z)`. The open ends move the comparison by one step
            // in the obvious direction.
            const float x = evalNode(nodes[static_cast<size_t>(node.args[0])], nodes, context)
                                .toFloat();
            const float low = evalNode(nodes[static_cast<size_t>(node.args[1])], nodes, context)
                                  .toFloat();
            const float high = evalNode(nodes[static_cast<size_t>(node.args[2])], nodes, context)
                                   .toFloat();
            const bool inside =
                (node.lowClosed ? x >= low : x > low) && (node.highClosed ? x <= high : x < high);
            return truthValue(node.text == "!=" ? !inside : inside);
        }
        case Kind::Trigger: {
            if (node.args.empty()) return context.trigger(node.text, {});
            std::vector<Value> args;
            args.reserve(node.args.size());
            for (const int index : node.args) {
                args.push_back(evalNode(nodes[static_cast<size_t>(index)], nodes, context));
            }
            return context.trigger(node.text, args);
        }
    }
    return Value::number(0.0f);
}

void collectNames(const std::vector<Expr::Node>& nodes, int index, std::vector<std::string>& out) {
    if (index < 0 || index >= static_cast<int>(nodes.size())) return;
    const Expr::Node& node = nodes[static_cast<size_t>(index)];
    if (node.kind == Expr::Node::Kind::Trigger) out.push_back(node.text);
    for (const int arg : node.args) collectNames(nodes, arg, out);
}

}  // namespace

Expr Expr::parse(std::string_view text) {
    Expr out;

    std::vector<Token> tokens;
    Scanner scanner(text);
    if (!scanner.run(tokens)) return out;

    // A trigger line with nothing on the right (`trigger1 =`) is common in
    // half-finished characters. It parses to an empty expression, which is
    // false, which is exactly what an empty condition should be.
    if (tokens.size() <= 1) return out;

    Parser parser(tokens);
    const int root = parser.parseAll();
    if (root < 0 || parser.failed()) return out;

    return Expr::fromNodes(parser.takeNodes(), root);
}

Value Expr::eval(const TriggerContext& context) const {
    if (nodes_.empty() || root_ < 0 || root_ >= static_cast<int>(nodes_.size())) {
        return Value::number(0.0f);
    }
    return evalNode(nodes_[static_cast<size_t>(root_)], nodes_, context);
}

std::vector<std::string> Expr::names() const {
    std::vector<std::string> out;
    collectNames(nodes_, root_, out);
    return out;
}

}  // namespace af
