// Things that belong to the fight rather than to a fighter: explods and projectiles.
//
// These are the two halves of what MUGEN calls a "non-player entity", and they are the
// difference between a roster whose moves work and one whose moves fire into nothing.
// Counted across the nineteen characters installed here, the `Explod` controller is used
// 1714 times and `Projectile` 104 -- sparks, dust, auras, charge glows and every beam a
// Dragon Ball character throws -- and neither had anywhere to go before this existed.
//
// An **explod** is an animation with a position: no collision, no state machine, just
// art that appears, drifts, animates and goes. A **projectile** is the same with
// velocity and hitboxes, which is a fireball. The two share almost all of their
// bookkeeping, which is why they are in one file.
//
// What they are *not* is a `Helper`, MUGEN's third kind of entity -- a second fighter
// with its own state machine. Helpers are fighters rather than entries in this layer,
// so they arrive by a different path: the HelperSpec/HelperHost pair below, which the
// fight screen answers.
#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "engine/projectile_rules.h"
#include "engine/statemachine.h"
#include "gfx/renderer.h"

namespace af {

class Character;
class SpriteBank;

// Where a position is measured from. MUGEN's `postype`, with its names.
enum class PostType {
    P1,     // relative to the owner, +x toward its front
    P2,     // relative to the opponent, the same way
    Front,  // from the edge the owner faces, +x to the right of the screen
    Back,   // from the edge the owner faces away from
    Left,   // from the screen's top-left corner
    Right,  // from the screen's top-right corner
};

[[nodiscard]] PostType parsePostType(std::string_view text, PostType fallback);

// Which palette an entity's indexed sprite draws with, as a table index plus
// the bank that index belongs to.
//
// An entity's art and its palette can come from two different sprite files: a
// `fightfx` spark is decoded against the screenpack's palette table, while an
// `ownpal = 0` explod has to draw in the *owner fighter's* current palette. The
// two tables are separate, so a bare index is not enough to name a LUT.
struct EntityPaletteChoice {
    // True for an index into the sprite's own bank, false for one into the
    // owner character's bank.
    bool fromSpriteBank = true;
    // The palette table entry, or -1 when neither side has one. A -1 leaves the
    // draw with no palette opinion, which is the renderer's default-LUT case.
    int index = -1;
};

// The palette-selection decision for one entity draw.
//
// `ownpal` is the controller's flag. `spritePalette` is
// `SpriteBank::spritePalette(group, image)` for the sprite about to be drawn --
// the table entry it decoded against, or -1 when the bank has none -- and
// `ownerPalette` is the owning fighter's current palette after RemapPal,
// `Character::paletteIndexFor(StateMachine::effectivePalette())`.
//
// The reference's `Explod` documentation defines the flag as "give the explod
// its own copy of its palette ... to keep temporary changes to the player's
// palette (e.g. from using the PalFX controller) from affecting the color of
// the explod", and gives it the default 0. So `ownpal = 0` inherits the owner's
// palette -- palno and RemapPal reach it -- and `ownpal = 1` uses the palette
// the sprite itself carries, which the owner's selection does not replace.
//
// One rule rides in front of both: a sprite whose own palette slot is non-zero
// (a PNG8's PLTE, a v2 palette node) keeps that palette either way. That is the
// same precedence `Fighter::draw` applies, and the reason is the same -- such a
// sprite is already outside palno's reach by the SFF's own convention.
[[nodiscard]] EntityPaletteChoice chooseEntityPalette(bool ownpal, int spritePalette,
                                                     int ownerPalette);

// One explod: an animation, a place, and how long it lasts.
struct Explod {
    // Whose sprite file and palette it draws with. Const because a state machine holds a
    // const character and an entity must not be a back door to changing one; the draw
    // site casts it back, since decoding a sprite needs a mutable bank.
    const Character* owner = nullptr;
    // The machine that spawned it, for the one thing an entity needs *live* from
    // its owner: the palette selection an `ownpal = 0` draw inherits. `palno` and
    // RemapPal can change while the explod is still on screen, so the draw reads
    // the machine rather than a palette index copied at spawn. Null when an explod
    // is built outside a state machine (a test fixture), in which case the owner's
    // default palette is the fallback.
    const StateMachine* ownerMachine = nullptr;
    // Whether the animation comes from the screenpack's `fightfx.air` rather than from the
    // owner's own .air. That is the difference a HitDef's plain `sparkno = 2` and its
    // `S`-prefixed `sparkno = S2` make, and it is the only reason an explod needs to know
    // which file to look in.
    bool fromFightFx = false;
    // Which side made it. An index rather than the pointer above, because a mirror match
    // has two fighters on the same character and a bound explod has to follow the right
    // one of them.
    int ownerSide = 0;
    int anim = -1;
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;               // height above the floor, in the engine's own frame
    float velX = 0.0f;
    float velY = 0.0f;
    float accelX = 0.0f;
    float accelY = 0.0f;
    int facing = 1;
    bool flipV = false;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    int spritePriority = 0;
    // Ticks left of being carried by the owner. -1 means "for ever", which is how a
    // charge aura stays on the character while the state runs.
    int bindTime = 0;
    float bindOffsetX = 0.0f;
    float bindOffsetY = 0.0f;
    // When it goes, in MUGEN's own three-number code, which is not a range: a
    // positive value is a lifetime in ticks, `-1` is *displayed for ever*, and `-2`
    // -- the documented default -- is "removed when its animtime reaches 0": one
    // pass of the animation, whether or not the animation loops (see
    // `entityLoopTimeReached`). The two negatives are two different answers, and
    // reading the default as `-1` collapsed them, so an explod written
    // `removetime = -1` for an aura meant to outlive its animation died with it.
    int removeTime = -2;
    // The explod's `trans`, and the reason it is optional rather than defaulted:
    // the reference's word for an absent `trans` is "does nothing", and the
    // thing it does nothing to is the animation's own colour-add code. A default
    // of `Normal` here would instead *cancel* an `A1` frame, which is a
    // different picture. Set means "override the animation", unset means "the
    // animation speaks" -- see `drawExplod`.
    std::optional<TransParams> trans;
    // `ownpal`: whether the explod draws in a palette of its own rather than in
    // its owner's current one. sctrls.html defines it as a boolean, says that
    // setting it to 1 gives the explod "its own copy of its palette" so that
    // "temporary changes to the player's palette (e.g. from using the PalFX
    // controller)" do not reach it, and states the default in as many words:
    // "Defaults to 0 if omitted." Zero is therefore inheritance, not a
    // no-op -- see `chooseEntityPalette`.
    bool ownpal = false;
    // `shadow = r,g,b`, the explod's own ground shadow. `0,0,0` -- the default
    // -- means none; `r == -1` asks for the stage's `[Shadow] color`; any other
    // triple is an explicit colour. The asymmetry with a projectile's
    // `projshadow` (where *any* component of -1 selects the stage colour) is in
    // the reference and is preserved -- see `resolveObjectShadowColor`.
    ShadowRgb shadow;
    // The explod's two ways of refusing to be frozen. They answer two different
    // freezes and the reference documents them apart, against different parameters:
    //
    //   * a *hitpause* -- the freeze a landed hit hands both fighters -- is what
    //     `ignorehitpause` speaks to, and sctrls.html gives it the permissive
    //     default of 1: an explod animates through a hitpause unless its author
    //     said otherwise, which is why a hit spark plays while the fighters it
    //     marks are stopped;
    //   * a running Pause or SuperPause holds the explod unless it asked for the
    //     first few ticks of that pause by name, and zero -- the default -- asks
    //     for none.
    //
    // One flag stood for both of these, and the two rules are not the same rule:
    // it gave every explod the superpause behaviour and none of them the hitpause
    // one, so a super spark hung where the reference plays it and a hit spark
    // outlived the fight it belongs to.
    bool ignoreHitPause = true;
    int pauseMoveTime = 0;
    int superMoveTime = 0;
    // Ticks since it appeared, and the animation's own clock.
    long long clock = 0;
    bool gone = false;

