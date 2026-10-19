// The little expression language MUGEN triggers are written in.
//
// Almost everything a character does is guarded by text like
//
//     trigger1 = command = "QCF_x" && statetype != A && time > 3
//     trigger1 = ifelse(var(4), p2bodydist X < 40, anim = 200)
//     trigger1 = enemynear, vel x < -1
//
// so a character's move set is only reachable at all once this can be read.
// The language is small -- numbers, strings, the usual arithmetic and
// comparison operators, `&&`/`||`/`^^`, parentheses, a few dozen named values
// and a handful of functions -- but it has two quirks that a generic expression
// parser gets wrong, and both are handled here:
//
//   * Some names contain a space. `pos y`, `vel x`, `p2bodydist X` and
//     friends are single values, not a value followed by a stray word, so the
//     scanner glues a known prefix onto the axis that follows it.
//   * `redirect, trigger` re-evaluates the trigger on another entity --
//     `enemynear, vel x`, `p2, statetype`, `root, var(0)`. The comma is not an
//     argument separator at the top level, and the redirect binds tighter than
//     any operator, so `enemynear, vel x < 0` means "their x velocity is
//     negative", not "their velocity, which is less than zero".
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace af {

// A trigger's result. MUGEN has both numbers and strings, and `command = "x"`
// is a string comparison, so the type has to be carried rather than assumed.
struct Value {
    bool isString = false;
    float num = 0.0f;
    std::string str;

    static Value number(float v) { return Value{false, v, {}}; }
    static Value boolean(bool v) { return Value{false, v ? 1.0f : 0.0f, {}}; }
    static Value text(std::string s) { return Value{true, 0.0f, std::move(s)}; }

    // A string is truthy when it is non-empty, a number when it is non-zero --
    // which is what makes a bare `command = "x"` usable as a condition and a
    // bare `numenemy` usable as one too.
    [[nodiscard]] bool truthy() const { return isString ? !str.empty() : num != 0.0f; }
    [[nodiscard]] float toFloat() const { return isString ? (str.empty() ? 0.0f : 1.0f) : num; }
    [[nodiscard]] int toInt() const { return static_cast<int>(toFloat()); }
};

// What an expression is evaluated against. The state machine implements this;
// tests implement it with a stub.
class TriggerContext {
public:
    virtual ~TriggerContext() = default;

    // `name` is normalised to lower case with runs of space collapsed, so the
    // implementation can compare against "pos x" and "p2bodydist x" directly.
    //
    // `args` holds the parenthesised arguments, empty for a bare value. A
    // redirect is delivered as two calls rather than one: the outer name is the
    // redirect target ("enemynear", "p2", "root") with a single argument that
    // is the inner trigger, so an implementation that does not support a
    // redirect can still return a sensible zero for it.
    virtual Value trigger(std::string_view name, const std::vector<Value>& args) const = 0;

    // The context to evaluate a `redirect, ...` against: `enemynear` and `p2`
    // give the opponent, `root` the character this one came from, `self` this
    // one. `selector` carries the optional argument of the forms that take one --
    // `helper(4000), ...`, `playerid(34), ...`, `enemy(2), ...` -- and is -1 for a
    // bare redirect. Returning nullptr makes the redirect evaluate to zero, which
    // is what a redirect the implementation does not support should do -- silently,
    // and without taking the rest of the expression down with it.
    virtual const TriggerContext* redirect(std::string_view target, int selector) const {
        (void)target;
        (void)selector;
        return nullptr;
    }

    // `var(0) := 4.5` evaluates from an expression: the write. Kind is one of
    // 'v' (var), 'f' (fvar), 's' (sysvar), 'S' (sysfvar). False when the write
    // is not understood -- the expression still gets the assigned value, which
    // is what keeps `trigger1 = var(0) := x > 3` evaluable everywhere.
    virtual bool assignVar(char kind, int index, float value) const {
        (void)kind;
        (void)index;
        (void)value;
        return false;
    }
};

// A parsed expression. Cheap to copy, and safe to evaluate repeatedly.
//
// `eval` is a method name, not dynamic evaluation: this file parses a fixed,
// closed grammar of numbers, operators and a whitelist of named triggers, and
// there is no way to reach a function, a symbol or a byte of memory that the
// whitelist does not name. Nothing here executes character-authored code --
// MUGEN characters are data, and this is the reader that keeps them data.
//
// A failed parse yields `valid() == false` and evaluates to zero everywhere,
// which is the right failure for this format: a trigger that cannot be
// understood must not fire, and a fighter that cannot be understood is still
// better than a fighter that crashes the match.
class Expr {
public:
    // One node of the tree. Public so the parser in the .cpp can build the
    // array directly; nothing outside this header has any business reading it.
    struct Node {
        enum class Kind : uint8_t {
            Constant,
            String,
            Trigger,
            Unary,
            Binary,
            Redirect,
            Assign,
            Interval,
        };

        Kind kind = Kind::Constant;
        float number = 0.0f;
        std::string text;      // trigger name, operator, or string payload
        std::vector<int> args; // child indices; -1 never appears

        // `Interval` only: whether each end of the range is included, which is
        // what the four bracket spellings (`[a,b]`, `[a,b)`, `(a,b]`, `(a,b)`)
        // differ by. `text` holds the operator the interval appeared under,
        // either "=" or "!=" -- the only two MUGEN allows in front of one.
        bool lowClosed = false;
        bool highClosed = false;
    };

    Expr() = default;

    static Expr parse(std::string_view text);

    // Takes ownership of a finished node array. `root` indexes the node the
    // expression evaluates from -- not necessarily 0, because the parser emits
    // a parent after its children, so the root is the last node it built.
    static Expr fromNodes(std::vector<Node> nodes, int root) {
        Expr out;
        out.nodes_ = std::move(nodes);
        out.root_ = root;
        return out;
    }

    [[nodiscard]] bool valid() const { return !nodes_.empty(); }
    [[nodiscard]] bool empty() const { return nodes_.empty(); }

    Value eval(const TriggerContext& context) const;
    [[nodiscard]] bool evalBool(const TriggerContext& context) const {
        return eval(context).truthy();
    }

    // The names this expression reads, for diagnostics and for the test that
    // asserts the scanner did not silently split a two-word trigger.
    [[nodiscard]] std::vector<std::string> names() const;

private:
    std::vector<Node> nodes_;
    int root_ = 0;  // index into nodes_; the expression evaluates from here
};

}  // namespace af
