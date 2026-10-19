// Host tests for the fight engine's logic layer: the trigger expression
// language (engine/expr) and the command parser and matcher (engine/cmd).
//
// Everything here runs off-device with host clang++; the contexts are stubs,
// so a test can say exactly what `pos y` is or which command just fired
// without standing up a fighter. Scenarios are named by SCENARIO() so a
// failure line says which rule it pins.
//
// Where the engine's behaviour deliberately differs from strict MUGEN the
// current behaviour is pinned and the divergence noted in a comment beside
// the check. Where the pinned behaviour looks like a genuine bug, the check
// asserts the intended behaviour and fails -- those failures are the point of
// this suite's bug report, not noise.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_expr_cmd \
//         tools/hosttest/test_expr_cmd.cpp app/src/main/cpp/engine/expr.cpp \
//         app/src/main/cpp/engine/cmd.cpp app/src/main/cpp/core/ini.cpp
//     /tmp/test_expr_cmd
//
// (expr.cpp, cmd.cpp and ini.cpp pull in nothing beyond the standard library,
// so no -lz is needed here -- unlike the SFF codec test.)

#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "engine/cmd.h"
#include "engine/expr.h"
#include "engine/input.h"

namespace {

// ------------------------------------------------------------ little runner

int gChecks = 0;
int gFailures = 0;
std::string gScenario;

void scenario(const char* name) { gScenario = name; }

#define CHECK(expr)                                                        \
    do {                                                                   \
        ++gChecks;                                                         \
        if (!(expr)) {                                                     \
            ++gFailures;                                                   \
            std::fprintf(stderr, "FAIL [%s] %s:%d: %s\n", gScenario.c_str(), \
                         __FILE__, __LINE__, #expr);                       \
        }                                                                  \
    } while (0)

// ------------------------------------------------------------- stub context

// A trigger context that answers from tables, so every assertion is about
// the expression engine and not about a half-built fighter. Records what it
// was asked so tests can pin *how* an expression read its values.
class Ctx : public af::TriggerContext {
public:
    af::Value trigger(std::string_view name,
                      const std::vector<af::Value>& args) const override {
        ++reads;
        readOrder.emplace_back(name);
        if (name == "command") {
            // The string on the right of `command = "x"` is the argument:
            // which command fired, case and all.
            lastCommandArg = args.empty() ? std::string() : args[0].str;
            return af::Value::boolean(!args.empty() && args[0].str == firedCommand &&
                                      !firedCommand.empty());
        }
        if (name == "hitdefattr") {
            // The engine folds `SCA, NA` into two string probes (lower cased
            // by the scanner). The stub stands in for a last-HitDef of class
            // "sca", type "na", and applies MUGEN's rule: the HitDef's class
            // must contain every probe letter, the type must equal the probe.
            hitdefattrArgs.clear();
            for (const af::Value& a : args) hitdefattrArgs.push_back(a.str);
            bool classOk = args.size() == 2;
            if (classOk) {
                for (const char c : args[0].str) {
                    if (std::string("sca").find(c) == std::string::npos) classOk = false;
                }
            }
            return af::Value::boolean(classOk && args[1].str == "na");
        }
        if ((name == "ifelse" || name == "cond") && args.size() == 3) {
            return args[0].truthy() ? args[1] : args[2];
        }
        if ((name == "var" || name == "fvar" || name == "sysvar" ||
             name == "sysfvar") &&
            args.size() == 1) {
            const auto it = vars.find(args[0].toInt());
            return af::Value::number(it != vars.end() ? it->second : 0.0f);
        }
        const auto it = values.find(std::string(name));
        return it != values.end() ? it->second : af::Value::number(0.0f);
    }

    const af::TriggerContext* redirect(std::string_view target,
                                       int selector) const override {
        lastRedirect = std::string(target);
        lastSelector = selector;
        const auto it = redirects.find({std::string(target), selector});
        return it == redirects.end() ? nullptr : it->second;
    }

    bool assignVar(char kind, int index, float value) const override {
        writes.emplace_back(kind, index, value);
        return true;
    }

    std::map<std::string, af::Value> values;
    std::map<int, float> vars;
    std::map<std::pair<std::string, int>, const Ctx*> redirects;
    std::string firedCommand;
    mutable std::string lastCommandArg;
    mutable std::vector<std::string> hitdefattrArgs;
    mutable std::string lastRedirect;
    mutable int lastSelector = -999;
    mutable int reads = 0;
    // The order `trigger()` was called in, so a test can pin that an operator
    // evaluates its arms left to right even though it evaluates both.
    mutable std::vector<std::string> readOrder;
    mutable std::vector<std::tuple<char, int, float>> writes;
};

float num(const char* text, const Ctx& ctx) {
    return af::Expr::parse(text).eval(ctx).toFloat();
}
bool yes(const char* text, const Ctx& ctx) {
    return af::Expr::parse(text).evalBool(ctx);
}
std::string str(const char* text, const Ctx& ctx) {
    return af::Expr::parse(text).eval(ctx).str;
}

// ----------------------------------------------------------- buffer driving

void pushTicks(af::CommandBuffer& buffer, uint32_t held, int ticks, int facing = 1) {
    af::InputState input{};
    input.held = held;
    for (int i = 0; i < ticks; ++i) buffer.push(input, facing);
}

af::CommandDef def(const char* text, int time = 15) {
    af::CommandDef d;
    d.name = "test";
    d.time = time;
    d.steps = af::parseCommandString(text);
    return d;
}

// ---------------------------------------------------------------- scenarios

void testPrecedence() {
    scenario("expr/precedence");
    const Ctx ctx;
    CHECK(num("2 + 3 * 4", ctx) == 14.0f);
    CHECK(num("(2 + 3) * 4", ctx) == 20.0f);
    CHECK(num("10 - 4 - 3", ctx) == 3.0f);  // left associative
    CHECK(num("1 + 2 > 2 && 4 % 3 = 1", ctx) != 0.0f);
    CHECK(num("-2 * 3", ctx) == -6.0f);

    // `^^` sits between `||` and `&&` in MUGEN's ordering. Each of these only
    // comes out right if the middle operator does not leak into either of its
    // neighbours' groupings.
    CHECK(yes("1 || 1 ^^ 1", ctx));    // 1 || (1 ^^ 1) -- loosest is ||
    CHECK(yes("1 ^^ 1 && 0", ctx));    // 1 ^^ (1 && 0) -- && binds tighter
    CHECK(yes("1 ^^ 1 ^^ 1", ctx));    // left associative: (1 ^^ 1) ^^ 1
    CHECK(yes("1 > 2 ^^ 3 > 2", ctx)); // comparisons underneath
}

void testTwoWordNames() {
    scenario("expr/two-word-names");
    Ctx ctx;
    ctx.values["pos y"] = af::Value::number(-8);
    ctx.values["p2bodydist x"] = af::Value::number(42);
    CHECK(yes("pos y < 0", ctx));
    CHECK(!yes("pos y > 0", ctx));
    CHECK(yes("p2bodydist x < 50", ctx));
    CHECK(yes("p2bodydist X < 50", ctx));  // case-folded
    CHECK(num("pos y + 8", ctx) == 0.0f);
    // The axis glue must not swallow an axis word that belongs to something
    // else: `pos y` is one name, a following bare ident is not part of it.
    CHECK(!af::Expr::parse("pos y z").valid());
}

void testCommandNames() {
    scenario("expr/command-names");
    Ctx ctx;
    ctx.firedCommand = "QCF_x";

    ctx.lastCommandArg.clear();
    CHECK(yes("command = \"QCF_x\"", ctx));
    CHECK(ctx.lastCommandArg == "QCF_x");
    CHECK(!yes("command = \"QCF_y\"", ctx));
    CHECK(yes("command != \"QCF_y\"", ctx));
    CHECK(!yes("command != \"QCF_x\"", ctx));

    // Bare names arrive via the source spelling, case intact: folding
    // `QCF_x` to lower case would silently unhook it from the command list.
    ctx.lastCommandArg.clear();
    CHECK(yes("command = QCF_x", ctx));
    CHECK(ctx.lastCommandArg == "QCF_x");
    CHECK(!yes("command = QCF_y", ctx));

    // The special form composes with the rest of a trigger line.
    ctx.values["time"] = af::Value::number(4);
    CHECK(yes("command = \"QCF_x\" && time > 3", ctx));
    CHECK(!yes("command = \"QCF_x\" && time > 30", ctx));
}

void testRedirects() {
    scenario("expr/redirects");
    Ctx us, enemy, helperCtx;
    us.values["time"] = af::Value::number(12);
    us.values["stateno"] = af::Value::number(200);
    us.values["vel x"] = af::Value::number(2.5f);
    us.values["id"] = af::Value::number(7);
    enemy.values["time"] = af::Value::number(5);
    enemy.values["stateno"] = af::Value::number(500);
    enemy.values["vel x"] = af::Value::number(-3);
    helperCtx.values["pos x"] = af::Value::number(99);

    us.redirects[{"enemynear", -1}] = &enemy;
    us.redirects[{"p2", -1}] = &enemy;
    us.redirects[{"helper", 4000}] = &helperCtx;

    CHECK(yes("enemynear, vel x < 0", us));   // their velocity, not ours
    CHECK(!yes("enemynear, vel x > 0", us));
    CHECK(yes("vel x > 0", us));              // unredirected reads ours

    // Selector forms: the parenthesised argument is evaluated in the current
    // context and handed to redirect().
    CHECK(num("helper(4000), pos x", us) == 99.0f);
    CHECK(us.lastSelector == 4000);
    us.redirects[{"playerid", 3}] = &enemy;
    CHECK(num("playerid(3), stateno", us) == 500.0f);
    CHECK(us.lastSelector == 3);
    us.redirects[{"target", 7}] = &enemy;
    CHECK(num("target(id), stateno", us) == 500.0f);  // `id` read from *us*
    CHECK(us.lastSelector == 7);

    // The selector is an expression, evaluated in the current context -- not
    // just an integer literal. `helper(var(0))` with var(0) = 4000 is the same
    // redirect as `helper(4000)`, and that is what a character uses when the
    // id it wants is computed.
    us.vars[0] = 4000;
    CHECK(num("helper(var(0)), pos x", us) == 99.0f);
    CHECK(us.lastSelector == 4000);
    us.vars[1] = 3;
    CHECK(num("playerid(var(1)), stateno", us) == 500.0f);
    CHECK(us.lastSelector == 3);

    // Bare redirect passes selector -1.
    CHECK(num("p2, stateno", us) == 500.0f);
    CHECK(us.lastSelector == -1);

    // A redirect to something that does not exist is zero and must not take
    // the rest of the expression down.
    CHECK(!yes("root, vel x > 0", us));
    CHECK(yes("time = 12 || root, vel x > 0", us));
    CHECK(num("helper(999), time", us) == 0.0f);

    // The engine deliberately groups the redirect tightly: the comma takes
    // only the next trigger, and operators apply to the redirected value.
    // Strict MUGEN evaluates the rest of the line in the target's context, so
    // `p2, time = 5 && stateno = 200` would be false there (their stateno is
    // 500). Pinned here as the engine's actual semantics: the comparison is
    // applied to their `time`, but `stateno` is read from *us*.
    CHECK(yes("p2, time = 5 && stateno = 200", us));
    // A redirect can still be nested: target's redirect of target's ...
    enemy.redirects[{"self", -1}] = &enemy;
    CHECK(num("p2, self, stateno", us) == 500.0f);
}

void testIfelseCond() {
    scenario("expr/ifelse-cond");
    Ctx ctx;
    ctx.vars[4] = 3;
    CHECK(num("ifelse(1, 7, 9)", ctx) == 7.0f);
    CHECK(num("ifelse(0, 7, 9)", ctx) == 9.0f);
    CHECK(num("cond(0, 7, 9)", ctx) == 9.0f);
    CHECK(num("cond(var(4), 1, 2)", ctx) == 1.0f);  // var(4) reads the slot, nonzero is truthy
    CHECK(str("ifelse(1, \"yes\", \"no\")", ctx) == "yes");
    CHECK(str("ifelse(0, \"yes\", \"no\")", ctx) == "no");
    // Nested in both arms.
    CHECK(num("ifelse(1 > 2, 10, ifelse(2 > 1, 20, 30))", ctx) == 20.0f);
}

void testStrings() {
    scenario("expr/string-compare-concat");
    Ctx ctx;
    ctx.values["name"] = af::Value::text("kfm");
    ctx.values["statetype"] = af::Value::text("A");
    CHECK(yes("name = \"kfm\"", ctx));
    CHECK(!yes("name = \"akari\"", ctx));
    CHECK(yes("name != \"akari\"", ctx));
    CHECK(yes("statetype = \"A\"", ctx));
    CHECK(str("\"he\" + \"y\"", ctx) == "hey");
    CHECK(str("name + \"2\"", ctx) == "kfm2");
    CHECK(str("\"v\" + 2", ctx) == "v2");  // number folds to int text
    // Strings are truthy when non-empty, so they work bare in &&.
    CHECK(yes("name && time > 1" /* time unknown -> 0 */ , ctx) == false);
    ctx.values["time"] = af::Value::number(5);
    CHECK(yes("name && time > 1", ctx));
    // A string against a number compares numerically: non-empty counts as 1.
    CHECK(yes("name = 1", ctx));
}

void testAssignment() {
    scenario("expr/assignment");
    Ctx ctx;
    ctx.values["pos y"] = af::Value::number(-8);

    CHECK(num("var(0) := 5", ctx) == 5.0f);  // the write evaluates to the value
    CHECK(ctx.writes.size() == 1);
    CHECK(std::get<0>(ctx.writes.back()) == 'v');
    CHECK(std::get<1>(ctx.writes.back()) == 0);
    CHECK(std::get<2>(ctx.writes.back()) == 5.0f);

    ctx.writes.clear();
    CHECK(num("var(3) := pos y", ctx) == -8.0f);  // value is an expression
    CHECK(std::get<1>(ctx.writes.back()) == 3);
    CHECK(std::get<2>(ctx.writes.back()) == -8.0f);

    ctx.writes.clear();
    num("var(1 + 2) := 9", ctx);  // index is an expression
    CHECK(ctx.writes.size() == 1 && std::get<1>(ctx.writes.back()) == 3);

    // The var family, all four kinds.
    ctx.writes.clear();
    num("fvar(1) := 2.5", ctx);
    num("sysvar(2) := -1", ctx);
    num("sysfvar(0) := 0.5", ctx);
    CHECK(ctx.writes.size() == 3);
    CHECK(std::get<0>(ctx.writes[0]) == 'f');
    CHECK(std::get<0>(ctx.writes[1]) == 's');
    CHECK(std::get<0>(ctx.writes[2]) == 'S');
    CHECK(std::get<2>(ctx.writes[0]) == 2.5f);

    // Right associative: both writes happen, inner first.
    ctx.writes.clear();
    CHECK(num("var(0) := var(1) := 4", ctx) == 4.0f);
    CHECK(ctx.writes.size() == 2);
    CHECK(std::get<1>(ctx.writes[0]) == 1);
    CHECK(std::get<1>(ctx.writes[1]) == 0);

    // Three deep: the chain unwinds right to left, so the innermost slot is
    // written first and the value flows outward.
    ctx.writes.clear();
    CHECK(num("var(0) := var(1) := var(2) := 7", ctx) == 7.0f);
    CHECK(ctx.writes.size() == 3);
    CHECK(std::get<1>(ctx.writes[0]) == 2);
    CHECK(std::get<1>(ctx.writes[1]) == 1);
    CHECK(std::get<1>(ctx.writes[2]) == 0);
    CHECK(std::get<2>(ctx.writes[0]) == 7.0f);
    CHECK(std::get<2>(ctx.writes[2]) == 7.0f);

    // The chain is per-kind: an `fvar` slot can hold the `sysvar` slot's value.
    ctx.writes.clear();
    num("fvar(0) := sysvar(1) := 2", ctx);
    CHECK(ctx.writes.size() == 2);
    CHECK(std::get<0>(ctx.writes[0]) == 's' && std::get<1>(ctx.writes[0]) == 1);
    CHECK(std::get<0>(ctx.writes[1]) == 'f' && std::get<1>(ctx.writes[1]) == 0);

    // `:=` binds looser than every comparison, so the whole right side is the
    // value: this writes 1 (5 > 3), not 5.
    ctx.writes.clear();
    CHECK(num("var(0) := 5 > 3", ctx) == 1.0f);
    CHECK(ctx.writes.size() == 1 && std::get<2>(ctx.writes[0]) == 1.0f);

    // An assignment is accepted wherever a subexpression begins, not just at
    // the top of a trigger line: inside a group and inside an argument list.
    // An earlier version of this parser took `:=` only at the top level and
    // pinned that as correct on the assumption that no character parenthesised
    // a write -- which the corpus disproves: `(var(0) := var(0) + 1)` and
    // friends appear 2136 times across the character packs, and
    // `ifelse(var(0) := 3, 1, 0)` and `abs(var(0) := ...)` are common too. The
    // failure was silent, and worse than silent in a trigger: the line did not
    // parse, so the controller it guarded never ran.
    ctx.writes.clear();
    CHECK(num("(var(0) := 3)", ctx) == 3.0f);
    CHECK(ctx.writes.size() == 1 && std::get<1>(ctx.writes[0]) == 0);
    ctx.writes.clear();
    CHECK(num("1 + (var(0) := 3)", ctx) == 4.0f);
    CHECK(ctx.writes.size() == 1);
    // The write still binds loosest inside the group, so the group's value is
    // the assigned value and arithmetic outside it sees that.
    ctx.writes.clear();
    CHECK(num("(var(0) := 2) * 3", ctx) == 6.0f);
    ctx.writes.clear();
    CHECK(num("ifelse(var(0) := 3, 1, 0)", ctx) == 1.0f);
    CHECK(ctx.writes.size() == 1);
    // The comma still separates arguments rather than being eaten by the write.
    // `ifelse` is a function and not a special form, so *both* of its value
    // arguments are evaluated before it picks one -- the write happens even
    // though the branch holding it is the one not taken, and the expression is
    // still 0.
    ctx.writes.clear();
    CHECK(num("ifelse(0, var(0) := 9, 0)", ctx) == 0.0f);
    CHECK(ctx.writes.size() == 1 && std::get<2>(ctx.writes[0]) == 9.0f);

    // `:=` binds looser than comparison but *tighter* than every logical and
    // bitwise operator -- the position the documentation's precedence table
    // gives it. So `var(0) := 1 && 0` assigns 1 and then ands it with 0: the
    // write holds 1, not 0. Read the other way (assignment loosest, as most
    // languages have it) the var would be written with the `&&` result instead,
    // and `life >= 800 && var(32) := 0` -- a real line, five of them in one
    // character -- would not parse at all.
    ctx.writes.clear();
    CHECK(num("var(0) := 1 && 0", ctx) == 0.0f);
    CHECK(ctx.writes.size() == 1 && std::get<2>(ctx.writes[0]) == 1.0f);
    ctx.writes.clear();
    CHECK(num("var(0) := 1 || 2", ctx) == 1.0f);
    CHECK(ctx.writes.size() == 1 && std::get<2>(ctx.writes[0]) == 1.0f);

    // The corpus' own shape: a write as the last term of an `&&` chain.
    ctx.values["life"] = af::Value::number(900);
    ctx.writes.clear();
    CHECK(num("life >= 800 && life <= 1000 && var(32) := 0", ctx) == 0.0f);
    CHECK(ctx.writes.size() == 1 && std::get<1>(ctx.writes[0]) == 32);

    // Only var-family slots are writable; anything else must fail to parse
    // rather than be silently read as a comparison.
    CHECK(!af::Expr::parse("stateno := 5").valid());
    CHECK(!af::Expr::parse("pos y := 5").valid());
    CHECK(!af::Expr::parse("var := 5").valid());
    // ... and that rule holds inside a group too, not just at the top level.
    CHECK(!af::Expr::parse("(stateno := 5)").valid());
    CHECK(af::Expr::parse("var(0) := 5").evalBool(ctx));  // usable as a trigger
}

void testHitdefattr() {
    scenario("expr/hitdefattr-fold");
    Ctx ctx;

    ctx.hitdefattrArgs.clear();
    CHECK(yes("hitdefattr = SCA, NA", ctx));
    CHECK(ctx.hitdefattrArgs.size() == 2);
    // The two probes arrive lower cased, spelled the way the engine's attr
    // normaliser spells them.
    CHECK(ctx.hitdefattrArgs[0] == "sca");
    CHECK(ctx.hitdefattrArgs[1] == "na");

    CHECK(!yes("hitdefattr != SCA, NA", ctx));    // same probe, negated
    CHECK(!yes("hitdefattr = SCA, AP", ctx));     // type probe must match
    CHECK(yes("hitdefattr = S, NA", ctx));        // class probe is a subset test
    CHECK(!yes("hitdefattr = SD, NA", ctx));      // 'd' is not in the class
    CHECK(yes("hitdefattr = sca, na", ctx));      // case insensitive

    // Divergence pinned: with anything after the pair the fold does not
    // trigger and the stranded comma makes the line unparseable. MUGEN
    // accepts `hitdefattr = SCA, NA && movecontact`; the engine reads the
    // pair only as a whole line, so characters that write it mid-expression
    // lose the whole trigger.
    CHECK(!af::Expr::parse("hitdefattr = SCA, NA && 1").valid());
}

void testRobustness() {
    scenario("expr/robustness");
    Ctx ctx;
    ctx.values["time"] = af::Value::number(12);

    CHECK(yes("time = 12 ; and this is a comment", ctx));
    CHECK(yes("; comment first\ntime = 12", ctx));

    // Empty and whitespace-only trigger lines: not valid, evaluate false.
    const char* const empties[] = {"", "   ", "\t", "; just a comment"};
    for (const char* text : empties) {
        CHECK(!af::Expr::parse(text).valid());
        CHECK(!af::Expr::parse(text).evalBool(ctx));
    }

    // Malformed input: must not crash, must not be valid, must not fire.
    const char* const broken[] = {
        "1 +", "(1 + 2", "1 2", ")", "))", "&& 1", "1 &&", "@", ":=",
        "\"unterminated", "1 + + 2", "< 3", "! ", "5 %",
    };
    for (const char* text : broken) {
        CHECK(!af::Expr::parse(text).valid());
        CHECK(!af::Expr::parse(text).evalBool(ctx));
    }

    // A zero-argument call is a parseable unknown trigger: valid, and zero.
    CHECK(af::Expr::parse("var()").valid());
    CHECK(!yes("var()", ctx));

    CHECK(!yes("some.unknown.trigger > 0", ctx));  // unknown reads as zero
    CHECK(num("7 / 0", ctx) == 0.0f);              // guarded, not a crash
    CHECK(num("7 % 0", ctx) == 0.0f);

    // Divergence pinned: && and || evaluate both sides (MUGEN short
    // circuits). The evalBinary comment claims short-circuit; the code does
    // not do it. Harmless for reads -- a null redirect is zero -- but a test
    // can see the read happen.
    ctx.reads = 0;
    CHECK(!yes("0 && probed", ctx));
    CHECK(ctx.reads == 1);  // rhs was read despite the false lhs
}

void testNoShortCircuit() {
    scenario("expr/no-short-circuit");
    Ctx ctx;

    // MUGEN's evaluator is a postfix stack machine with no short-circuit arms
    // (decompile pass 6), and the engine matches it on purpose. So both sides
    // of &&, || and ^^ are always evaluated -- an unknown trigger on the "dead"
    // side still counts as a read. Pinned by counting reads, which is the only
    // observable a pure read has.
    ctx.reads = 0;
    CHECK(!yes("0 && probed", ctx));
    CHECK(ctx.reads == 1);  // rhs read despite the false lhs

    ctx.reads = 0;
    CHECK(yes("1 || probed", ctx));
    CHECK(ctx.reads == 1);  // rhs read despite the true lhs

    ctx.reads = 0;
    CHECK(yes("probed ^^ 1", ctx));
    CHECK(ctx.reads == 1);

    // And the arms still run left to right -- evaluating both is not the same
    // as evaluating them in any order, because a trigger can have side effects
    // in a real context.
    ctx.reads = 0;
    ctx.readOrder.clear();
    CHECK(!yes("alpha && beta", ctx));
    CHECK(ctx.readOrder.size() == 2);
    CHECK(ctx.readOrder[0] == "alpha" && ctx.readOrder[1] == "beta");

    ctx.values["gamma"] = af::Value::number(1);
    ctx.reads = 0;
    ctx.readOrder.clear();
    CHECK(yes("gamma || delta", ctx));
    CHECK(ctx.readOrder.size() == 2);
    CHECK(ctx.readOrder[0] == "gamma" && ctx.readOrder[1] == "delta");
}

void testCmdParseElements() {
    scenario("cmd/parse-elements");
    auto steps = af::parseCommandString("D, DF, F, x");
    CHECK(steps.size() == 4);
    CHECK(steps[0].symbols == af::CmdKey::kDown);
    CHECK(steps[1].symbols == (af::CmdKey::kDown | af::CmdKey::kForward));
    CHECK(!steps[0].hold && !steps[0].release && !steps[0].chained);

    // Trailing comma / empty elements are tolerated.
    CHECK(af::parseCommandString("D, DF, F, x,").size() == 4);
    CHECK(af::parseCommandString("D,, x").size() == 2);

    steps = af::parseCommandString(">DF");
    CHECK(steps.size() == 1 && steps[0].chained);

    steps = af::parseCommandString("~D, DF, F, x");
    CHECK(steps.size() == 4 && steps[0].release && !steps[0].hold);

    steps = af::parseCommandString("/x");
    CHECK(steps.size() == 1 && steps[0].hold && steps[0].holdTicks == 0);

    // A `/` with a count is the same hold with a floor on the duration, which
    // is the spelling some characters use for a charge instead of `N$D`.
    steps = af::parseCommandString("10/D");
    CHECK(steps.size() == 1 && steps[0].hold && steps[0].holdTicks == 10);

    // A leading count implies the hold (`45$D` with no `/`).
    steps = af::parseCommandString("45$D");
    CHECK(steps.size() == 1 && steps[0].hold && steps[0].holdTicks == 45);

    // The standard charge spelling keeps both flags and the count...
    steps = af::parseCommandString("~30$D");
    CHECK(steps.size() == 1 && steps[0].release && steps[0].hold &&
          steps[0].holdTicks == 30);

    // `$` is recorded as no flag at all -- the matcher is inclusive by
    // construction, so plain and 4-way forms land on the same set.
    steps = af::parseCommandString("$D");
    CHECK(steps.size() == 1 && !steps[0].hold && !steps[0].release &&
          steps[0].symbols == af::CmdKey::kDown);

    // Start is an addressable button.
    steps = af::parseCommandString("s");
    CHECK(steps.size() == 1 && steps[0].symbols == af::CmdKey::button('s'));
    steps = af::parseCommandString("a+s");
    CHECK(steps.size() == 1 && steps[0].symbols ==
                                   (af::CmdKey::button('a') | af::CmdKey::button('s')));

    // An element the parser cannot make sense of kills the whole command
    // rather than matching a subsequence of itself.
    CHECK(af::parseCommandString("QCF_x").empty());  // upper case non-direction
    CHECK(af::parseCommandString("D, @, x").empty());
    CHECK(af::parseCommandString("").empty());
}

void testCmdChainOperator() {
    scenario("cmd/chain-operator");
    const af::CommandDef chained = def("D, >DF, x");

    // Clean run: D, then DF on the very next tick, silence, then x.
    {
        af::CommandBuffer b;
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kDown, 1);
        pushTicks(b, af::Button::kDown | af::Button::kRight, 1);
        pushTicks(b, 0, 2);
        pushTicks(b, af::Button::kX, 1);
        CHECK(b.match(chained));
    }

    // Junk between D and DF breaks the chain...
    {
        af::CommandBuffer b;
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kDown, 1);
        pushTicks(b, af::Button::kA, 1);  // the junk tick
        pushTicks(b, af::Button::kDown | af::Button::kRight, 1);
        pushTicks(b, 0, 2);
        pushTicks(b, af::Button::kX, 1);
        CHECK(!b.match(chained));
    }