    // `removeongethit`: the explod goes the tick its owner is hit, whatever its
    // own life had left. It is how a character takes an aura or a charge glow off
    // itself the moment the charge is interrupted, rather than leaving it to
    // play out around a fighter that is no longer doing the move.
    bool removeOnGetHit = false;
    // `ontop`: drawn over the fighters and over every other explod, whatever
    // `sprpriority` says -- the reference gives this "precedence over
    // sprpriority" in as many words. It is the one explod property that is a
    // *layer* rather than an ordering within a layer, so the layer has to exist
    // for it to mean anything: see `EntityLayer::drawTop`, and the note there
    // about how far up "over" reaches.
    bool onTop = false;

    // Aura-like explods are drawn behind their owner, sparks in front of everything.
    bool behindOwner = false;
};

// A `ModifyExplod`: which explods to reach, and what to change about them.
//
// The reference's whole description of this controller is that its "syntax is
// basically the same as Explod" -- and the operative difference is that it is not a
// creation. Every field in here is optional, an absent one leaves that property of
// the target exactly as it was, and the explod keeps its clock, its place in the
// animation and everything else nobody named. An explod cannot be *unmade* by this;
// that is `RemoveExplod`.
//
// It is worth having because of what it is for: a character that makes one long-lived
// effect and then re-dresses it as the move progresses -- a charge aura that changes
// colour, a beam core that grows, a portal whose animation steps through its own
// stages. The corpus writes it 149 times across 19 packs, and every one of those was
// a controller this engine did not know.
struct ExplodModify {
    // Whose explods are being reached. `id` is the only selector the reference's
    // documentation gives this controller, and an omitted `id` is the id 0 that an
    // explod written without one carries.
    //
    // A later reference added an `index` parameter for picking one out of several
    // explods sharing an id. It is deliberately not here, and the reason is worth
    // writing down rather than leaving as an absence: the documentation this engine
    // works from lists no such parameter, not one of the 46 packs of the corpus writes
    // it -- 238 `ModifyExplod` blocks between them -- and the numbering a reader would
    // have to invent (oldest first? and does a new explod displace one already at that
    // index?) is not recoverable from anything on hand. Reading it as "all explods with
    // this id" is the documented behaviour of the controller, so a file that wrote an
    // index would get *more* explods modified than it asked for rather than a
    // differently-chosen one.
    const Character* owner = nullptr;
    int id = 0;

