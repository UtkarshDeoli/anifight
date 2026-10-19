// Checks the trigger expression language (engine/expr).
//
// The two rules worth pinning are the ones a generic expression parser gets
// wrong on real character files, and both fail *quietly* -- a mis-split name
// evaluates to zero, a mis-grouped redirect evaluates to something plausible --
// so a move simply never happens and nothing is logged.
//
//   1. Names with a space: `pos x`, `vel x`, `p2bodydist X`. If the scanner
//      splits these into two identifiers, the leading one becomes an unknown
//      trigger (zero) and the axis word becomes another, and every trigger
//      written against position or velocity silently reads as 0.
//   2. `redirect, trigger` binds tighter than any operator, so
//      `enemynear, vel x < 0` asks about *their* velocity, not ours. Grouping
//      it the other way round compares a redirect to a number and is true
//      almost always, which is worse than failing: it makes moves fire at the
//      wrong times rather than not at all.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_expr \
//         tools/hosttest/test_expr.cpp app/src/main/cpp/engine/expr.cpp
//     /tmp/test_expr

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "engine/expr.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++failures;
}

// A stub whose triggers are a plain name -> value table, so a test can say
// exactly what `pos y` is without standing up a whole fighter.
class Stub : public af::TriggerContext {
public:
    af::Value trigger(std::string_view name, const std::vector<af::Value>& args) const override {
        asked.emplace_back(name);
        // `command` answers by its *argument*, which is the whole point of the
        // syntax `command = "QCF_x"`: the state machine is handed the command's
        // name and tells the character whether that command just fired. A stub
        // that answered by name alone could not tell the two strings apart and
        // would make the assertions below meaningless.
        if (name == "command") {
            lastCommand = args.empty() ? std::string() : args[0].str;
            return af::Value::boolean(lastCommand == "QCF_x");
        }
        const auto it = values.find(std::string(name));
        if (it != values.end()) return it->second;
        // Functions the tests use, implemented directly against the stub.
        if (name == "ifelse" && args.size() == 3) return args[0].truthy() ? args[1] : args[2];
        if (name == "abs" && args.size() == 1) return af::Value::number(std::fabs(args[0].toFloat()));
        // `Const(name)` and `GetHitVar(name)` take a *name* and answer by it, so
        // the stub answers by the string it was handed: that is what tells the
        // test whether the name survived the parser as a name.
        if (name == "const" && args.size() == 1) {
            lastArg = args[0].str;
            return af::Value::number(args[0].str == "data.life" ? 1000.0f : 0.0f);
        }
        if (name == "gethitvar" && args.size() == 1) {
            lastArg = args[0].str;
            return af::Value::number(args[0].str == "yaccel" ? 0.35f : 0.0f);
        }
        return af::Value::number(0.0f);
    }

    const af::TriggerContext* redirect(std::string_view target, int selector) const override {
        const auto it = redirects.find(std::string(target));
        return it == redirects.end() ? nullptr : it->second;
    }

    std::map<std::string, af::Value> values;
    std::map<std::string, const af::TriggerContext*> redirects;
    mutable std::vector<std::string> asked;
    // The name the last `command = "..."` handed over, so a test can assert the
    // argument arrived rather than merely that a call happened.
    mutable std::string lastCommand;
    // Likewise for the triggers whose first argument is a name.
    mutable std::string lastArg;
};

bool truth(const char* text, const Stub& stub) {
    return af::Expr::parse(text).evalBool(stub);
}

float number(const char* text, const Stub& stub) {
    return af::Expr::parse(text).eval(stub).toFloat();
}

// True when `name` is exactly one of the names the expression reads.
bool readsName(const char* text, const std::string& name) {
    for (const std::string& read : af::Expr::parse(text).names()) {
        if (read == name) return true;
    }
    return false;
}

}  // namespace

