// A fighter in a match: one character, and the state machine running it.
//
// This is a thin layer now. It holds the things the fight loop cares about --
// where the fighter stands, how much life it has, what it is doing -- and it
// draws it, but everything a character *does* comes from its own .cns/.cmd by
// way of engine/statemachine. A character's punch is its own punch, its
// fireball is its own fireball, and its animation numbers are the ones its
// author wrote.
//
// A fighter built without a character is the engine test's placeholder. It has
// no files and so no behaviour, and it falls back to a small hand-written set
// of moves -- stand, walk, jump, punch, kick -- purely so the debug arena has
// something to hit. A real match never reaches that path.
#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "engine/character.h"
#include "engine/entities.h"
#include "engine/input.h"
#include "engine/statemachine.h"
#include "gfx/renderer.h"

namespace af {

class Fighter {
public:
    // MUGEN's move numbering, kept for the placeholder and for the fallback
    // animations a missing state lands on.
    static constexpr int kStandAnim = 0;
    static constexpr int kWalkAnim = 20;
    static constexpr int kJumpAnim = 40;
    static constexpr int kPunchAnim = 200;
    static constexpr int kKickAnim = 210;
    static constexpr int kHurtAnim = 5000;
    static constexpr int kKnockdownAnim = 5150;

    // What the fighter is doing, read off the state machine for the HUD and for
    // the fight screen's knock-out check.
    enum class Phase { Stand, Walk, Air, Attack, Hurt, KO };

    // Placeholder cosmetics, honoured only when there is no character to draw.
    void setFallbackLook(const std::string& name, Color body, Color trim);

    void setup(const Character* character, float startX, int facing);

    // Replaces the walk limits setup() installed. Stage coordinates only make
    // sense after translation into the fight's units, and the stage also decides
    // how close to the screen's edge a fighter may stand (`[Bound]`), so the
    // fight screen resolves the pair once per match and hands it down here.
    void setStageBounds(float min, float max) { machine_.setStageBounds(min, max); }

    // How hard the CPU plays, 0..9, matching the tiers select.def's
    // `arcade.maxmatches` is written in. Only the CPU is affected; the player's
    // fighter ignores it.
    //
    // MUGEN's difficulty is a property of the *match* rather than of the
    // fighter -- the same character is a pushover in fight one and a problem in
    // fight eight -- which is why this is set per match rather than read from
    // the character file.
    void setAiDifficulty(int level);

    // Where this fighter's sounds go. Handed to the state machine, so it is what
    // makes `type = PlaySnd` audible; a null sink leaves the fight silent, which
    // is what the engine test does.
    void setSoundSink(SoundSink* sink) { machine_.setSoundSink(sink); }
    // Where this fighter's explods and projectiles go. Handed to the state machine, so it
    // is what makes `type = Explod` visible and `type = Projectile` a beam rather than a
    // gap in the log.
    void setEntitySink(EntitySink* sink) { machine_.setEntitySink(sink); }
    // Which side of the fight this fighter is on, so an explod bound to it follows the
    // right one.
    void setSide(int side) { machine_.setSide(side); }

    // Whether a projectile's collision boxes -- the boxes of an animation, drawn from the
    // projectile's own position rather than from a fighter's -- would touch this fighter's
    // body. It is the one piece of hit geometry a projectile needs, and the reason the
    // fight loop can resolve a beam without a Fighter on the other end of it. There is no
    // scale parameter: `projscale` is draw-only (§7.2), so collision always uses the raw
    // position and the unscaled Clsn record.
    [[nodiscard]] bool overlapsEntityBox(const CollisionBox& box, int facing, float originX,
                                        float originY, float floorY) const;

    // The mirror image, for the player-attack-vs-projectile pass (§4): whether
    // one of this fighter's active Clsn1 boxes -- the armed HitDef's current
    // animation frame -- touches an entity's hurt box (a projectile's Clsn2,
    // converted from the projectile's own position and facing). Requires an
    // armed HitDef as well as a boxed frame, because a move that has already
    // spent its `hitcount` has no attack left to shoot a beam down with.
    [[nodiscard]] bool attackOverlapsEntityBox(const CollisionBox& box, int facing,
                                               float originX, float originY,
                                               float floorY) const;

    // One tick of steering, then one tick of physics. The opponent is needed
    // for auto-facing, for every `p2` trigger, and for the AI.
    void control(const InputState& input);
    void controlAI(Fighter& opponent);
    void step(Fighter& opponent);