    // The animation, in the same two-file convention Explod uses (`F` in front of the
    // number means fightfx).
    std::optional<int> anim;
    std::optional<bool> animFromFightFx;
    // `pos`, already resolved through `postype` into the engine's own world frame by
    // the state machine, which is the only place that knows where the opponent and the
    // screen edges are. A moved explod is re-bound about its new place: an explod that
    // is following its owner has to go on following it from where it was just put, or
    // the layer's next tick would drag it straight back.
    bool movesPosition = false;
    float x = 0.0f;
    float y = 0.0f;
    // Where the owner stands at the moment of the modification, so a moved explod can
    // be re-bound about its new place rather than about the one it was born at. The
    // layer cannot measure this itself: it holds entity copies and the fighters are
    // somebody else's.
    float ownerX = 0.0f;
    float ownerY = 0.0f;
    // `vel` and `accel`, in the engine's frame: the sign flip MUGEN's downward y needs
    // has already happened when these are set.
    std::optional<float> velX;
    std::optional<float> velY;
    std::optional<float> accelX;
    std::optional<float> accelY;
    std::optional<int> facing;
    std::optional<bool> flipV;
    std::optional<float> scaleX;
    std::optional<float> scaleY;
    std::optional<int> bindTime;
    // The three-number lifetime, whose two negatives are two different answers -- see
    // `Explod::removeTime`. A modification is not a restart: the clock is left where it
    // is, so a `removetime` written here counts from now and means what it means.
    std::optional<int> removeTime;
    // `trans`. Two levels of "nothing", which is why this is a flag beside the value
    // rather than an `optional<optional<...>>`: the outer question is whether the file
    // named the parameter at all, and the inner one is whether what it named was
    // `none` -- a real value, meaning "stop the animation's own colour-add", as opposed
    // to an absent parameter, meaning "leave whatever is there".
    bool setTrans = false;
    std::optional<TransParams> trans;
    // `ownpal`, in the same two-level shape: `ModifyExplod`'s syntax is "basically
    // the same as Explod", so naming it re-dresses an explod's palette policy, and
    // omitting it leaves the policy the explod was born with alone. See
    // `Explod::ownpal` for the parameter's meaning and documented default.
    std::optional<bool> ownpal;
    // `shadow`, in the same two-level shape as `ownpal`: naming it re-dresses
    // the explod, omitting it leaves the colour it was born with alone.
    std::optional<ShadowRgb> shadow;
    std::optional<bool> ignoreHitPause;
    std::optional<int> pauseMoveTime;
    std::optional<int> superMoveTime;
    // `sprpriority` and `ontop`, and the layer the pair implies. The state machine
    // answers `behindOwner` itself, off its own sprite priority, because the layer has
    // no fighter to compare against -- see the note on `ontop` in `doExplod` for why the
    // flag replaces the comparison rather than joining it.
    std::optional<int> spritePriority;
    std::optional<bool> onTop;
    std::optional<bool> behindOwner;
    std::optional<bool> removeOnGetHit;
};

// One projectile: an animation that travels and hits. Its hit properties are the
// HitDef the owner had armed when it was created, which is MUGEN's rule and the reason
// a character writes `type = Projectile` *inside* an attack state.
struct Projectile {
    const Character* owner = nullptr;
    // The spawning machine, for the live palette selection an `ownpal = 0`
    // projectile inherits -- the same reason `Explod` carries it.
    const StateMachine* ownerMachine = nullptr;
    int ownerSide = 0;
    int anim = -1;
    int hitAnim = -1;      // played where it connected
    int removeAnim = -1;   // played where it went, in place of `anim`
    int cancelAnim = -1;   // `projcancelanim`, for a projectile-vs-projectile loss
    // Which file each of those four is in. A projectile's animations are written the
    // same way an explod's is -- a bare number means the character's own .air and an `F`
    // in front of it means the screenpack's `fightfx.air` -- and they are resolved
    // separately because nothing stops a character from mixing them, which is exactly
    // what a screenpack's common beam-and-dust set is for. Without these, `projanim =
    // F60` reached the expression parser as an identifier, came back 0, and drew the
    // character's *stand* sprite as its fireball.
    bool animFromFightFx = false;
    bool hitAnimFromFightFx = false;
    bool removeAnimFromFightFx = false;
    bool cancelAnimFromFightFx = false;
    // The explicit state (§5.1). `Flying` is the only state that moves with flight
    // velocity, counts towards `numproj` and collides with another projectile; the
    // three removal states play one animation each and then free the slot. This
    // replaced a `removing` bool because the animation a projectile plays, and
    // whether it may still hit a player, differ per state.
    ProjectileState state = ProjectileState::Flying;
    [[nodiscard]] bool isFlying() const { return projectileIsFlying(state); }
    // True for states 2, 3 and 4 -- "this slot is on its way out", which is what
    // the clock, drawing and the end-of-animation test all care about.
    [[nodiscard]] bool isRemoving() const { return !isFlying(); }
    // `remvelocity`: the velocity a removal state uses after its first tick. Stored
    // world-scaled like `velX`/`velY`, so `remVelY` is already flipped for the
    // height-above-floor y axis.
    float remVelX = 0.0f;
    float remVelY = 0.0f;
    // `projpriority`, default 1. Raw and unclamped: negatives are meaningful and
    // are compared exactly as read (§2.4).
    int priority = 1;
    // `ownpal`: whether the projectile draws in a palette of its own instead of
    // its owner's current one. The reference's Projectile section does not list
    // the parameter, but the corpus's characters write it on projectiles exactly
    // as they do on explods, and 0 is the documented default everywhere the flag
    // *is* documented (Explod and Helper): inherited, not own. See
    // `chooseEntityPalette`.
    bool ownpal = false;
    // `projshadow = r,g,b`. The default `0,0,0` means no shadow; unlike
    // Explod's `shadow`, *any* component equal to -1 selects the stage colour
    // (SHADOW-RE §4.5). See `resolveObjectShadowColor`.
    ShadowRgb shadow;
    float x = 0.0f;
    float y = 0.0f;
    float velX = 0.0f;
    float velY = 0.0f;
    float accelX = 0.0f;
    float accelY = 0.0f;
    float velMulX = 1.0f;
    float velMulY = 1.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    int facing = 1;
    int spritePriority = 0;
    int id = 0;
    // How many times it may connect before going, and how long it waits between hits.
    int hitsLeft = 1;
    int missTime = 0;
    int missTicks = 0;
    int removeTime = -1;
    // `projremove`: whether spending its hits takes it out of the world. The
    // reference still decrements the hit counter when it is 0 (`decl
    // 0x1C(%eax)` at 0x46FDA9) and only skips the forced removal
    // (`cmpl $0, 0x38(%eax)` at 0x46FDC9), so a `projremove = 0` beam that has
    // hit everything it can keeps flying and drawing.
    bool removeOnHit = true;
    // Where it is destroyed: past the screen edge, past the stage's bounds, or outside
    // a vertical range. MUGEN's three bounds, in its own units.
    float edgeBound = 40.0f;
    float stageBound = 40.0f;
    float lowBound = -240.0f;
    float highBound = 100000.0f;
    // A player contact is recorded during the fight screen's hit pass and applied at
    // the start of the next entity update (§3.2). `pendingCooldown` is copied into
    // `missTicks`, and `pendingPause` into `pause`, when it is applied.
    bool pendingHit = false;
    int pendingPause = 0;
    int pendingCooldown = 0;
    // Its own freeze, from the HitDef's attacker `pausetime` (§2.5/§3.2). A frozen
    // projectile neither moves nor ages, but its pending hit still settles.
    int pause = 0;
    // Ticks spent in the current removal state. The first one uses the flight
    // velocity, every later one `remvelocity` (§5.3).
    int stateAge = 0;
    // The reaction it gives, copied from the owner's HitDef at spawn time.
    StateMachine::HitDef hit;
    long long clock = 0;
    bool gone = false;
};

// What a `Helper` controller asks for, resolved against the spawner before it
// leaves the state machine: position is already world-space (MUGEN's postype
// applied), facing already folded in. Everything else is the file's own words.
//
// A helper is MUGEN's third kind of entity and the one the other two are
// *not*: a second state machine running the same character's states. Summoned
// partners, super flash doubles, beam cores, thrown pets -- all helpers.
struct HelperSpec {
    const Character* character = nullptr;  // the spawner's own character
    StateMachine* spawner = nullptr;       // the machine that asked: becomes its parent
    int side = 0;
    int helperId = 0;      // MUGEN's `id`: what `helper(id)` and `numhelper(id)` match
    std::string name;      // MUGEN's `name`: for logs and nothing else
    float x = 0.0f;        // world x; y is height above the floor as everywhere here
    float y = 0.0f;
    int facing = 1;
    int stateNo = 0;       // MUGEN's `stateno`: the state the helper starts in
    bool keyCtrl = false;  // reads the pad its side's player holds
    // How the helper behaves while its owner is frozen by a Pause/SuperPause:
    // the ticks it keeps moving past the start of one. Zeroes mean "frozen with
    // the rest of the fight", which is the common case.
    int pauseMoveTime = 0;
    int superMoveTime = 0;
};

// Whoever can turn a HelperSpec into a running helper. The fight screen does,
// because it owns the fighters a helper is one of; host tests do not, and the
// controller then quietly drops the spawn exactly as a null sound sink drops a
// PlaySnd.
class HelperHost {
public:
    virtual ~HelperHost() = default;
    // Returns the new helper's state machine, or null when nobody is listening.
    virtual StateMachine* spawnHelper(const HelperSpec& spec) = 0;
    // Match-wide id lookup, for the `playerid(n)` redirect and `playeridexist`.
    // Mutable because TargetAdd both finds and *acts on* the player it names.
    virtual StateMachine* playerById(int id) = 0;
};

// The state of a match-wide freeze, owned by the entity layer so every state machine
// can read it through its sink. MUGEN has two: `Pause` (any move may take one) and
// `SuperPause` (the super-flash freeze, with its screen dim and power theatre).
// Semantics here are MUGEN's: everyone frozen except the actor for its `movetime`
// allowance, and a helper of the actor's family moves while the pause is younger
// than its own pausemovetime/supermovetime budget.
struct PauseInfo {
    int ticks = 0;                     // remaining; zero means no pause
    int total = 0;                     // the pause's full length
    const StateMachine* actor = nullptr;
    int actorMove = 0;                 // ticks the actor itself may still move
    bool isSuper = false;
    bool darken = false;               // superpause dimmed backdrop
    float p2DefMul = 1.0f;             // victim defence scaling while it runs
    bool pauseBg = true;               // whether stage/explod time also holds
};

// Somewhere for a state machine to send the entities it creates.
//
// An interface rather than a direct pointer, for the same reason `SoundSink` is: the
// state machine is a host-testable unit that has no business knowing what a fight screen
// is, and a test can then assert that a controller *asked* for an explod without any of
// the fight being present.
class EntitySink {
public:
    virtual ~EntitySink() = default;
    virtual void spawnExplod(const Explod& explod) = 0;
    virtual void spawnProjectile(const Projectile& projectile) = 0;
    // The shake an `EnvShake` controller asked for: amplitude in virtual units, a
    // frequency in cycles a second, and how long it lasts.
    virtual void shake(int time, float frequency, float amplitude) = 0;
    // A pause a controller asked for. Pause and SuperPause share the clock: a second
    // one replaces the first, which is MUGEN's rule -- two supers cannot freeze the
    // same frame twice.
    virtual void beginPause(int time, const StateMachine* actor, int moveTime) {
        (void)time;
        (void)actor;
        (void)moveTime;
    }
    virtual void beginSuperPause(int time, const StateMachine* actor, int moveTime,
                                 bool darken, float p2DefMul) {
        (void)time;
        (void)actor;
        (void)moveTime;
        (void)darken;
        (void)p2DefMul;
    }
    // Defaults to "no pause", which is what a sink with no sense of time answers.
    [[nodiscard]] virtual PauseInfo pauseInfo() const { return PauseInfo{}; }
    // How many explods and projectiles with this id the owner has out, for the
    // `numexplod` and `numproj` triggers that gate so much of a character's logic.
    // `ownerMachine` is the state owner asking, and it is what the reference's
    // lists are keyed by: every player struct -- a fighter and each of its
    // helpers -- owns its own sparse slot array (`PROJECTILE-PARAMS-RE.md` §1).
    // A null machine keeps the old character-wide reading for callers that have
    // no state owner to name.
    [[nodiscard]] virtual int countExplods(const Character* owner,
                                           const StateMachine* ownerMachine, int id) const = 0;
    [[nodiscard]] virtual int countProjectiles(const Character* owner,
                                              const StateMachine* ownerMachine, int id) const = 0;
    // The owning side's last projectile contact, for the `ProjHit` /
    // `ProjGuarded` / `ProjContact` triggers. The reference keeps this on the
    // player rather than on the projectile, so it outlives the projectile slot
    // (`PROJECTILE-COLLISION-RE.md` §1). A sink with no projectile machinery
    // answers "no contact ever", which is what every trigger treats as false.
    [[nodiscard]] virtual ProjectileContactRecord projectileContact(int ownerSide) const {
        (void)ownerSide;
        return ProjectileContactRecord{};
    }
    // `ModifyExplod`: re-dress explods that are already out. The default does nothing,
    // which is what a sink with no entities answers.
    virtual void modifyExplod(const ExplodModify& modify) { (void)modify; }
    // `RemoveExplod`: take the owner's explods carrying this id off the screen. `all`
    // is the file having named no id at all, which the reference reads as "every
    // explod this player has" -- a distinction without which the id-0 explods of a
    // character that meant to clear everything would outlive a round's end. Not
    // implemented, this controller was 921 uses across 48 of the corpus' packs that
    // silently did nothing: every charge aura and every super flash that its author
    // took down by name stayed up until its own `removetime` ran out.
    virtual void removeExplod(const Character* owner, int id, bool all) {
        (void)owner;
        (void)id;
        (void)all;
    }
    // `EnvColor`: a fullscreen colour wash for `time` ticks. `under` draws it
    // behind the fighters instead of over them.
    virtual void envColor(float r, float g, float b, int time, bool under) {
        (void)r;
        (void)g;
        (void)b;
        (void)time;
        (void)under;
    }
    // `BGPalFX` and `AllPalFX`: the two global palette grades. They are PalFX's
    // arithmetic written to a slot on the fight rather than to one fighter -- the
    // background layer, and the whole screen -- and they are distinct slots, not
    // aliases of each other or of the fighter's PalFX. Defaults drop them, which is
    // what a sink with no picture does.
    virtual void setBgPalFx(const StateMachine::PalFxState& state) { (void)state; }
    virtual void setAllPalFx(const StateMachine::PalFxState& state) { (void)state; }
    // A spawned helper, or null when the listener keeps no fighters. Not pure:
    // a sink that only draws sparks must not have to know what a helper is.
    virtual StateMachine* spawnHelper(const HelperSpec& spec) {
        (void)spec;
        return nullptr;
    }
    // Match-wide id lookup. Default: unknown.
    [[nodiscard]] virtual StateMachine* playerById(int id) {
        (void)id;
        return nullptr;
    }
    // Where the camera's middle sits in stage space and where the floor line is,
    // for the `screenpos` triggers, which read screen-space. Defaults are the
    // canonical 320x240 values so tests that never drive a fight still answer.
    // The fight screen writes them every tick through its EntityLayer.
    [[nodiscard]] virtual float cameraMidX() const { return 160.0f; }
    [[nodiscard]] virtual float floorLineY() const { return 210.0f; }
};

// The entities of one fight.
class EntityLayer : public EntitySink {
public:
    // The stage view of the fight, so `screenpos` reads against the real camera.
    void setStageView(float cameraMidX, float floorY) {
        cameraMidX_ = cameraMidX;
        floorY_ = floorY;
    }
    [[nodiscard]] float cameraMidX() const override { return cameraMidX_; }
    [[nodiscard]] float floorLineY() const override { return floorY_; }