    // ...but neutral ticks are silence, so a gap with nothing held is fine.
    {
        af::CommandBuffer b;
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kDown, 1);
        pushTicks(b, 0, 2);
        pushTicks(b, af::Button::kDown | af::Button::kRight, 1);
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kX, 1);
        CHECK(b.match(chained));
    }

    // Without `>` the same junk is ignored (a junk-tolerant command).
    {
        af::CommandBuffer b;
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kDown, 1);
        pushTicks(b, af::Button::kA, 1);
        pushTicks(b, af::Button::kDown | af::Button::kRight, 1);
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kX, 1);
        CHECK(b.match(def("D, DF, x")));
    }
}

void testCmdStartSymbol() {
    scenario("cmd/start-symbol");
    af::CommandList list;
    list.parse("[Command]\n"
               "name = \"start_press\"\n"
               "command = s\n");

    af::CommandBuffer b;
    pushTicks(b, 0, 2);
    pushTicks(b, af::Button::kStart, 1);
    CHECK(list.triggered("start_press", b));

    // Stale by one tick: the press was last tick, not this one.
    pushTicks(b, 0, 1);
    CHECK(!list.triggered("start_press", b));

    // Holding start into a second tick is not a fresh press either.
    af::CommandBuffer held;
    pushTicks(held, af::Button::kStart, 2);
    CHECK(!held.match(def("s")));
}