int main() {
    Stub stub;
    stub.values["time"] = af::Value::number(12);
    stub.values["stateno"] = af::Value::number(200);
    stub.values["pos x"] = af::Value::number(30);
    stub.values["pos y"] = af::Value::number(-8);
    stub.values["vel x"] = af::Value::number(2.5f);
    stub.values["vel y"] = af::Value::number(0);
    stub.values["statetype"] = af::Value::text("A");
    stub.values["name"] = af::Value::text("kfm");
    stub.values["var 4"] = af::Value::number(3);
    // The animation clock the AnimElem second form is compared against.
    stub.values["animelemtime"] = af::Value::number(4);

    // ---------------------------------------------------------------- names
    check(readsName("pos y < 0", "pos y"),
          "`pos y` scans as one name rather than two");
    check(!readsName("pos y < 0", "pos"), "and not as the bare word `pos`");
    check(!readsName("pos y < 0", "y"), "and not as a bare `y`");
    check(truth("pos y < 0", stub), "`pos y < 0` reads the position, not zero");
    check(truth("vel x > 2", stub), "`vel x > 2` reads the velocity");
    check(readsName("p2bodydist X < 60", "p2bodydist x"),
          "a two-word name is case-folded to lower case");
    check(!af::Expr::parse("pos y < 0").valid() == false, "a normal expression parses");

    // ------------------------------------------------------------- operators
    check(number("2 + 3 * 4", stub) == 14.0f, "multiplication binds tighter than addition");
    check(number("(2 + 3) * 4", stub) == 20.0f, "parentheses override precedence");
    check(number("-3 + 1", stub) == -2.0f, "unary minus applies");
    check(number("10 / 4", stub) == 2.5f, "division is not integer division");
    check(number("10 % 4", stub) == 2.0f, "modulo works");
    check(number("7 / 0", stub) == 0.0f, "division by zero yields zero rather than a crash");
    check(truth("1 < 2 && 3 >= 3", stub), "comparison chains under &&");
    check(!truth("1 < 2 && 3 > 4", stub), "&& requires both sides");
    check(truth("1 > 2 || 3 > 2", stub), "|| requires one side");
    check(truth("1 > 2 ^^ 3 > 2", stub), "^^ is exclusive or");
    check(!truth("1 < 2 ^^ 3 > 2", stub), "^^ is false when both sides are true");
    check(truth("!(1 > 2)", stub), "! negates");
    check(truth("time > 3 && stateno = 200", stub), "a compound state trigger");

    // --------------------------------------------- the precedence table
    // MUGEN publishes the whole thing (cns.html, "Precedence and associativity
    // of operators"), and two of its levels are not where a C programmer would
    // guess, so both are pinned here.
    check(number("2 ** 3", stub) == 8.0f, "** is the power operator");
    // Only unary operators and `:=` associate right to left, so this is
    // (2**3)**2 and not 2**(3**2).
    check(number("2 ** 3 ** 2", stub) == 64.0f, "and evaluates left to right");

    // Relational binds tighter than equality, so this is `0 = (1 < 2)`, which
    // is false -- the other grouping would make it true.
    check(!truth("0 = 1 < 2", stub), "`<` binds tighter than `=`");

    // `:=` binds tighter than the logical operators, not looser -- which is the
    // reading the documentation's own worked example uses, and the reading the
    // corpus needs for `life >= 800 && life <= 1000 && var(32) := 0`. What the
    // assignment *writes* is pinned in test_expr_cmd.cpp, which carries the
    // context that records writes; here the point is that the line parses at
    // all, since an unparsed trigger reads as false and the controller it guards
    // never runs.
    check(af::Expr::parse("life >= 800 && life <= 1000 && var(32) := 0").valid(),
          "an assignment may end an && chain");
    check(af::Expr::parse("var(0) := 1 && 0").valid(), "and stand inside one");

    // `&`, `^` and `|` are the *bitwise* operators the documentation lists, and
    // files write them as if they were the logical ones -- which gives the same
    // answer whenever both operands are 0/1, as they are in `stateno = 200 | ...`.
    check(number("5 & 3", stub) == 1.0f, "& is bitwise and");
    check(number("5 | 3", stub) == 7.0f, "| is bitwise or");
    check(number("5 ^ 3", stub) == 6.0f, "^ is bitwise xor");
    check(number("~0", stub) == -1.0f, "~ is bitwise not");
    check(truth("(stateno = 200 | stateno = 5)", stub),
          "a one-character | works as a logical or on 0/1 operands");
    check(truth("(stateno = 200 & time > 0)", stub), "and a one-character &");
    check(!truth("(stateno = 5 & time > 0)", stub), "and is false when either side is");

    // ------------------------------------------------------------- functions
    check(number("ifelse(1, 7, 9)", stub) == 7.0f, "ifelse takes the true branch");
    check(number("ifelse(0, 7, 9)", stub) == 9.0f, "ifelse takes the false branch");
    check(number("abs(0 - 5)", stub) == 5.0f, "abs of a negative");

    // --------------------------------------------------------------- strings
    check(truth("name = \"kfm\"", stub), "a string comparison matches");
    check(!truth("name = \"akari\"", stub), "and does not match a different string");
    check(truth("name != \"akari\"", stub), "string inequality");

    // MUGEN's `command = "..."` is the one comparison that is not a comparison:
    // the string is the trigger's argument. Read the other way it evaluates to
    // false always, which silently makes every move of every character
    // unreachable -- so both halves of it are pinned here.
    stub.lastCommand.clear();
    check(truth("command = \"QCF_x\"", stub),
          "`command = \"x\"` is true when the command fired");
    check(stub.lastCommand == "QCF_x",
          "and the name is handed to the trigger as its argument");
    check(!truth("command = \"QCF_y\"", stub),
          "and false when a different command is the one that fired");
    check(truth("command != \"QCF_y\"", stub),
          "`command != \"x\"` is the negation of the same trigger");

    // The name is often written without quotes, and the case has to survive:
    // command names are case-sensitive, so a scanner that folds `QCF_x` to
    // `qcf_x` silently unhooks the move.
    stub.lastCommand.clear();
    check(truth("command = QCF_x", stub), "a bare command name works too");
    check(stub.lastCommand == "QCF_x", "and arrives with its capitals intact");
    check(!truth("command = QCF_y", stub), "and a bare name that did not fire is false");

    // -------------------------------------------------------------- redirects
    Stub enemy;
    enemy.values["vel x"] = af::Value::number(-3);
    enemy.values["statetype"] = af::Value::text("S");
    enemy.values["stateno"] = af::Value::number(5050);
    stub.redirects["enemynear"] = &enemy;
    stub.redirects["p2"] = &enemy;
    stub.redirects["enemy"] = &enemy;
    stub.redirects["partner"] = &enemy;

    // The point of the redirect rule: this must ask about the *enemy's*
    // velocity. Ours is +2.5, theirs is -3, so the two groupings disagree and
    // the test can tell them apart.
    check(truth("enemynear, vel x < 0", stub), "a redirect reads the other entity's value");
    check(!truth("enemynear, vel x > 0", stub), "and not our own (+2.5)");
    check(truth("p2, statetype = \"S\"", stub), "a redirect through p2 works");
    check(truth("vel x > 0", stub), "the unredirected form still reads ours");

    // `enemy` and `partner` are documented redirect targets and by far the
    // commonest of all -- `triggerall = enemy, stateno != 5050` is how a
    // character asks what the opponent is doing. They were missing from the
    // recognised list, so the target parsed as a bare value and the comma
    // behind it stranded the rest of the line: 898 corpus lines carry a
    // comparison behind one of these two, all of them silently false.
    check(truth("enemy, stateno = 5050", stub), "`enemy` is a redirect target");
    check(!truth("enemy, stateno != 5050", stub), "and reads the enemy's state, not ours");
    check(truth("enemy, stateno != [200,650]", stub),
          "a range behind a redirect is compared against the enemy's value");
    check(truth("partner, stateno = 5050 ; with a trailing comment", stub),
          "`partner` is a redirect target too, and a redirected comparison may carry a comment");
    check(truth("vel x = [2.4, 2.6]", stub),
          "the unredirected range still reads our own value");

    // A redirect selector is the same thing with an argument: `enemy(1)`,
    // `helper(4000)`, `playerid(34)`.
    check(truth("enemy(1), vel x < 0", stub), "a redirect with a selector parses");

    // `TimeMod [oper] divisor, value1` is the one comparison whose right side is
    // a *pair*, and the docs say the `%` operator subsumes it. Read as an
    // ordinary comparison the `, value` was left over and the whole line failed
    // to parse -- 984 of them in the corpus.
    check(truth("timemod = 4, 0", stub), "TimeMod = divisor, value is the modulo test");
    check(!truth("timemod = 4, 1", stub), "and is false when the remainder differs");
    check(truth("timemod = 5, 2", stub), "12 % 5 is 2");
    check(!truth("timemod != 5, 2", stub), "and negates");
    check(truth("timemod < 2, 1", stub), "the operator is one of the six comparisons");
    check(truth("time > 0 && timemod = 4, 0", stub), "and combines with the rest");

    // A redirect to something that does not exist must be zero and must not
    // take the rest of the expression down: `a && b` with b false is false,
    // while `a || b` with b false is a.
    check(!truth("root, vel x > 0", stub), "an unknown redirect is false");
    check(truth("time = 12 || root, vel x > 0", stub),
          "an unknown redirect does not poison the other side of an ||");

    // ------------------------------------------- triggers that take a name
    // `Const(data.life)` and `GetHitVar(yaccel)` are written with the parameter
    // *bare*, and it is not an expression: it names no trigger. Parsed as one it
    // became a call to an unknown trigger, which is zero, so the name arrived at
    // the handler empty and every one of these read as 0 -- silently, and in a
    // controller parameter as much as in a trigger.
    check(readsName("Const(data.life) > 0", "const"),
          "a bare name parses as an argument, not as a name of its own");
    check(!readsName("Const(data.life) > 0", "data.life"),
          "the constant's name is not looked up as a trigger");
    check(stub.lastArg.empty(), "and nothing has been asked yet");
    check(truth("Const(data.life) > 0", stub), "Const(data.life) reads the constant");
    check(stub.lastArg == "data.life", "the name reaches the handler intact");
    check(number("Const(Data.Life)", stub) == 1000.0f,
          "and case-folded, since that is how the constant table is keyed");
    check(number("Const(data.life)", stub) == 1000.0f, "and returns its value");
    check(number("GetHitVar(yaccel)", stub) == 0.35f,
          "GetHitVar(yaccel) returns the hit's own value, not zero");
    check(stub.lastArg == "yaccel", "a six-letter name is not truncated");
    check(number("GetHitVar(Yaccel)", stub) == 0.35f, "and case does not matter");
    // The name-taking rule is narrow: `ifelse` and `abs` still take expressions,
    // and `PlayerIDExist` takes an ID number rather than a name.
    check(number("ifelse(1, 40, 0)", stub) == 40.0f, "ifelse still takes expressions");
    check(number("abs(-3)", stub) == 3.0f, "abs still takes expressions");
    check(readsName("PlayerIDExist(var(4))", "var"),
          "a non-name argument is still parsed as an expression");

    // `AnimElem = value1, [oper] value2` is the documented second form of the
    // trigger, and it asks the question `AnimElemTime(value1) [oper] value2`
    // asks -- how long the animation has been inside that element. The comma
    // stranded the rest of the line, so `animelem = 5,>=5` was a parse failure
    // and read as false.
    check(readsName("animelem = 2, >= 0", "animelemtime"),
          "`AnimElem = n, [oper] v` reads as the AnimElemTime form");
    check(!readsName("animelem = 2, >= 0", "animelem"),
          "and not as a plain AnimElem test of the element number");
    check(truth("animelem = 2, >= 0", stub), "the comparison is against that form's value");
    check(!truth("animelem = 2, > 5", stub), "and is false when the operator says so");
    check(truth("animelem = 2, <= 4", stub), "the other operators work too");
    check(truth("animelem = 2, != 5", stub), "including the negated equality");

    // -------------------------------------------------------------- intervals
    // `x = [y,z]` is a range test rather than a comparison of two values, and it
    // is the spelling Elecbyte's own common1.cns uses to hand a falling victim
    // over to the ground. Before this the bracket was not even a token, so the
    // whole trigger failed to parse and read as false.
    check(truth("stateno = [200,650]", stub), "a closed range includes its low end");
    check(truth("stateno = [150,200]", stub), "and its high end");
    check(!truth("stateno = [201,650]", stub), "and excludes what is below it");
    check(!truth("stateno = (200,650)", stub), "an open range excludes its low end");
    check(truth("stateno = (199,650)", stub), "but includes what is inside it");
    check(truth("stateno = [200,650)", stub), "a half-open range takes the closed end");
    check(!truth("stateno = (199,200)", stub), "and respects the open one");
    check(truth("stateno = (199,200]", stub), "the other half-open spelling works too");
    check(!truth("stateno != [150,300]", stub), "a negated range is false inside it");
    check(truth("stateno != [201,650]", stub), "and true outside it");
    check(!truth("stateno != (199,201)", stub), "the negated open range is false inside");
    check(truth("stateno != (200,300)", stub), "and true at the excluded end");
    check(truth("vel x = [2.4, 2.6]", stub), "ranges compare floats, not just integers");
    check(truth("pos y = [-10, -5]", stub), "and negative bounds");
    check(truth("time = [10 + 2, 20]", stub), "the bounds may be expressions");
    check(truth("stateno = [200,650] && time = 12", stub),
          "a range combines with the rest of the expression");
    check(!truth("stateno = [200,650] && time = 11", stub),
          "and is not simply always true");
    // The docs' own example of where an interval may sit: inside a group, on the
    // right of the subexpression, and not in the middle of a comparison chain.
    check(!truth("(1 = [0,2]) = (0,1)", stub),
          "an interval may appear on the right of a parenthesised subexpression");
    check(truth("(1 + 1) = (1,3)", stub), "a group that is not a range still parses");
    check(number("(2 + 3) * 4", stub) == 20.0f, "and still groups arithmetic");
    check(!af::Expr::parse("stateno = [200,650").valid(),
          "an unterminated range is a parse failure, not a silent zero");

    // ------------------------------------------------------- failure modes
    check(!af::Expr::parse("").valid(), "an empty trigger is not valid");
    check(!af::Expr::parse("   ").valid(), "a whitespace-only trigger is not valid");
    check(!af::Expr::parse("1 +").valid(), "a malformed trigger is not valid");
    check(!af::Expr::parse("(1 + 2").valid(), "an unbalanced parenthesis is not valid");
    check(!truth("1 +", stub), "a malformed trigger evaluates to false, not to something");
    check(!truth("some.unknown.trigger > 0", stub),
          "an unknown trigger is zero rather than an error");

    // A trailing comment is how half of all real trigger lines end.
    check(truth("time = 12 ; and this is a comment", stub),
          "a trailing comment is ignored");

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}