    // The stage's world x range, so `projstagebound` can be measured from its
    // edges (§6.1). The layer cannot read a stage itself, so the fight screen
    // supplies the pair once at match setup; the defaults are effectively
    // unbounded, which keeps host tests and the training screen working.
    void setProjectileStageBounds(float left, float right) {
        projectileStageLeft_ = left;
        projectileStageRight_ = right;
    }

    // Enters a removal state from outside the layer. The projectile-vs-
    // projectile pass lives in the fight screen because it needs both fighters'
    // boxes, so it asks for the cancel here rather than reaching into the state
    // logic. A projectile already in a removal state is left alone.
    void cancelProjectile(Projectile& projectile);

    // Enters the state-2 removal path from outside the layer. This is the state
    // a player's attack gives a projectile (§4, `0x46F5B2`): `projremanim`,
    // deliberately distinct from the state-3 cancel a projectile-vs-projectile
    // loss uses. The player-hit pending machinery is *not* used -- an attack
    // spends the hit and sets the state here, in the same tick.
    void removeProjectileByAttack(Projectile& projectile);

    // Records one projectile contact against the *owning side*, as the
    // reference writes `+0x25C`/`+0x260` on the owner. A later contact
    // replaces an earlier one. The fight screen's three collection passes call
    // this; the state machine reads it through `projectileContact`.
    void noteProjectileContact(int ownerSide, int projectileId, int code);
    [[nodiscard]] ProjectileContactRecord projectileContact(int ownerSide) const override;