void testCmdSlashHold() {
    scenario("cmd/slash-hold");
    const af::CommandDef holdD = def("/D, x");
    const af::CommandDef pressD = def("D, x");

    // D already held for longer than any time window, then x pressed *while
    // D stays down*: only the `/` form may match, because plain `D` demands
    // the direction to have gone down within recorded history.
    af::CommandBuffer b;
    pushTicks(b, af::Button::kDown, 20);
    pushTicks(b, af::Button::kDown | af::Button::kX, 1);
    CHECK(b.match(holdD));
    CHECK(!b.match(pressD));  // D never went freshly down

    // A freshly pressed D satisfies both.
    af::CommandBuffer fresh;
    pushTicks(fresh, 0, 1);
    pushTicks(fresh, af::Button::kDown, 1);
    pushTicks(fresh, 0, 1);
    pushTicks(fresh, af::Button::kX, 1);
    CHECK(fresh.match(pressD));
    CHECK(fresh.match(holdD));  // a fresh press is also held right now
}

void testCmdTildeRelease() {
    scenario("cmd/tilde-release");
    const af::CommandDef releaseD = def("~D, x");

    // D held, then released, then x fires.
    af::CommandBuffer b;
    pushTicks(b, af::Button::kDown, 2);
    pushTicks(b, 0, 1);
    pushTicks(b, af::Button::kX, 1);
    CHECK(b.match(releaseD));

    // D still held all the way through the press: no release tick exists.
    af::CommandBuffer blocked;
    pushTicks(blocked, af::Button::kDown, 3);
    pushTicks(blocked, af::Button::kDown | af::Button::kX, 1);
    pushTicks(blocked, af::Button::kDown | af::Button::kX, 1);
    CHECK(!blocked.match(releaseD));
}

