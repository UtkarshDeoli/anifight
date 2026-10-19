#include "engine/cns.h"

#include <cctype>
#include <limits>

namespace af {

namespace {

// `[statedef 200]` and `[state 200, 3]`. The number is the first token either
// way; only the section name differs, in the spelling and in whether a comma
// follows.
bool parseIntPrefix(std::string_view text, int& out) {
    return tryParseInt(trim(text), out);
}

std::string lower(std::string_view text) { return toLower(text); }

// Whether a state's type/physics letter is `U`, MUGEN's "leave this alone".
bool isUnchanged(std::string_view text) {
    const std::string t = lower(trim(text));
    return !t.empty() && t[0] == 'u';
}

}  // namespace

StateType parseStateType(std::string_view text, StateType fallback) {
    const std::string t = lower(trim(text));
    if (t.empty()) return fallback;
    switch (t[0]) {
        case 's': return StateType::Stand;
        case 'c': return StateType::Crouch;
        case 'a': return StateType::Air;
        case 'l': return StateType::Liedown;
        default: return fallback;
    }
}

MoveType parseMoveType(std::string_view text, MoveType fallback) {
    const std::string t = lower(trim(text));
    if (t.empty()) return fallback;
    switch (t[0]) {
        case 'i': return MoveType::Idle;
        case 'a': return MoveType::Attack;
        case 'h': return MoveType::Hit;
        // MUGEN writes `movetype = U` (unchanged) and `movetype = S` (stand,
        // a synonym for idle) in different generations of the format.
        case 'u': return MoveType::Unchanging;
        case 's': return MoveType::Idle;
        default: return fallback;
    }
}

Physics parsePhysics(std::string_view text, Physics fallback) {
    const std::string t = lower(trim(text));
    if (t.empty()) return fallback;
    switch (t[0]) {
        case 's': return Physics::Stand;
        case 'c': return Physics::Crouch;
        case 'a': return Physics::Air;
        case 'n': return Physics::None;
        default: return fallback;
    }
}

const char* stateTypeName(StateType type) {
    switch (type) {
        case StateType::Stand: return "S";
        case StateType::Crouch: return "C";
        case StateType::Air: return "A";
        case StateType::Liedown: return "L";
    }
    return "S";
}

const char* moveTypeName(MoveType type) {
    switch (type) {
        case MoveType::Idle: return "I";
        case MoveType::Attack: return "A";
        case MoveType::Hit: return "H";
        case MoveType::Unchanging: return "U";
    }
    return "I";
}

const char* physicsName(Physics physics) {
    switch (physics) {
        case Physics::Stand: return "S";
        case Physics::Crouch: return "C";
        case Physics::Air: return "A";
        case Physics::None: return "N";
    }
    return "S";
}

// ------------------------------------------------------------------ controller

void Controller::parse(const IniSection& section) {
    for (const IniLine& line : section.lines) {
        if (line.key.empty()) continue;

        // Triggers are collected separately from parameters because they are
        // not parameters: `triggerall` lines all have to hold and the numbered
        // ones only need one to. They also repeat, which is exactly why the
        // ini parser keeps duplicates.
        if (line.key == "triggerall") {
            triggerAll_.push_back(Expr::parse(line.value));
            continue;
        }
        if (line.key.compare(0, 7, "trigger") == 0) {
            // `trigger1` .. `triggerN`. The number is the grouping, not a label:
            // triggers sharing one are ANDed and the groups are ORed. A suffix
            // that is not a number is treated as group 1, which is where a
            // bare `trigger` belongs.
            int group = 0;
            if (!tryParseInt(trim(line.key.substr(7)), group) || group < 1) group = 1;
            triggerGroups_[group].push_back(Expr::parse(line.value));
            continue;
        }
        if (line.key == "type") {
            type_ = lower(line.value);
            continue;
        }

        // Everything else is a parameter. `ignorehitpause` is kept like any
        // other so a controller can read it back.
        params_.push_back(Param{line.key, line.value, Expr{}, false});
    }
}

const Controller::Param* Controller::find(std::string_view key) const {
    for (const Param& param : params_) {
        if (param.key == key) return &param;
    }
    return nullptr;
}

std::string_view Controller::raw(std::string_view key) const {
    const Param* param = find(key);
    return param == nullptr ? std::string_view{} : std::string_view(param->raw);
}

int Controller::intParam(std::string_view key, int fallback) const {
    int value = 0;
    const std::string_view text = raw(key);
    if (text.empty() || !tryParseInt(text, value)) return fallback;
    return value;
}

float Controller::floatParam(std::string_view key, float fallback) const {
    float value = 0.0f;
    const std::string_view text = raw(key);
    if (text.empty() || !tryParseFloat(text, value)) return fallback;
    return value;
}

bool Controller::boolParam(std::string_view key, bool fallback) const {
    int value = 0;
    const std::string_view text = raw(key);
    if (text.empty() || !tryParseInt(text, value)) return fallback;
    return value != 0;
}

const Expr* Controller::exprFor(const Param& param) const {
    if (!param.parsed) {
        param.expr = Expr::parse(param.raw);
        param.parsed = true;
    }
    return param.expr.valid() ? &param.expr : nullptr;
}

float Controller::evalNumber(std::string_view key, const TriggerContext& context,
                             float fallback) const {
    const Param* param = find(key);
    if (param == nullptr) return fallback;
    const Expr* expr = exprFor(*param);
    if (expr == nullptr) return fallback;
    return expr->eval(context).toFloat();
}

Value Controller::evalValue(std::string_view key, const TriggerContext& context,
                            Value fallback) const {
    const Param* param = find(key);
    if (param == nullptr) return fallback;
    const Expr* expr = exprFor(*param);
    if (expr == nullptr) return fallback;
    return expr->eval(context);
}

bool Controller::triggered(const TriggerContext& context) const {
    if (triggerGroups_.empty()) return false;
    for (const Expr& expr : triggerAll_) {
        if (!expr.evalBool(context)) return false;
    }
    for (const auto& group : triggerGroups_) {
        bool all = true;
        for (const Expr& expr : group.second) {
            if (!expr.evalBool(context)) {
                all = false;
                break;
            }
        }
        if (all) return true;
    }
    return false;
}

// --------------------------------------------------------------------- states

void StateLibrary::parse(std::string_view text) {
    const IniFile document = IniFile::parse(text);

    // The state the most recent `[Statedef n]` opened, or kNoState before the
    // document has opened one.
    constexpr int kNoState = std::numeric_limits<int>::min();
    int current = kNoState;

    for (const IniSection& section : document.sections()) {
        if (section.name.compare(0, 8, "statedef") == 0) {
            int number = 0;
            if (!parseIntPrefix(section.name.substr(8), number)) continue;
            current = number;

            StateDef& state = states_[number];
            state.number = number;
            state.defined = true;

            for (const IniLine& line : section.lines) {
                if (line.key.empty()) continue;
                const std::string& key = line.key;

                if (key == "type") {
                    state.typeUnchanged = isUnchanged(line.value);
                    state.type = parseStateType(line.value, StateType::Stand);
                } else if (key == "movetype") {
                    state.moveTypeUnchanged = isUnchanged(line.value);
                    state.moveType = parseMoveType(line.value, MoveType::Idle);
                } else if (key == "physics") {
                    state.physicsUnchanged = isUnchanged(line.value);
                    state.physics = parsePhysics(line.value, Physics::Stand);
                } else if (key == "ctrl") {
                    state.ctrl = IniFile::getBool(section, "ctrl", false);
                    state.hasCtrl = true;
                } else if (key == "anim") {
                    // A plain number is kept as one; anything else (and there
                    // is real work out there writing `anim = 40+var(49)`) keeps
                    // its text for the runtime to evaluate.
                    state.animRaw = line.value;
                    state.hasAnim = true;
                    if (int value = 0; tryParseInt(line.value, value)) state.anim = value;
                } else if (key == "velset") {
                    const std::vector<std::string> parts = splitCommas(line.value);
                    if (parts.size() == 2) {
                        tryParseFloat(parts[0], state.velX);
                        tryParseFloat(parts[1], state.velY);
                        state.hasVelSet = true;
                    }
                } else if (key == "poweradd") {
                    if (int value = 0; tryParseInt(line.value, value)) {
                        state.powerAdd = value;
                        state.hasPowerAdd = true;
                    }
                } else if (key == "juggle") {
                    if (int value = 0; tryParseInt(line.value, value)) {
                        state.juggle = value;
                        state.hasJuggle = true;
                    }
                } else if (key == "sprpriority") {
                    if (int value = 0; tryParseInt(line.value, value)) {
                        state.sprPriority = value;
                    }
                } else if (key == "facep2") {
                    if (int value = 0; tryParseInt(line.value, value)) {
                        state.faceP2 = value != 0;
                        state.hasFaceP2 = true;
                    }
                } else if (key == "movehitpersist") {
                    state.moveHitPersist = IniFile::getBool(section, "movehitpersist", false);
                } else if (key == "hitcountpersist") {
                    state.hitCountPersist = IniFile::getBool(section, "hitcountpersist", false);
                } else if (key == "hitdefpersist") {
                    state.hitDefPersist = IniFile::getBool(section, "hitdefpersist", false);
                }
            }
            continue;
        }

        // `[State n, id]`, `[State n]`, and MUGEN's own bare `[State]` with the
        // number left off altogether. The number is optional and the section is a
        // controller either way: dropping the section instead of the missing label
        // loses every controller headed that way, and a file that writes all of
        // them that way loses all of them at once. (`[Statedef ...]` never reaches
        // here -- it is handled and `continue`d above.)
        std::string_view rest;
        if (section.name == "state") {
            rest = {};
        } else if (section.name.compare(0, 6, "state ") == 0 ||
                   section.name.compare(0, 6, "state,") == 0) {
            rest = std::string_view(section.name).substr(6);
        } else {
            continue;
        }
        int labelled = 0;
        const bool hasLabel = parseIntPrefix(rest, labelled);

        // Which state this controller belongs to.
        //
        // The number in `[State n, id]` is *not* the answer, and this is the one
        // place where reading the format naively gets a whole roster wrong.
        // Authors label these sections with the number of whatever state they
        // copied them from: a file will write `[State 0, AngleDraw]`,
        // `[State 200, 1]`, `[State 20, 3]` inside a `[Statedef 1300]`, and mean
        // state 1300 every time. Counting the labels in the characters on disk,
        // most files disagree with their own enclosing statedef for the majority
        // of their controllers -- one of them disagrees 2525 times out of 2599.
        // Taken literally, `[State 0, ...]` in every attack state of the file
        // would pour hundreds of stray controllers into state 0 -- a self
        // destruct, a sound, a velocity set -- and the fighter would fall over
        // the moment the match began. MUGEN attaches to the state being defined,
        // and so must this.
        //
        // The three states the engine runs by itself are the exception, because
        // those labels are always deliberate: a file writes `[State -3, AI]`
        // inside its `[Statedef -1]` and means the AI state, not state -1.
        int owner = current;
        if (hasLabel && (labelled < 0 || owner == kNoState)) owner = labelled;

        Controller controller;
        // The id is everything after the first comma in the section name; a
        // section with no comma has none.
        const size_t comma = section.name.find(',');
        if (comma != std::string::npos) controller.setId(section.name.substr(comma + 1));
        controller.parse(section);
        if (controller.empty()) continue;

        StateDef& state = states_[owner];
        state.number = owner;
        state.controllers.push_back(std::move(controller));
    }
}

const StateDef* StateLibrary::find(int number) const {
    const auto it = states_.find(number);
    return it == states_.end() ? nullptr : &it->second;
}

size_t StateLibrary::controllerCount() const {
    size_t total = 0;
    for (const auto& entry : states_) total += entry.second.controllers.size();
    return total;
}

bool StateLibrary::usesController(std::string_view type) const {
    for (const auto& entry : states_) {
        for (const Controller& controller : entry.second.controllers) {
            if (lower(controller.type()) == type) return true;
        }
    }
    return false;
}

}  // namespace af