    // The screenpack's spark bank, so that an explod whose animation came from fightfx can
    // be drawn from fightfx's sprites rather than from a character's. Null is fine: those
    // explods are then skipped, which is what happens in a host test.
    void setFightFx(SpriteBank* sprites, const AnimationSet* animations) {
        fxSprites_ = sprites;
        fxAnimations_ = animations;
    }

    // The stage-owned shadow appearance, and the two global gates: `[Config]
    // drawshadows` and the match-wide `globalnoshadow`. Object shadows are not
    // gated on any player's `noshadow` (the reference's per-player object pass
    // does not test it), so this is all the layer needs to know.
    void setShadowStyle(const ShadowStyle& style, bool drawShadows) {
        shadow_ = style;
        drawShadows_ = drawShadows;
    }

    void spawnExplod(const Explod& explod) override;
    void spawnProjectile(const Projectile& projectile) override;
    void shake(int time, float frequency, float amplitude) override;
    [[nodiscard]] int countExplods(const Character* owner, const StateMachine* ownerMachine,
                                   int id) const override;
    [[nodiscard]] int countProjectiles(const Character* owner, const StateMachine* ownerMachine,
                                       int id) const override;
    void modifyExplod(const ExplodModify& modify) override;
    void removeExplod(const Character* owner, int id, bool all) override;