void testCmdPressVsHold() {
    scenario("cmd/press-vs-hold");
    // A button element with no hold prefix is a *fresh press*: held now and not
    // held on the tick before. That is what stops a command re-firing on every
    // tick a button is left down.
    {
        af::CommandBuffer held;
        pushTicks(held, af::Button::kX, 3);
        CHECK(!held.match(def("x")));  // down now, but not newly down
    }
    {
        af::CommandBuffer fresh;
        pushTicks(fresh, 0, 1);
        pushTicks(fresh, af::Button::kX, 1);
        CHECK(fresh.match(def("x")));
    }

    // A direction-only element counts as pressed when the held *direction set*
    // changed on the way in -- the reading the canonical roll depends on. A
    // direction held since before the window has not changed, so a double tap
    // needs a real second tap: `F, F` with F never released does not fire.
    {
        af::CommandBuffer held;
        pushTicks(held, af::Button::kRight, 5);
        CHECK(!held.match(def("F, F")));
    }
    {
        af::CommandBuffer tap;
        pushTicks(tap, 0, 1);
        pushTicks(tap, af::Button::kRight, 1);
        pushTicks(tap, 0, 1);
        pushTicks(tap, af::Button::kRight, 1);
        CHECK(tap.match(def("F, F")));
    }

    // Divergence pinned: cmd.h calls `/` a tap -- "pressed and released" -- but
    // the matcher implements it as a hold, so `/x` fires on any tick where x is
    // down, release or not. Documented as current behaviour rather than
    // asserted as correct, because a strict tap would need a different reading
    // of the history and no character here depends on the difference.
    {
        af::CommandBuffer held;
        pushTicks(held, af::Button::kX, 3);
        CHECK(held.match(def("/x")));
    }
}

