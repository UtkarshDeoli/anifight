// The .cns / .cmd state machine, as data.
//
// A character's behaviour is a list of numbered states, each holding an ordered
// list of controllers, each guarded by triggers. That is the whole language:
//
//     [Statedef 1000]                 what the state is
//     type = A
//     movetype = A
//     physics = N
//     anim = 1000
//
//     [State 1000, 1]                 what the state does, in order
//     type = VelSet
//     trigger1 = animelem = 2
//     x = 4
//
// Nothing here interprets any of it -- this file only reads the shape. The
// running of it is engine/statemachine.
//
// Two details of the format matter enough to be worth stating up front:
//
//   * Controller parameters are expressions, not literals. `x = 4` is the easy
//     case; `value = ifelse(var(3), 200, 201)` and `x = 3 * const(velocity.run
//     .fwd)` are just as legal and common. So every parameter keeps its raw
//     text and is parsed as an expression on first use -- lazily, because a
//     character has tens of thousands of controllers and most of them are in
//     states that never run.
//   * Some parameters are comma-separated *lists* rather than expressions
//     (`attr = S, NA`, `damage = 30, 40`, `offset = 0, -20`). Those fail to
//     parse as expressions, which is fine and detectable: the raw text is
//     still there and the list readers use it.
#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <vector>

#include "core/ini.h"
#include "engine/expr.h"

namespace af {

// MUGEN's state-type letters, kept as the letters themselves so a parsed value
// can be printed back into a log that matches the file.
enum class StateType : uint8_t { Stand, Crouch, Air, Liedown };
enum class MoveType : uint8_t { Idle, Attack, Hit, Unchanging };
enum class Physics : uint8_t { Stand, Crouch, Air, None };

[[nodiscard]] StateType parseStateType(std::string_view text, StateType fallback);
[[nodiscard]] MoveType parseMoveType(std::string_view text, MoveType fallback);
[[nodiscard]] Physics parsePhysics(std::string_view text, Physics fallback);
[[nodiscard]] const char* stateTypeName(StateType type);
[[nodiscard]] const char* moveTypeName(MoveType type);
[[nodiscard]] const char* physicsName(Physics physics);

// One controller: a type, some parameters, and the triggers that gate it.
class Controller {
public:
    void parse(const IniSection& section);

    // The `id` half of `[State <stateno>, <id>]`, kept only so a controller can
    // be named in a log. Authors label these usefully -- "Stand", "Combo
    // condition Check", "Hit Detection" -- which makes them the fastest way to
    // find out what a character is actually doing.
    void setId(std::string_view id) { id_ = std::string(trim(id)); }
    [[nodiscard]] const std::string& id() const { return id_; }

    [[nodiscard]] const std::string& type() const { return type_; }
    [[nodiscard]] bool empty() const { return type_.empty(); }

    // Raw text of a parameter, or an empty view. This is what the list-valued
    // parameters are read through.
    [[nodiscard]] std::string_view raw(std::string_view key) const;

    // Whether the parameter is present at all, which is how a controller tells
    // `ctrl = 0` from an absent `ctrl`.
    [[nodiscard]] bool has(std::string_view key) const { return find(key) != nullptr; }

    [[nodiscard]] int intParam(std::string_view key, int fallback) const;
    [[nodiscard]] float floatParam(std::string_view key, float fallback) const;
    [[nodiscard]] bool boolParam(std::string_view key, bool fallback) const;

    // Evaluates a parameter in the given context. `fallback` is returned when
    // the parameter is absent or is not an expression.
    [[nodiscard]] float evalNumber(std::string_view key, const TriggerContext& context,
                                   float fallback = 0.0f) const;
    [[nodiscard]] Value evalValue(std::string_view key, const TriggerContext& context,
                                  Value fallback = Value::number(0.0f)) const;

    // Whether this controller runs: every `triggerall` holds, and the numbered
    // triggers let it through.
    //
    // The numbered triggers are the subtle part. They group by *number*: the
    // triggers sharing a number are ANDed and the groups are ORed. A character
    // writes
    //
    //     trigger1 = command = "FF"
    //     trigger1 = statetype = S
    //     trigger1 = ctrl
    //
    // and means all three -- but it writes
    //
    //     trigger1 = stateno = 100
    //     trigger2 = stateno = 105
    //
    // and means either. Reading the numbers as decoration and ORing everything
    // is not a small mistake: that `trigger1 = ctrl` above would fire a
    // double-tap dash on every tick the fighter was standing still, and half the
    // characters on disk are written this way.
    //
    // A controller with no triggers at all never runs -- MUGEN requires one, and
    // a controller that silently ran unconditionally would be the worst possible
    // reading of a half-written state.
    [[nodiscard]] bool triggered(const TriggerContext& context) const;
    [[nodiscard]] bool hasTriggers() const { return !triggerGroups_.empty(); }