    // Who turns a Helper request into a fighter: the fight screen, which owns the
    // roster of running fighters. Unset in tests, where spawnHelper quietly fails.
    void setHelperHost(HelperHost* host) { helperHost_ = host; }
    StateMachine* spawnHelper(const HelperSpec& spec) override;
    [[nodiscard]] StateMachine* playerById(int id) override;

    // One tick. `boundTo` is the fighter each side belongs to, so that a bound explod
    // follows them.
    void update(const StateMachine* p1, const StateMachine* p2);

    // Clears everything. Called at the start of a round: a spark from the last round
    // must not be drawn over the next one.
    void clear();

    // One explod, from whichever bank its animation came from.
    void drawExplod(Renderer& renderer, const Explod& explod, float scrollX, float floorY) const;

    // Explods drawn behind the fighters, in sprite-priority order.
    void drawBehind(Renderer& renderer, float scrollX, float floorY) const;
    // Explods and projectiles drawn in front.
    void drawFront(Renderer& renderer, float scrollX, float floorY) const;
    // The `ontop` explods: drawn after everything else in the world, over the
    // fighters and over every explod and projectile that is not itself `ontop`.
    // They are a layer of their own rather than a high sprite priority because
    // the reference separates the two in those terms -- `ontop` "has precedence
    // over sprpriority" -- and a priority could only ever be above or below the
    // number a fighter happens to carry this tick.
    void drawTop(Renderer& renderer, float scrollX, float floorY) const;