void testCmdDollarDirections() {
    scenario("cmd/dollar-directions");
    const af::CommandDef dollar = def("$D, x");

    // A diagonal freshly entered counts, because the matcher is inclusive:
    // the element's bits are a subset of what is held.
    af::CommandBuffer diagonal;
    pushTicks(diagonal, 0, 1);
    pushTicks(diagonal, af::Button::kDown | af::Button::kRight, 1);
    pushTicks(diagonal, 0, 1);
    pushTicks(diagonal, af::Button::kX, 1);
    CHECK(diagonal.match(dollar));

    // Forward without down does not.
    af::CommandBuffer forwardOnly;
    pushTicks(forwardOnly, 0, 1);
    pushTicks(forwardOnly, af::Button::kRight, 1);
    pushTicks(forwardOnly, 0, 1);
    pushTicks(forwardOnly, af::Button::kX, 1);
    CHECK(!forwardOnly.match(dollar));
}

void testCmdLeadingHoldCount() {
    scenario("cmd/leading-hold-count");
    // The count is honoured when the element is the last step.
    const af::CommandDef charge = def("30$D");
    af::CommandBuffer shortHold, longHold;
    pushTicks(shortHold, af::Button::kDown, 29);
    pushTicks(longHold, af::Button::kDown, 30);
    CHECK(!shortHold.match(charge));
    CHECK(longHold.match(charge));

    // `~10$D, F, x` is the standard charge spelling: hold D for at least 10
    // ticks, release it, then F and x. The two buffers below pin the two ways a
    // charge can fail to qualify -- too short, and never held at all -- and both
    // must be refused. They were not, once: the matcher used to test the release
    // before the hold count, so the duration was dropped and either buffer fired
    // the move. The count is honoured now; these are the regression guards.
    {
        af::CommandBuffer b;
        pushTicks(b, af::Button::kDown, 3);  // far too short a charge
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kRight, 1);
        pushTicks(b, af::Button::kX, 1);
        CHECK(!b.match(def("~10$D, F, x")));   // three ticks is not the ten-tick charge
    }
    {
        af::CommandBuffer b;
        pushTicks(b, 0, 1);                    // D never held at all
        pushTicks(b, af::Button::kRight, 1);
        pushTicks(b, af::Button::kX, 1);
        CHECK(!b.match(def("~10$D, F, x")));   // a charge that was never held is refused
    }

    // And the two negatives above are the shadow of this: a charge that
    // actually charged -- D held ten ticks, let go, then F and x -- fires,
    // which is what proves the two refusals are about the charge and not about
    // the whole form being broken.
    {
        af::CommandBuffer b;
        pushTicks(b, af::Button::kDown, 10);
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kRight, 1);
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kX, 1);
        CHECK(b.match(def("~10$D, F, x")));
    }

    // A charge that does meet its count is honoured even with a neutral tick
    // after it: `5$D, x` with D held for its full five ticks and then released
    // matches, because the counted window closes on the last tick the direction
    // was held -- not on the newest tick -- so the neutral tick before the
    // button is not taken out of the count.
    {
        af::CommandBuffer b;
        pushTicks(b, af::Button::kDown, 5);
        pushTicks(b, 0, 1);
        pushTicks(b, af::Button::kX, 1);
        CHECK(b.match(def("5$D, x")));         // the counted charge is satisfied
    }
}