    // `persistent = n`: at most one run per n ticks while its triggers hold. MUGEN's
    // default is 1 (checked every tick), so the cheap path is the common one.
    [[nodiscard]] int persistent() const { return intParam("persistent", 1); }
    // `ignorehitpause = 1`: the controller runs even while the fighter is frozen
    // by a hit. Spark and meter side effects during a connect are written this way.
    [[nodiscard]] bool ignoresHitPause() const { return intParam("ignorehitpause", 0) != 0; }

    // For diagnostics: how many trigger groups gate this controller.
    [[nodiscard]] size_t triggerCount() const { return triggerGroups_.size(); }

private:
    // A parameter plus its expression, parsed the first time it is asked for.
    // Mutable because evaluation is const and the cache is not part of the
    // value -- and because a character has far more controllers than it has
    // controllers that ever run.
    struct Param {
        std::string key;
        std::string raw;
        mutable Expr expr;
        mutable bool parsed = false;
    };

    [[nodiscard]] const Param* find(std::string_view key) const;
    [[nodiscard]] const Expr* exprFor(const Param& param) const;

    std::string type_;
    std::string id_;
    std::vector<Param> params_;
    std::vector<Expr> triggerAll_;  // all must hold
    // Keyed by the number in `triggerN`, because that number is the grouping:
    // same number ANDs, different numbers OR. A map keeps the groups in the
    // order the author numbered them.
    std::map<int, std::vector<Expr>> triggerGroups_;
};

// One numbered state.
struct StateDef {
    int number = 0;

    // True when the file wrote a `[Statedef n]` header for this state, as
    // opposed to only attaching controllers to it or only naming it as the
    // target of a ChangeState. The difference matters: MUGEN has built-in
    // behaviour for the states it defines itself, and a character that leaves
    // state 20 out inherits MUGEN's walking rather than an inert state.
    bool defined = false;

    StateType type = StateType::Stand;
    MoveType moveType = MoveType::Idle;
    Physics physics = Physics::Stand;
    // `type = U`, `movetype = U`, `physics = U` mean *unchanged* rather than any
    // particular value: the state keeps whatever the fighter already had. It is
    // the usual spelling in the states a character wants to be transparent --
    // a hit state that must not disturb the hit state it was entered from --
    // and reading `U` as "stand" would reset exactly what it was there to keep.
    bool typeUnchanged = false;
    bool moveTypeUnchanged = false;
    bool physicsUnchanged = false;

    // `ctrl` and whether the header actually said so. A state that leaves it
    // out is not the same as one that sets it to 0: MUGEN's own standing state
    // carries no `ctrl` line and is controllable, because control is the
    // engine's default for it -- see StateMachine::enterState.
    bool ctrl = false;
    bool hasCtrl = false;

    // -1 means "keep whatever animation is playing", which is MUGEN's default
    // for a state that does not name one.
    int anim = -1;
    bool hasAnim = false;
    // `anim = 40+var(49)` is a legal header and real characters write it, so
    // the raw text is kept for the runtime to evaluate when it is not a plain
    // number. `anim` is the fast path, which is nearly every header.
    std::string animRaw;

    bool hasVelSet = false;
    float velX = 0.0f;
    float velY = 0.0f;

    bool hasPowerAdd = false;
    int powerAdd = 0;

    bool hasJuggle = false;
    int juggle = 0;

    // -1 means "do not change it".
    int sprPriority = -1;

    bool hasFaceP2 = false;
    bool faceP2 = false;

    // `movehitpersist`/`hitcountpersist`/`hitdefpersist`: whether the combat
    // bookkeeping (movecontact/movehit/movereversed, hitcount, armed HitDef)
    // survives entering this state. MUGEN resets all three by default, and the
    // binary's statedef record pays that out at entry.
    bool moveHitPersist = false;
    bool hitCountPersist = false;
    bool hitDefPersist = false;

    std::vector<Controller> controllers;
};

// Every state a character declares. A character's states can be split across
// its .cns *and* its .cmd (which conventionally carries states -3, -2, -1 and
// any states the author keeps with the commands), so this merges documents
// rather than replacing them.
class StateLibrary {
public:
    void parse(std::string_view text);

    [[nodiscard]] const StateDef* find(int number) const;
    [[nodiscard]] bool has(int number) const { return find(number) != nullptr; }
    [[nodiscard]] size_t size() const { return states_.size(); }
    [[nodiscard]] size_t controllerCount() const;

    // Whether any state in the library carries a controller of this type, named
    // in MUGEN's own spelling and matched case-insensitively (`HitFallDamage`).
    // The library is the only thing that knows a character's whole controller
    // set, and a few engine-level questions are really questions about it -- the
    // fall landing is the one that needs it: a file that runs `HitFallDamage`
    // itself must be the one that charges the fall, so whoever applies the
    // engine's built-in landing has to know whether the file took over. See
    // `StateMachine::scanLanding`.
    [[nodiscard]] bool usesController(std::string_view type) const;

private:
    std::unordered_map<int, StateDef> states_;
};

}  // namespace af