    // The match-wide freeze. The countdown happens in update(), which the fight
    // loop calls exactly once a tick, and state machines read it through
    // pauseInfo() at the top of their step.
    void beginPause(int time, const StateMachine* actor, int moveTime) override;
    void beginSuperPause(int time, const StateMachine* actor, int moveTime, bool darken,
                         float p2DefMul) override;
    [[nodiscard]] PauseInfo pauseInfo() const override { return pause_; }

    // The running colour wash, for the fight to draw.
    struct EnvColorState {
        float r = 1.0f, g = 1.0f, b = 1.0f;
        int ticks = 0;
        bool under = false;
    };
    void envColor(float r, float g, float b, int time, bool under) override {
        envColor_ = EnvColorState{r / 255.0f, g / 255.0f, b / 255.0f, std::max(0, time), under};
    }
    [[nodiscard]] EnvColorState envColorState() const { return envColor_; }

    // The two global palette grades, drawn by the fight. BGPalFX covers the stage
    // layer; AllPalFX covers everything drawn after it is set. They age with the
    // fight's clock in update(), like the colour wash above.
    void setBgPalFx(const StateMachine::PalFxState& state) override { bgPalFx_ = state; }
    void setAllPalFx(const StateMachine::PalFxState& state) override { allPalFx_ = state; }
    [[nodiscard]] const StateMachine::PalFxState& bgPalFx() const { return bgPalFx_; }
    [[nodiscard]] const StateMachine::PalFxState& allPalFx() const { return allPalFx_; }