void testCmdTimeWindow() {
    scenario("cmd/time-window");
    af::CommandList list;
    list.parse("[Command]\n"
               "name = \"wide\"\n"
               "command = D, x\n"
               "time = 5\n"
               "\n"
               "[Command]\n"
               "name = \"tight\"\n"
               "command = D, x\n"
               "time = 1\n"
               "\n"
               "[Command] ; a third block\n"
               "name = \"default_window\"\n"
               "command = D, x\n");

    const af::CommandDef* wide = list.find("wide");
    const af::CommandDef* tight = list.find("tight");
    const af::CommandDef* dflt = list.find("default_window");
    CHECK(wide && tight && dflt);
    CHECK(wide->time == 5 && tight->time == 1 && dflt->time == 15);

    // D, one neutral tick, x: a span of 2, inside 5 and 15 but outside 1.
    af::CommandBuffer b;
    pushTicks(b, 0, 1);
    pushTicks(b, af::Button::kDown, 1);
    pushTicks(b, 0, 1);
    pushTicks(b, af::Button::kX, 1);
    CHECK(b.match(*wide));
    CHECK(!b.match(*tight));
    CHECK(b.match(*dflt));

    // A nonpositive time falls back to the 15-tick default.
    af::CommandList clamped;
    clamped.parse("[Command]\nname = \"neg\"\ncommand = D, x\ntime = -4\n");
    CHECK(clamped.find("neg") != nullptr && clamped.find("neg")->time == 15);

    // The window also cannot read past recorded history.
    af::CommandBuffer shallow;
    pushTicks(shallow, af::Button::kX, 1);
    CHECK(!shallow.match(*wide));  // no D anywhere; at() reads zero
}

