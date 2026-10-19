// The fight screen: the round flow, the entire HUD read out of fight.def, and
// the two fighters.
//
// Two ways in. A real match arrives with a character for each side and drives
// them through the built-in move set -- the player's pad on one side, the CPU
// on the other, one shared pad on this screen for now. The engine test arrives
// with none and draws placeholder boxes instead, and every part *around* the
// fighters -- lifebars, power bars, portraits, names, timer, banners, win
// icons -- is the same code either way, so a screenpack whose fight.def is
// wrong shows it identically in both.
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engine/continue_flow.h"
#include "engine/defs.h"
#include "engine/entities.h"
#include "engine/fighter.h"
#include "engine/screen.h"
#include "gfx/renderer.h"

namespace af {

class Character;
class Game;
class Stage;

// The ladder an arcade run climbs: the fighter the player picked, and the
// opponents to face in order. This is all MUGEN's arcade mode is -- win and the
// next fighter steps up, lose and the run is over -- so modelling it as a list
// rather than as a mode-specific branch keeps the fight screen's round flow the
// same whichever way it was reached.
struct ArcadeLadder {
    Character* player = nullptr;
    std::vector<Character*> opponents;

    // The palette the player locked in on the select screen, as a 1-based palNo.
    // 0 means they took the character's own default, which is not the same as
    // pal 1: `pal.defaults` names where a character wants to start, and a run
    // that forced pal 1 would quietly ignore it.
    int playerPalNo = 0;

    // One difficulty per opponent, parallel to `opponents`. This is what
    // select.def's `arcade.maxmatches` actually says -- how many fights at each
    // of MUGEN's ten difficulty tiers -- so flattening it to a total, as this
    // engine did at first, threw away the whole point of the key.
    std::vector<int> difficulty;
};

class FightScreen : public Screen, public HelperHost {
public:
    // Placeholder-fighter mode, reached from the title menu's ENGINE TEST.
    FightScreen(Game& game, bool engineTest);

    // A real match. Either pointer may be null, in which case that side falls
    // back to its placeholder: a roster holding one character should still give
    // a fight rather than a refusal.
    //
    // The two palNos are what the select screen locked in, 1-based, and 0 means
    // that side takes its character's own `pal.defaults` -- which is the right
    // answer for a CPU opponent, who never chose.
    //
    // `stage` is the arena the stage select screen handed over. Null keeps the
    // MUGEN rule: the player's select.def entry names its stages, else the
    // first extra stage.
    FightScreen(Game& game, Character* p1, Character* p2, int p1PalNo = 0, int p2PalNo = 0,
                Stage* stage = nullptr);

    // An arcade run. The player is always side 0; each win advances to the next
    // opponent and the run ends when the ladder is cleared or the player loses.
    // The stage, when given, is the one the player picked for the whole run.
    FightScreen(Game& game, ArcadeLadder ladder, Stage* stage = nullptr);

    void enter() override;
    void leave() override;
    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

    // A fight dims the pad when it is not being used. It is the only screen that
    // draws under it -- the stage and the fighters fill the band the pad sits in
    // -- so it is the only one that wants the pad out of the way.
    [[nodiscard]] bool fadesTouchPad() const override { return true; }

private:
    // MUGEN's round sequence: the round banner, then FIGHT!, then the match,
    // then whichever ending applies.
    enum class Phase {
        RoundIntro,
        FightIntro,
        Fight,
        KnockOut,
        TimeOver,
        MatchOver,
        DrawGame,
        // The victory card is up, or on its way up. The card is its own screen,
        // so this phase exists only long enough for the card to pop and hand
        // control back here; the ladder action it was standing in front of is
        // queued in `afterVictory_`.
        Victory,
        // The run-cleared card from `[Win Screen]` is up, or on its way up. It
        // is shown once, after the final match's victory card (or directly when
        // the screenpack disabled Victory), and is the last thing before the
        // end-of-run hold. A phase of its own so the card cannot be pushed a
        // second time while it is on the stack.
        Win,
        // The `[Continue Screen]` prompt is up after a lost arcade match. Like
        // Victory and Win it holds the fight until it pops; `continueResult_`
        // then says whether to restart this match or end the run. The phase is
        // also the guard that stops a second prompt being pushed for one loss.
        Continue,
        // The arcade ladder is finished and won. A phase of its own rather than
        // an immediate exit, because clearing a run is the one thing the game
        // never otherwise says out loud.
        ArcadeClear,
        // One of the screenpack's storyboards owns the screen: the Game Over
        // scene after a lost run, or the optional Default Ending / End Credits
        // scenes after a cleared one. Each is held until its storyboard pops and
        // the pop handler at the top of update() decides what comes next.
        GameOver,
        DefaultEnding,
        EndCredits,
    };

    // What happens once the victory card has finished. Kept separate from
    // `matchDecided_` because the card is shown *before* the ladder moves, and
    // MatchOver's own condition (`phaseTicks_ >= winTime`) is still true when
    // the card pops -- running advanceOrLeave() again would push a second card
    // for the same win.
    enum class AfterVictory {
        None,
        NextOpponent,
        ArcadeClear,
    };