    // Active attack frame meeting the opponent's hurt box. Needs the floor's
    // stage position, because collision space is screen-aligned.
    // `landed` reports whether the hit connected at all: an attack can be refused
    // outright, by the victim's air-juggle allowance running out or by the victim
    // already being dead, and the caller must not count that as a hit.
    bool tryHit(Fighter& target, float floorY, bool* landed = nullptr);
    // Whether this fighter's armed attack would connect right now, without
    // resolving it. The fight loop asks both fighters this before it resolves
    // either, because which of two simultaneous attacks wins depends on their
    // declared `priority` and not on which was checked first.
    [[nodiscard]] bool wouldHit(const Fighter& target, float floorY) const;
    // The `priority` of the armed HitDef. Higher wins; equal is a trade.
    [[nodiscard]] int attackPriority() const;
    // The *class* beside that number, which decides a tie between equal ones --
    // `priority = hit_prior, hit_type`. Only meaningful when `attackPriority()`
    // answered for an armed HitDef; with nothing armed the machine's own default
    // comes back, which is the `Hit` a clash with no attack at all cannot use.
    [[nodiscard]] StateMachine::PriorityClass attackPriorityClass() const {
        return machine_.attackPriorityClass();
    }
    // Tells both fighters whether the other is swinging -- or has something
    // swinging -- from within blocking range, which is MUGEN's `inguarddist`.
    // Called once a tick by the fight loop, which is the only place with both
    // fighters, the beams in flight and the floor in one space. `a` is side 0 and
    // `b` side 1, which is the fight loop's own convention and the one the
    // projectile list is indexed by.
    static void updateGuardDistances(Fighter& a, Fighter& b,
                                     const std::vector<Projectile>& projectiles, float floorY);
    // A hit applied from outside the fight loop, which is what the engine
    // test's debug keys do.
    void takeHit(int damage, int fromFacing);
    void gainPower(int amount);
    // Hands life back, for the part of a blocked hit that a fighter recovers by not
    // being hit again.
    void heal(int amount) { machine_.gainLife(amount); }
    // Pushes both grounded fighters apart when they overlap. Called after
    // both fighters stepped.
    static void separate(Fighter& a, Fighter& b);

    // `drawShadows` is the two global gates already resolved by the fight
    // screen (`[Config] drawshadows` and `globalnoshadow`). A fighter's own
    // `noshadow` and invisibility are read here from its machine.
    void draw(Renderer& renderer, float originX, float floorY, const ShadowStyle& shadow,
              bool drawShadows = true) const;

    [[nodiscard]] float x() const { return machine_.x(); }
    [[nodiscard]] float y() const { return machine_.y(); }
    [[nodiscard]] int facing() const { return machine_.facing(); }
    [[nodiscard]] Phase phase() const;
    [[nodiscard]] int life() const { return machine_.life(); }
    [[nodiscard]] int maxLife() const { return machine_.maxLife(); }
    [[nodiscard]] int power() const { return machine_.power(); }
    // Whether this fighter has been hit at all this round, which is what the
    // "perfect" win method is decided on. It is sticky rather than momentary:
    // the win is read at the end of a round, and a hit earlier in the round
    // still counts.
    [[nodiscard]] bool tookDamage() const { return everTookDamage_; }
    // Non-const: drawing from the character's sprite bank uploads its textures.
    [[nodiscard]] Character* character() const { return character_; }
    [[nodiscard]] const std::string& name() const;
    // The scale this fighter is drawn and collided at.
    //
    // `baseScale` is the character's localcoord ratio to the screenpack's --
    // 1.0 for this whole roster, because every character declares 320x240. The
    // fighter is then scaled again so that its standing art draws at
    // kReferenceStandingHeight, which is the one thing that makes the roster
    // playable as a roster: see the definition for why.
    void setScale(float baseScale);
    [[nodiscard]] float scale() const { return scale_; }
    [[nodiscard]] float baseScale() const { return baseScale_; }
    // Placeholder cosmetics, for the screen drawn when there is no character.
    [[nodiscard]] Color bodyColor() const { return bodyColor_; }
    [[nodiscard]] Color trimColor() const { return trimColor_; }

    // One ghost of the AfterImage trail: the sprite the fighter was showing at
    // some recent tick, at the place it stood. The fighter keeps a short ring of
    // them while the machine's trail clock runs and draws them fading out behind
    // the live sprite.
    struct Ghost {
        int group = -1;
        int image = -1;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float x = 0.0f;
        float y = 0.0f;
        int facing = 1;
    };
    [[nodiscard]] const std::deque<Ghost>& ghosts() const { return ghosts_; }

    // The running state machine, for anything that wants to ask a character a
    // question the fight loop does not.
    [[nodiscard]] StateMachine& machine() { return machine_; }
    [[nodiscard]] const StateMachine& machine() const { return machine_; }

private:
    // The placeholder's own moves, for the fighter that has no character.
    void controlPlaceholder(const InputState& input);
    void stepPlaceholder();
    // Records an AfterImage ghost when the trail clock is running. Called at the
    // end of step(), on the frame the machine ended the tick showing.
    void recordGhost();
    // Drops the trail -- round starts and character changes must not leave last
    // round's ghosts standing in the arena.
    void clearGhosts() { ghosts_.clear(); }

    [[nodiscard]] int animNumber() const { return machine_.animNumber(); }
    // Converts one of this fighter's collision boxes to world space.
    [[nodiscard]] Rect4 worldBox(const CollisionBox& box, float floorY) const;
    // The fighter's current hurt box in world space, so a *target* can be hit.
    [[nodiscard]] Rect4 worldHurtBox(float floorY) const;

    Character* character_ = nullptr;  // null when this is a placeholder
    StateMachine machine_;

    std::string fallbackName_ = "FIGHTER";
    Color bodyColor_{0.8f, 0.4f, 0.35f, 1.0f};
    Color trimColor_{1.0f, 0.8f, 0.65f, 1.0f};
    float scale_ = 1.0f;
    float baseScale_ = 1.0f;
    bool everTookDamage_ = false;
    // Ticks since the round began, which is what `gametime` reports.
    long long matchTicks_ = 0;

    // The AfterImage ring: newest at the back, capped at the trail's length.
    std::deque<Ghost> ghosts_;

    // Placeholder-only state: a phase, its clock, and the last attack started.
    Phase placeholderPhase_ = Phase::Stand;
    int placeholderTicks_ = 0;
    int placeholderAttackTicks_ = 0;
    int placeholderAttackAnim_ = kPunchAnim;
    bool placeholderHasHit_ = false;
};

}  // namespace af