void testCmdSharedNames() {
    scenario("cmd/shared-command-names");
    af::CommandList list;
    list.parse("[Command]\n"
               "name = \"QCF_x\"\n"
               "command = D, DF, F, x\n"
               "\n"
               "[Command]\n"
               "name = \"QCF_x\"\n"
               "command = D, x\n"
               "\n"
               "[Command]\n"
               "name = \"taunt\"\n"
               "command = a+b\n");

    // find() returns the first declaration; the name is case sensitive.
    const af::CommandDef* first = list.find("QCF_x");
    CHECK(first != nullptr && first->steps.size() == 4);
    CHECK(list.find("qcf_x") == nullptr);
    CHECK(list.size() == 3);

    // The first declaration's motion fires (rolled with neutral gaps).
    af::CommandBuffer motion;
    pushTicks(motion, 0, 1);
    pushTicks(motion, af::Button::kDown, 1);
    pushTicks(motion, 0, 1);
    pushTicks(motion, af::Button::kDown | af::Button::kRight, 1);
    pushTicks(motion, 0, 1);
    pushTicks(motion, af::Button::kRight, 1);
    pushTicks(motion, 0, 1);
    pushTicks(motion, af::Button::kX, 1);
    CHECK(list.triggered("QCF_x", motion));

    // The *second* declaration fires too: triggered() tries every block that
    // shares the name, not just the first.
    af::CommandBuffer shortcut;
    pushTicks(shortcut, 0, 1);
    pushTicks(shortcut, af::Button::kDown, 1);
    pushTicks(shortcut, 0, 1);
    pushTicks(shortcut, af::Button::kX, 1);
    CHECK(list.triggered("QCF_x", shortcut));

    CHECK(!list.triggered("QCF_x", af::CommandBuffer{}));
    CHECK(!list.triggered("never_declared", motion));

    af::CommandBuffer taunt;
    pushTicks(taunt, 0, 1);
    pushTicks(taunt, af::Button::kA | af::Button::kB, 1);
    CHECK(list.triggered("taunt", taunt));
}