    // Lays out the arena and both sides for one match. Called once on entry,
    // and again for each new opponent on an arcade run -- which is why it is
    // separate from enter().
    //
    // `showVersusCard` is false only for a continue: the VS card introduces a
    // new match, and accepting a continue resumes the match that was just lost.
    // It is a parameter rather than a member because it is a property of the
    // call, not of the screen.
    void setupMatch(bool showVersusCard = true);
    void startRound();
    void endRound(int winner);  // -1 for a draw, 0 or 1 for a fighter

    // Puts each side into the palette its player picked on the select screen.
    // Called immediately after *every* `Fighter::setup`, not once per match:
    // setup is what resets `palNo_` to the character's own `pal.defaults`, and
    // it runs again at the top of each round, so a palette applied any earlier
    // would survive exactly one round.
    void applyChosenPalettes();

    // A decided match: climb to the next opponent, or leave the screen.
    void advanceOrLeave();

    // Ends a cleared arcade run. `[Win Screen]` and `[Victory Screen]` are
    // independent records, so this is the one place that decides between "show
    // the win card, then ArcadeClear" and "go straight to ArcadeClear". It is
    // reached both from the victory card's pop and from the path where the
    // screenpack disabled Victory, which is why it is not inlined there.
    void finishArcadeRun();

    // Ends a lost arcade run: the Game Over gate, then the title. Reached from
    // a declined or expired continue, and from a loss with no Continue screen
    // enabled.
    void endArcadeRun();

    // Pushes the storyboard a switch names and parks this screen in `phase`.
    // Returns false when the switch is off, the path is empty, or the file
    // could not be loaded -- the callers then keep their old fallback. The
    // caller must not have set `phase_` yet: this sets it only on success.
    bool pushStoryboard(const StoryboardSwitch& sw, Phase phase);

    // The end-of-run chain after the Win card (or directly after the final
    // victory). `allowDefaultEnding` is true only on the first call: the
    // Default Ending is shown once, and when it pops this runs again with
    // `false` so the chain moves on to End Credits instead of replaying it.
    // Ends at ArcadeClear when no storyboard is enabled or loadable.
    void advanceEndSequence(bool allowDefaultEnding);

    // Accepting a continue restarts the match that was just lost. Same ladder
    // step, same `matchno`, no VS card. This is the smallest honest reuse of
    // `setupMatch`; see the definition for why it is not a separate copy.
    void restartMatch();

    // Pops the fight and the select screen together, which is how MUGEN's
    // "return to the title" is expressed on this stack. See the definition.
    void returnToTitle();

    // The match index this fight reports as `matchno`. Versus and the engine
    // test are always match 1; an arcade run counts the opponent it is on, which
    // moves only when a match is won.
    [[nodiscard]] int matchNo() const;

    // Raised by setupMatch() and cleared by the first update() after it, which
    // is where the VS card is pushed from. See the definition for why it is not
    // pushed from enter().
    bool cardPending_ = false;
    // The ladder move the victory card is holding back. See `AfterVictory`.
    AfterVictory afterVictory_ = AfterVictory::None;
    // The explods, the projectiles and the screen shake this fight has out. Belongs to
    // the fight rather than to either fighter, because a beam outlives the move that fired
    // it and a spark belongs to the impact rather than to the arm that made it.
    EntityLayer entities_;
    // Whether a projectile hit landed this tick, for the damage counter.
    bool projectileLanded_[2] = {false, false};

    // --------------------------------------------------------------- helpers
    //
    // The helpers both sides have out: second fighters spawned by the Helper
    // controller, running their character's own states, owned here and swept when
    // they DestroySelf. They fight, draw and die like fighters because in MUGEN
    // they *are* fighters -- the only difference a root has is a pad.
    StateMachine* spawnHelper(const HelperSpec& spec) override;
    [[nodiscard]] StateMachine* playerById(int id) override;
    std::vector<std::unique_ptr<Fighter>> helpers_;
    // Ids handed to helpers as they spawn. Roots hold MUGEN's own 1 and 2, so a
    // helper never collides with one; MUGEN's real ids are large and arbitrary and
    // nothing written against them depends on the value.
    int nextPlayerId_ = 1000;
    // Removes the destroyed, and detaches them from their family's list first so a
    // redirect can never point at a fighter that is gone.
    void sweepHelpers();

    // The fight's own window: the screenpack's height, the device's aspect. See the
    // definition for why the fight is allowed to fill the display and the menus are not.
    static constexpr float kWindowHeight = 240.0f;
    [[nodiscard]] float windowWidth() const;
    void applyWindow();
    // The width last handed to the renderer, so a resize is noticed and a steady frame is
    // not re-set every tick.
    float appliedWindowWidth_ = 0.0f;