    [[nodiscard]] std::vector<Projectile>& projectiles() { return projectiles_; }
    [[nodiscard]] const std::vector<Projectile>& projectiles() const { return projectiles_; }
    [[nodiscard]] size_t explodCount() const { return explods_.size(); }
    [[nodiscard]] const std::vector<Explod>& explods() const { return explods_; }

    // The shake the engine is applying right now: a drawn offset, in virtual units.
    [[nodiscard]] float shakeOffsetX() const { return shakeX_; }
    [[nodiscard]] float shakeOffsetY() const { return shakeY_; }

private:
    std::vector<Explod> explods_;
    std::vector<Projectile> projectiles_;
    // Indexed by owning side. The reference keeps the triple on the player;
    // this engine has one record per side, updated by whichever pass made the
    // contact. Reset with the rest of the layer at the start of a round.
    ProjectileContactRecord projectileContacts_[2];
    SpriteBank* fxSprites_ = nullptr;
    const AnimationSet* fxAnimations_ = nullptr;
    HelperHost* helperHost_ = nullptr;
    ShadowStyle shadow_;
    bool drawShadows_ = true;
    PauseInfo pause_;
    EnvColorState envColor_;
    StateMachine::PalFxState bgPalFx_;
    StateMachine::PalFxState allPalFx_;
    float cameraMidX_ = 160.0f;
    float floorY_ = 210.0f;
    float projectileStageLeft_ = -1.0e9f;
    float projectileStageRight_ = 1.0e9f;

    int shakeTicks_ = 0;
    float shakeFrequency_ = 60.0f;
    float shakeAmplitude_ = 0.0f;
    long long shakeClock_ = 0;
    float shakeX_ = 0.0f;
    float shakeY_ = 0.0f;
};

// The frame of an animation at a clock, and whether the animation has run out. Shared
// by both entity kinds, and the reason an entity needs no state machine.
struct EntityFrame {
    int group = -1;
    int image = -1;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    // The frame's own `.air` colour-add code. An explod with no `trans` of its
    // own draws with this -- the reference's word for the parameter's absence is
    // "does nothing", and what it does nothing *to* is the animation.
    TransParams trans;
    bool finished = false;
};
[[nodiscard]] EntityFrame entityFrame(const AnimationSet* animations, int anim, long long clock);

// Whether an animation run at `clock` has reached its looptime -- MUGEN's
// `AnimTime = 0`, and the question `removetime = -2` is defined against. Not the
// same as `EntityFrame::finished`: an action that loops reaches its looptime once
// per pass and still never "finishes". See the definition for why that difference
// is the whole reason this exists.
[[nodiscard]] bool entityLoopTimeReached(const AnimationSet* animations, int anim, long long clock);

}  // namespace af