void testCmdFacingRelative() {
    scenario("cmd/facing-relative-directions");
    // Back and forward are the same two physical directions, swapped by
    // facing: facing left (-1) turns screen-left into Forward.
    const af::InputState left{af::Button::kLeft, 0, 0, false};
    const af::InputState right{af::Button::kRight, 0, 0, false};
    CHECK(af::commandSymbols(left, 1) == af::CmdKey::kBack);
    CHECK(af::commandSymbols(right, 1) == af::CmdKey::kForward);
    CHECK(af::commandSymbols(left, -1) == af::CmdKey::kForward);
    CHECK(af::commandSymbols(right, -1) == af::CmdKey::kBack);

    // So the same command string means "toward the opponent" either way.
    const af::CommandDef fwd = def("F, x");
    af::CommandBuffer facingLeft;
    pushTicks(facingLeft, 0, 1, -1);
    pushTicks(facingLeft, af::Button::kLeft, 1, -1);
    pushTicks(facingLeft, 0, 1, -1);
    pushTicks(facingLeft, af::Button::kX, 1, -1);
    CHECK(facingLeft.match(fwd));

    af::CommandBuffer wrongWay;
    pushTicks(wrongWay, 0, 1, 1);
    pushTicks(wrongWay, af::Button::kLeft, 1, 1);
    pushTicks(wrongWay, 0, 1, 1);
    pushTicks(wrongWay, af::Button::kX, 1, 1);
    CHECK(!wrongWay.match(fwd));
}

void testCmdSmoothRoll() {
    scenario("cmd/smooth-roll-motion");
    // The canonical fireball roll D -> DF -> F -> x, pressed with no neutral
    // gaps, is how every player performs `D, DF, F, x`. The engine reads a
    // direction-only step as "pressed" when the set of held directions changes,
    // so each element of the roll is fresh on the tick the next direction joins
    // (DF) or the previous one leaves (F), and the gap-free roll matches. (It
    // did not once: a step used to count as fresh only when the direction was
    // newly *down*, which F never is after DF has it held, so the smooth roll
    // could not chain. The fix landed; this is the guard.) The gap-filled
    // control below matches for the same reason, showing the two spellings are
    // equivalent.
    {
        af::CommandBuffer roll;
        pushTicks(roll, 0, 1);
        pushTicks(roll, af::Button::kDown, 1);
        pushTicks(roll, af::Button::kDown | af::Button::kRight, 1);
        pushTicks(roll, af::Button::kRight, 1);
        pushTicks(roll, af::Button::kX, 1);
        CHECK(roll.match(def("D, DF, F, x")));    // the gap-free roll matches
        CHECK(roll.match(def("~D, DF, F, x")));   // and with the kfm open-release form
    }
    // Control: the same four inputs with neutral gaps matches fine.
    {
        af::CommandBuffer gaps;
        pushTicks(gaps, 0, 1);
        pushTicks(gaps, af::Button::kDown, 1);
        pushTicks(gaps, 0, 1);
        pushTicks(gaps, af::Button::kDown | af::Button::kRight, 1);
        pushTicks(gaps, 0, 1);
        pushTicks(gaps, af::Button::kRight, 1);
        pushTicks(gaps, 0, 1);
        pushTicks(gaps, af::Button::kX, 1);
        CHECK(gaps.match(def("D, DF, F, x")));
    }
}

}  // namespace

int main() {
    testPrecedence();
    testTwoWordNames();
    testCommandNames();
    testRedirects();
    testIfelseCond();
    testStrings();
    testAssignment();
    testHitdefattr();
    testRobustness();
    testNoShortCircuit();

    testCmdParseElements();
    testCmdChainOperator();
    testCmdStartSymbol();
    testCmdSlashHold();
    testCmdTildeRelease();
    testCmdPressVsHold();
    testCmdDollarDirections();
    testCmdLeadingHoldCount();
    testCmdTimeWindow();
    testCmdSharedNames();
    testCmdFacingRelative();
    testCmdSmoothRoll();

    std::printf("%d checks, %d failed\n", gChecks, gFailures);
    if (gFailures == 0) std::printf("ok\n");
    return gFailures == 0 ? 0 : 1;
}