    void drawStage(Renderer& renderer, float scrollX);
    void drawFighters(Renderer& renderer, float scrollX, bool drawShadows);
    // `globalnoshadow` is engine-global in the reference and per-machine in this
    // engine, so the fight screen answers "did any running machine assert it this
    // tick?" by OR-ing both fighters and every helper. See SHADOW-RE §2.2.
    [[nodiscard]] bool anyGlobalNoShadow() const;
    void drawHud(Renderer& renderer);
    // The health bar's two extra quantities, which MUGEN has no equivalent of and Shin
    // Budokai's bar is drawn against: the yellow trail of damage just taken, which is
    // eaten away over the next second, and the red part of it that a blocked fighter
    // can still recover by not being hit again.
    int lifeTrail_[2] = {0, 0};
    int recoverable_[2] = {0, 0};
    int lastLife_[2] = {0, 0};
    // The damage counter: how many hits the current combo is, how much it has done, and
    // how long since the last one, so that the readout clears itself.
    int comboHits_[2] = {0, 0};
    int comboDamage_[2] = {0, 0};
    int comboTicks_[2] = {0, 0};
    void drawFighterHudFace(Renderer& renderer, const FaceSide& face, Character* character,
                            const std::string& fallbackName);
    void drawBars(Renderer& renderer, const BarSide& p1, const BarSide& p2, int valueP1,
                  int valueP2, int maxP1, int maxP2, bool showCounters);
    void drawWinIcons(Renderer& renderer);
    void drawBanners(Renderer& renderer);

    // The camera's travel, in this screen's coordinates. Taken from the stage
    // when one loaded, so a narrow stage keeps the fighters on its art.
    [[nodiscard]] std::pair<float, float> cameraBounds() const;

    // Where each side stands, resolved from the stage. Held rather than
    // recomputed because a stage's start positions must survive a new round:
    // re-reading them per round would be the same answer every time, and
    // hard-coding them here is how the stage's own values got lost once.
    float startX_[2] = {100.0f, 220.0f};

    Game& game_;
    bool engineTest_ = false;

    // Set only for an arcade run. An exhibition match leaves this empty and
    // ends after one match, the way VS mode does.
    ArcadeLadder ladder_;
    std::size_t ladderStep_ = 0;

    // The arena. Null when no stage file loaded at all, which is the one case
    // that still falls back to the procedural test stage.
    Stage* stage_ = nullptr;

    // The stage the player picked on the stage select screen, kept across an
    // arcade run so every opponent is fought in the arena they chose. Null
    // means "let the roster decide", which is the MUGEN behaviour.
    Stage* stageOverride_ = nullptr;

    // Where fighters stand when no stage says. MUGEN's usual value for a
    // 320x240 screenpack; a stage's own zoffset overrides it.
    static constexpr float kDefaultFloorY = 190.0f;
    float floorY_ = kDefaultFloorY;

    // The stage's local coordinate space translated into this screen's units,
    // from stageTranslation(). A 1280x720 stage becomes a third as large on a
    // 240-high fight window; a 320x240 stage is 1:1. Kept for the match because
    // the stage's numbers are read once and used by the floor, the starts, the
    // camera and the draw.
    Vec2 stageScale_{1.0f, 1.0f};

    // The x range both fighters may walk to, resolved for this match from the
    // stage's `[Bound]` margins, its camera travel and `[PlayerInfo]`
    // leftbound/rightbound. Only used when a stage loaded; without one the
    // fighters keep the procedural arena's own limits.
    float fighterLeftBound_ = 0.0f;
    float fighterRightBound_ = 0.0f;

    // The stage's shadow settings, or MUGEN's defaults when no stage loaded.
    ShadowStyle stageShadow_;

    // The characters this fight was constructed with, kept for the HUD's
    // portrait lookups (group 9000 of each side's own sprite file).
    Character* characters_[2] = {nullptr, nullptr};

    Fighter fighters_[2];
    int roundsWon_[2] = {0, 0};

    // The palNos the select screen locked in, one per side; 0 is "not chosen",
    // which leaves the character in its own default. Held here rather than read
    // off the screen because the select screen is gone by the time the fight
    // sets up, and it is re-applied at each round rather than once.
    int palNo_[2] = {0, 0};

    // The camera's stage position: between the fighters, clamped to the
    // stage's ends. It starts centred and moves only once the fight does.
    float camera_ = 320.0f;

    Phase phase_ = Phase::RoundIntro;
    int phaseTicks_ = 0;
    // One Continue prompt per loss. Set when the prompt is pushed and cleared
    // when a new match is set up (including a restart after a continue); the
    // phase alone already blocks a second push while the card is up, and this
    // keeps a stray call on the match-decided path from offering it again.
    bool continueOffered_ = false;
    // Written by ContinueScreenView just before it pops, read by the Phase::
    // Continue handler. Default `Declined` is what a prompt that never wrote a
    // result (an unexpected teardown) will leave behind: ending the run is the
    // safer failure.
    ContinueResult continueResult_ = ContinueResult::Declined;
    int round_ = 1;
    int timer_ = 99;       // displayed seconds
    int timerTicks_ = 0;   // ticks toward the next displayed second
    long long tick_ = 0;
    bool matchDecided_ = false;
    int winner_ = -1;
    std::string winMethod_;  // "n", "perfect", ...
};

}  // namespace af