#include "engine/screens/fight_screen.h"

#include "engine/screens/continue_screen.h"
#include "engine/screens/storyboard_screen.h"
#include "engine/screens/versus_screen.h"
#include "engine/screens/victory_screen.h"
#include "engine/screens/win_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

#include "core/log.h"
#include "engine/engine.h"
#include "engine/character.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "engine/projectile_rules.h"
#include "engine/stage.h"
#include "gfx/font.h"

namespace af {

namespace {

constexpr float kP1StartX = 240.0f;
constexpr float kP2StartX = 400.0f;

// The camera's travel when no stage says.
constexpr float kCameraMin = 160.0f;
constexpr float kCameraMax = 480.0f;

// Where the middle of the arena is in this screen's coordinates.
//
// A stage writes its coordinates with 0 at the middle of the arena, because
// that is where MUGEN's camera origin is; this screen counts from the far left
// of its world instead. The two are related by this one offset, which is why it
// is a named constant rather than a bare number at each use. It has to agree
// with kP1StartX/kP2StartX and kCameraMin/kCameraMax, which all put the arena's
// middle at 320 -- a value that disagrees by even half the width slides the
// stage art and every fighter sideways by the difference.
constexpr float kStageOriginX = 320.0f;

constexpr int kPowerMax = 3000;
constexpr int kRoundSeconds = 99;

// How long the arcade-clear banner holds. Longer than a round banner: the run
// is over, and this is the only moment that says so.
constexpr int kArcadeClearTicks = 180;

constexpr float kBoxWidth = 26.0f;
constexpr float kBoxHeight = 62.0f;

constexpr Color kGroundTop = Color::rgba8(34, 40, 58);
constexpr Color kGroundBottom = Color::rgba8(18, 22, 34);
constexpr Color kSkyTop = Color::rgba8(24, 30, 52);
constexpr Color kSkyBottom = Color::rgba8(74, 62, 96);
constexpr Color kShadow = Color{0.0f, 0.0f, 0.0f, 0.35f};

// How much life one press of an attack button takes off in the engine test,
// where the fighters are boxes with no moves. Chosen so a round is a few dozen
// presses: long enough to watch the bar drain, short enough to reach a KO.
constexpr int kTestDamage = 34;

int winsNeeded(const RoundSection& round) { return std::max(1, round.matchWins); }

// MUGEN requires `facing` on each bar layer rather than once for the side, and a
// screenpack is free to set it on some layers and not others. The fill is the
// layer that matters, so that is where the side's direction is read from.
int barFacing(const BarSide& bar) {
    if (bar.front.present) return bar.front.facing;
    if (bar.bg1.present) return bar.bg1.facing;
    if (bar.bg0.present) return bar.bg0.facing;
    return 1;
}

}  // namespace

FightScreen::FightScreen(Game& game, bool engineTest) : game_(game), engineTest_(engineTest) {}

FightScreen::FightScreen(Game& game, Character* p1, Character* p2, int p1PalNo, int p2PalNo,
                         Stage* stage)
    : game_(game), engineTest_(false), stageOverride_(stage) {
    characters_[0] = p1;
    characters_[1] = p2;
    palNo_[0] = p1PalNo;
    palNo_[1] = p2PalNo;
}

FightScreen::FightScreen(Game& game, ArcadeLadder ladder, Stage* stage)
    : game_(game), engineTest_(false), ladder_(std::move(ladder)), stageOverride_(stage) {
    characters_[0] = ladder_.player;
    if (!ladder_.opponents.empty()) characters_[1] = ladder_.opponents.front();
    // The player picked their colour; every CPU opponent along the ladder takes
    // its own character's default, which is what leaving palNo_[1] at 0 does.
    palNo_[0] = ladder_.playerPalNo;
}

// The fight's own window: the screenpack's height, the device's width.
//
// A MUGEN screenpack is authored for 320x240 and the engine letterboxes it, which on a
// phone held sideways leaves a third of the display black on either side. A *fight* has
// no reason to accept that. The stages tile their layers horizontally and the camera
// follows the fighters, so a wider window simply shows more stage, and every gauge on
// this screen is drawn in code rather than at a coordinate a screenpack picked. So the
// fight is given a window with the device's aspect at the screenpack's height, which maps
// onto the display exactly: no bars, no stretching, and the HUD -- laid out as fractions
// of the width -- ends up at the screen's edges on any phone.
//
// The menus keep the screenpack's own 320x240, because their layout *is* the screenpack's
// and a wider window would leave them bunched against the left edge.
float FightScreen::windowWidth() const {
    if (engine() == nullptr) return 320.0f;
    const Renderer& renderer = engine()->renderer();
    const float height = static_cast<float>(std::max(1, renderer.framebufferHeight()));
    const float aspect = static_cast<float>(renderer.framebufferWidth()) / height;
    // Clamped, because an extreme aspect -- a fold-out, or the phone held upright -- should
    // widen the view rather than give a fighting game a window it cannot fill.
    return std::clamp(kWindowHeight * aspect, 320.0f, 560.0f);
}

void FightScreen::applyWindow() {
    if (engine() == nullptr) return;
    const float width = windowWidth();
    if (std::fabs(width - appliedWindowWidth_) < 0.5f) return;
    appliedWindowWidth_ = width;
    engine()->renderer().setVirtualSize(width, kWindowHeight);
    engine()->renderer().setAspectMode(AspectMode::Fit);
}

void FightScreen::enter() {
    applyWindow();
    ladderStep_ = 0;
    setupMatch();
}

void FightScreen::leave() {
    // Hand the screenpack's own space back for the menus.
    if (engine() == nullptr) return;
    appliedWindowWidth_ = 0.0f;
    const Vec2 local = game_.system().localCoord;
    if (local.x > 0.0f && local.y > 0.0f) engine()->renderer().setVirtualSize(local.x, local.y);
    engine()->renderer().setAspectMode(game_.config().aspect);
}

// The run's counter, not the round's. An exhibition or engine-test fight is a
// single match; an arcade ladder is one match per opponent, counted from 1.
int FightScreen::matchNo() const {
    return ladder_.opponents.empty() ? 1 : static_cast<int>(ladderStep_) + 1;
}

void FightScreen::setupMatch(bool showVersusCard) {
    // The ladder decides who is on which side; an exhibition match was handed
    // its two fighters at construction.
    if (!ladder_.opponents.empty()) {
        characters_[0] = ladder_.player;
        characters_[1] = ladder_.opponents[std::min(ladderStep_, ladder_.opponents.size() - 1)];
    }

    // A character names itself; a side without one is the placeholder the
    // engine test draws as a box, and names itself the same way.
    const char* kFallbackNames[2] = {
        engineTest_ ? "TEST P1" : "PLAYER 1",
        engineTest_ ? "TEST P2" : "PLAYER 2",
    };
    const int kStartFacing[2] = {1, -1};

    roundsWon_[0] = roundsWon_[1] = 0;

    // The sparks, from the screenpack's own fightfx pair: the entities need to be able to
    // draw an animation that belongs to no character, which is where every HitDef's plain
    // `sparkno` points.
    entities_.setFightFx(&game_.fightFxSprites(), &game_.fightFxAnimations());
    // And the helpers, which this screen turns into real fighters.
    entities_.setHelperHost(this);
    helpers_.clear();

    // The stage is resolved before anything is placed, because it decides where
    // the floor is and where the two fighters start. A stage that fails to load
    // leaves both to this screen's own defaults, which is what keeps the engine
    // test's procedural arena working.
    //
    // A player-chosen stage wins over the roster's own list: the stage select
    // screen exists because the roster names no stages, and its answer is the
    // whole point of that screen.
    stage_ = stageOverride_ != nullptr ? stageOverride_ : game_.stageForMatch(characters_[0]);
    stageScale_ = Vec2{1.0f, 1.0f};
    stageShadow_ = ShadowStyle{};
    floorY_ = kDefaultFloorY;
    if (stage_ != nullptr) {
        const StageDef& def = stage_->def();
        // Every number in a stage is written in the stage's own coordinate
        // space. A 1280x720 stage puts its floor at y=670 and starts its
        // fighters at x=+-280; read as fight units those place the floor below
        // a 240-high window and the fighters off its sides. The translation is
        // the ratio of the window's height to the stage's, which is what keeps
        // `zoffset` at the fraction of the screen its author drew it at (see
        // stageTranslation()).
        const StageTranslation translation = stageTranslation(def, kWindowHeight);
        stageScale_ = Vec2{translation.x, translation.y};
        floorY_ = def.zOffset * translation.y;
        // `[Shadow] fade.range` is in the same stage-y units as the fighters'
        // heights, so it moves with them rather than staying in stage pixels.
        stageShadow_ = def.shadow;
        stageShadow_.fadeTop *= translation.y;
        stageShadow_.fadeBottom *= translation.y;
        AF_LOGI("fight: stage \"%s\" at %gx%g localcoord: scale %.3f,%.3f floor %.0f, "
                "camera %.0f..%.0f",
                def.name.c_str(), static_cast<double>(def.localCoord.x),
                static_cast<double>(def.localCoord.y), static_cast<double>(translation.x),
                static_cast<double>(translation.y), static_cast<double>(floorY_),
                static_cast<double>(def.boundLeft * translation.x),
                static_cast<double>(def.boundRight * translation.x));
    } else {
        AF_LOGI("fight: no stage loaded; using the procedural test arena");
    }
    // `projstagebound` is measured from the stage's edges (§6.1), but the entity
    // layer owns no stage, so it is handed the arena's world x range here. With
    // no stage the camera's own travel stands in, which keeps a beam's default
    // bound meaningful in the procedural test arena.
    entities_.setProjectileStageBounds(
        stage_ != nullptr ? kStageOriginX + stageScale_.x * stage_->def().boundLeft : kCameraMin,
        stage_ != nullptr ? kStageOriginX + stageScale_.x * stage_->def().boundRight : kCameraMax);

    // How hard the CPU plays this match. An exhibition match gets the middle of
    // the range, which is where the AI was tuned before difficulty existed.
    int level = 5;
    if (!ladder_.opponents.empty() && ladderStep_ < ladder_.difficulty.size()) {
        level = ladder_.difficulty[ladderStep_];
    }
    fighters_[1].setAiDifficulty(level);
    if (!ladder_.opponents.empty()) {
        AF_LOGI("arcade: match %zu of %zu at difficulty %d", ladderStep_ + 1,
                ladder_.opponents.size(), level);
    }

    startX_[0] = stage_ != nullptr
                     ? kStageOriginX + stage_->def().p1Start.x * stageScale_.x
                     : kP1StartX;
    startX_[1] = stage_ != nullptr
                     ? kStageOriginX + stage_->def().p2Start.x * stageScale_.x
                     : kP2StartX;

    for (int i = 0; i < 2; ++i) {
        Character* character = characters_[i];
        const float x = startX_[i];
        fighters_[i].setup(character, x, kStartFacing[i]);
        // Explods and projectiles go to the fight's own layer, and each fighter is told
        // which side it is on so a bound aura follows the right one in a mirror match.
        fighters_[i].setEntitySink(&entities_);
        fighters_[i].setSide(i);
        fighters_[i].machine().setPlayerId(i + 1);
        // Every sound this fighter makes goes to the one mixer, and its own
        // sound file is registered ahead of the screenpack's so a character that
        // did ship one is heard instead of the shared hit sound.
        if (engine() != nullptr) {
            if (characters_[i] != nullptr) engine()->mixer().addBank(&characters_[i]->sounds());
            fighters_[i].setSoundSink(&engine()->mixer());
        }
        const Color body = i == 0 ? Color::rgba8(220, 110, 90) : Color::rgba8(90, 140, 220);
        const Color trim = i == 0 ? Color::rgba8(255, 210, 170) : Color::rgba8(190, 220, 255);
        fighters_[i].setFallbackLook(kFallbackNames[i], body, trim);
    }
    applyChosenPalettes();
    camera_ = (startX_[0] + startX_[1]) * 0.5f;

    // Stage walk limits: the camera's travel plus MUGEN's `[Bound]` margins, so
    // a fighter can never stand in the strip past the art when the camera has
    // run out of travel, further tightened by the stage's own `[PlayerInfo]`
    // hard bounds. Without a stage the fighters keep their procedural limits.
    if (stage_ != nullptr) {
        const StageDef& def = stage_->def();
        const auto [cameraLow, cameraHigh] = cameraBounds();
        const float halfWidth = windowWidth() * 0.5f;
        const float screenLow = cameraLow - halfWidth + def.screenLeft * stageScale_.x;
        const float screenHigh = cameraHigh + halfWidth - def.screenRight * stageScale_.x;
        const float hardLow = kStageOriginX + def.playerLeftBound * stageScale_.x;
        const float hardHigh = kStageOriginX + def.playerRightBound * stageScale_.x;
        fighterLeftBound_ = std::max(screenLow, hardLow);
        fighterRightBound_ = std::min(screenHigh, hardHigh);
        // A stage whose numbers contradict each other must not weld a fighter
        // in place; the camera's own ends are the safe fallback.
        if (!(fighterRightBound_ > fighterLeftBound_)) {
            fighterLeftBound_ = cameraLow;
            fighterRightBound_ = cameraHigh;
        }
        for (int i = 0; i < 2; ++i) {
            fighters_[i].setStageBounds(fighterLeftBound_, fighterRightBound_);
        }
    }

    // A character is measured in its own coordinate space; a fighter authored
    // for a 1280x720 localcoord is drawn half-size next to one authored for
    // 640x360, and neither file has to know about the other.
    const Vec2 screenCoord = game_.system().localCoord;
    for (int i = 0; i < 2; ++i) {
        if (characters_[i] == nullptr) continue;
        const Vec2 own = characters_[i]->localCoord();
        if (own.x > 0.0f && own.y > 0.0f && screenCoord.y > 0.0f) {
            fighters_[i].setScale(screenCoord.y / own.y);
        }
    }

    round_ = 1;
    matchDecided_ = false;
    winner_ = -1;
    winMethod_.clear();
    // A quote chosen in the previous match must not reach this one's card:
    // "This controller only affects the victory screen immediately following
    // the current match" (sctrls.html). This is the per-match reset -- not
    // StateMachine::setup, which runs again at every round of the same match
    // and deliberately leaves the selection alone.
    for (int i = 0; i < 2; ++i) fighters_[i].machine().resetVictoryQuote();
    // A new match has a fresh set of losses, so it may offer its own continue.
    continueOffered_ = false;
    cardPending_ = showVersusCard;
    startRound();
    AF_LOGI("fight: %zu HUD animations in fight.def",
            game_.fight().animations.size());
}

// -------------------------------------------------------------------- helpers

StateMachine* FightScreen::spawnHelper(const HelperSpec& spec) {
    // A helper without a character is nothing at all: its states, sprites and
    // sounds are all the spawner's character's.
    if (spec.character == nullptr) return nullptr;
    // MUGEN caps how many helpers one match carries, because a character that
    // spawns one every tick of a looping state would otherwise take the machine
    // down. 56 is the engine's own limit.
    if (helpers_.size() >= 56) return nullptr;

    auto helper = std::make_unique<Fighter>();
    helper->setup(spec.character, spec.x, spec.facing);
    // A helper is a fighter in every respect, walk limits included.
    if (stage_ != nullptr) helper->setStageBounds(fighterLeftBound_, fighterRightBound_);
    helper->machine().setHeight(spec.y);
    helper->machine().setSide(spec.side);
    helper->machine().setEntitySink(&entities_);
    helper->machine().setPlayerId(nextPlayerId_++);
    helper->machine().setRoundNo(round_);
    helper->machine().setMatchNo(matchNo());
    if (engine() != nullptr) {
        helper->machine().setSoundSink(&engine()->mixer());
    }
    // Drawn at the root's scale: a helper is the same character, in every unit.
    const int side = (spec.side == 0 || spec.side == 1) ? spec.side : 0;
    helper->setScale(fighters_[side].baseScale());
    // The family links and the entry state go on last, after the machine is a
    // working root: becomeHelper rewires what setup established.
    helper->machine().becomeHelper(spec.spawner, spec.helperId, spec.name, spec.keyCtrl,
                                   spec.pauseMoveTime, spec.superMoveTime, spec.stateNo);
    // Stage bounds and opponent come from the fight around it, as the roots' do.
    helper->machine().setOpponent(&fighters_[side == 0 ? 1 : 0].machine());
    AF_LOGD("fight: helper \"%s\" (id %d) spawned for side %d in state %d",
            spec.name.c_str(), spec.helperId, spec.side, spec.stateNo);

    StateMachine* machine = &helper->machine();
    helpers_.push_back(std::move(helper));
    return machine;
}

StateMachine* FightScreen::playerById(int id) {
    for (Fighter& fighter : fighters_) {
        if (fighter.machine().playerId() == id) return &fighter.machine();
    }
    for (const auto& helper : helpers_) {
        if (helper->machine().playerId() == id) return &helper->machine();
    }
    return nullptr;
}

void FightScreen::sweepHelpers() {
    for (size_t i = helpers_.size(); i-- > 0;) {
        // doomed rather than destroyed: a helper whose spawner chain ended goes
        // with it, which is how a flash double never outlives the flash.
        if (!helpers_[i]->machine().doomed()) continue;
        StateMachine* going = &helpers_[i]->machine();
        // Every link that points at it goes first, in every machine that stays:
        // a target redirect or a bind anchor into a freed machine reads garbage.
        for (Fighter& fighter : fighters_) fighter.machine().clearReferencesTo(going);
        for (auto& staying : helpers_) {
            if (staying.get() != helpers_[i].get()) staying->machine().clearReferencesTo(going);
        }
        going->removeFamilyHelper(going);
        helpers_.erase(helpers_.begin() + static_cast<ptrdiff_t>(i));
    }
}

void FightScreen::applyChosenPalettes() {
    for (int i = 0; i < 2; ++i) {
        // Nothing chosen leaves the machine's own palNo_ alone, and that is not
        // an oversight: `setup` has just set it from the character's
        // `pal.defaults`, which is the colour its author drew it to be.
        if (palNo_[i] < 1) continue;
        fighters_[i].machine().setPalNo(palNo_[i]);
    }
}

void FightScreen::startRound() {
    // The helpers die with the round they were spawned in: a super flash from
    // round one must not open round two. Cleared before the roots re-setup, so
    // no root ever holds a pointer to a machine that is gone.
    helpers_.clear();
    // Every round starts the fighters where the stage put them. Reading the
    // defaults here instead is what would make round two ignore the stage.
    const int match = matchNo();
    for (int i = 0; i < 2; ++i) {
        const int facing = i == 0 ? 1 : -1;
        // `roundno` and `matchno` are how a character scales itself up over a
        // match, and `setup()` reads the round number itself: `[State 5900]`'s
        // `VarRangeSet` is guarded by `roundsexisted = 0`, so the first round
        // clears the character variables and later rounds keep them. The order
        // here is therefore load-bearing.
        fighters_[i].machine().setRoundNo(round_);
        fighters_[i].machine().setMatchNo(match);
        fighters_[i].setup(characters_[i], startX_[i], facing);
        fighters_[i].machine().setRoundOutcome(-1);
        // A match is only over once its deciding round has played out, so no
        // new round or new opponent starts with the previous verdict still set.
        fighters_[i].machine().setMatchOver(false);
    }
    applyChosenPalettes();
    timer_ = kRoundSeconds;
    timerTicks_ = 0;
    entities_.clear();
    // The gauges that describe a round rather than a fighter. A trail from the last
    // round's damage would otherwise be drawn over a full life bar.
    for (int i = 0; i < 2; ++i) {
        lifeTrail_[i] = 0;
        recoverable_[i] = 0;
        lastLife_[i] = fighters_[i].life();
        comboHits_[i] = 0;
        comboDamage_[i] = 0;
        comboTicks_[i] = 0;
    }
    phase_ = Phase::RoundIntro;
    phaseTicks_ = 0;
    winner_ = -1;
    winMethod_.clear();
}

void FightScreen::endRound(int winner) {
    winner_ = winner;
    if (winner >= 0) {
        ++roundsWon_[winner];
        // A perfect round is one where the winner never lost any life.
        winMethod_ = fighters_[winner].tookDamage() ? "n" : "perfect";
    } else {
        winMethod_ = "n";
    }
    // The machines' own verdicts, for their win/lose triggers.
    for (int i = 0; i < 2; ++i) {
        fighters_[i].machine().setRoundOutcome(winner < 0 ? 2 : (winner == i ? 1 : 0));
    }

    matchDecided_ = winner >= 0 && roundsWon_[winner] >= winsNeeded(game_.fight().round);
    AF_LOGI("fight: round %d over, winner %d (%s)", round_, winner, winMethod_.c_str());
}

void FightScreen::update(double dt, const InputState& input) {
    // The device can change shape underneath a running fight -- a rotation, a fold-out --
    // and the window is the device's aspect, so it has to be recomputed rather than set
    // once on entry.
    applyWindow();
    // The VS card plays before the round does.
    //
    // Pushed from here rather than from enter() because enter() runs while the
    // engine is walking its pending-push list, and pushing from inside that walk
    // would add to the list being iterated.
    //
    // Nothing else is needed to hold the round: the engine only updates the top
    // of the stack, so while the card is up no tick reaches this screen, the
    // fighters do not move and no input is read. That is the whole reason the
    // card is a screen rather than an overlay.
    if (cardPending_) {
        cardPending_ = false;
        if (engine() != nullptr) {
            engine()->pushScreen(std::make_unique<VersusScreen>(
                game_, characters_[0], characters_[1],
                static_cast<int>(ladderStep_) + 1));
        }
        return;
    }

    // The victory card has popped, so this is the first tick that can act on
    // the win it was covering. The card was pushed from advanceOrLeave(), which
    // returned before the ladder moved; doing the move there would have set up
    // the next match underneath a screen that was still on top. Doing it here,
    // once, is what "advance exactly once" means: the queued action is cleared
    // before it runs and `phase_` leaves Victory with it.
    if (phase_ == Phase::Victory) {
        const AfterVictory next = afterVictory_;
        afterVictory_ = AfterVictory::None;
        switch (next) {
            case AfterVictory::NextOpponent:
                ++ladderStep_;
                setupMatch();
                return;
            case AfterVictory::ArcadeClear:
                // The final match is won. The run-cleared card, if the
                // screenpack enabled it, comes now -- once -- and ArcadeClear
                // waits until it has popped. The two gates are independent:
                // this path is only reached when Victory was shown, but the
                // no-Victory path below reaches the same helper.
                finishArcadeRun();
                return;
            case AfterVictory::None:
                // No queued action means nothing was waiting behind the card.
                // Fall back to the decided-match hold rather than leave the
                // fight stuck in a phase that cannot tick.
                phase_ = Phase::MatchOver;
                break;
        }
    }

    // The Continue prompt has popped, so the loss it was covering can be acted
    // on. This is the only transition out of Phase::Continue, which is what
    // keeps the prompt from being pushed twice for one loss.
    if (phase_ == Phase::Continue) {
        if (continueResult_ == ContinueResult::Accepted) {
            // SCREEN-FLOW-RE §3.2 step 6a: an accepted continue resumes the
            // fight loop without advancing the match number. The prompt drew in
            // the screenpack's own space, so restore the fight's wide window
            // before this frame is drawn, exactly as the Win card does.
            appliedWindowWidth_ = 0.0f;
            restartMatch();
            applyWindow();
        } else {
            endArcadeRun();
        }
        return;
    }

    // The Win card has popped, so the run is ready for the end-of-run hold.
    // This is the only transition out of Phase::Win, which is what keeps the
    // card from being pushed more than once for a cleared run.
    if (phase_ == Phase::Win) {
        // The card drew in the screenpack's own 320x240 space, so the wide
        // window this cache believes is applied has been replaced underneath
        // it. Clearing the cache makes applyWindow() restore the fight's window
        // now rather than leaving the end-of-run banner laid out for 320x240.
        appliedWindowWidth_ = 0.0f;
        applyWindow();
        // SCREEN-FLOW-RE §3.2: a cleared run may continue into `[Default
        // Ending]` and then `[End Credits]`. Both are storyboard switches, and
        // this engine has no per-character ending of its own, so "the character
        // has no ending" is always true here and the default ending is eligible.
        advanceEndSequence(true);
        return;
    }

    // A storyboard has popped. Each phase it can park the fight in has one
    // successor, and this is the only transition out of each -- which is what
    // keeps a storyboard from being pushed twice for one event.
    if (phase_ == Phase::GameOver) {
        returnToTitle();
        return;
    }
    if (phase_ == Phase::DefaultEnding) {
        // The storyboard drew in its own coordinate space, so the window this
        // screen believes is applied has been replaced underneath it. Clearing
        // the cache makes the next applyWindow() restore the fight's wide view
        // for the credits or the banner.
        appliedWindowWidth_ = 0.0f;
        applyWindow();
        // Shown once. Continue down the chain: End Credits, then the banner.
        advanceEndSequence(false);
        return;
    }
    if (phase_ == Phase::EndCredits) {
        // Same restore before the end-of-run banner draws in the fight's space.
        appliedWindowWidth_ = 0.0f;
        applyWindow();
        phase_ = Phase::ArcadeClear;
        phaseTicks_ = 0;
        return;
    }

    (void)dt;
    ++tick_;
    ++phaseTicks_;

    // The round's state rides on both machines every tick: 1 while the banners
    // play, 2 while the fighting is live, 3 through the KO spill, 4 when decided.
    int roundState;
    switch (phase_) {
        case Phase::RoundIntro:
        case Phase::FightIntro:
            roundState = 1;
            break;
        case Phase::Fight:
            roundState = 2;
            break;
        case Phase::KnockOut:
        case Phase::TimeOver:
            roundState = 3;
            break;
        default:
            roundState = 4;
            break;
    }
    for (int i = 0; i < 2; ++i) {
        fighters_[i].machine().setRoundState(roundState);
    }
    // `matchover` is the reference's gate, not just "the match is decided": it
    // stays false until the deciding round has finished and the winner is in
    // its victory pose. This engine has no separate state-180 phase -- the
    // winner is stepped through the KO spill and then held in MatchOver -- so
    // that phase is the nearest equivalent, and Victory/ArcadeClear (which
    // follow it) keep the answer true through the card and the end-of-run hold.
    // A draw never decides a match, so `matchDecided_` keeps it false there.
    const bool matchOver =
        matchDecided_ && (phase_ == Phase::MatchOver || phase_ == Phase::Victory ||
                          phase_ == Phase::Win || phase_ == Phase::Continue ||
                          phase_ == Phase::ArcadeClear || phase_ == Phase::GameOver ||
                          phase_ == Phase::DefaultEnding || phase_ == Phase::EndCredits);
    for (int i = 0; i < 2; ++i) {
        fighters_[i].machine().setMatchOver(matchOver);
    }
    for (const auto& helper : helpers_) {
        helper->machine().setRoundState(roundState);
        helper->machine().setMatchOver(matchOver);
    }

    const RoundSection& round = game_.fight().round;

    switch (phase_) {
        case Phase::RoundIntro:
            if (phaseTicks_ >= std::max(1, round.roundDisplayTime)) {
                phase_ = Phase::FightIntro;
                phaseTicks_ = 0;
            }
            break;

        case Phase::FightIntro:
            // `fight.time` is how long after the FIGHT! banner control starts.
            if (phaseTicks_ >= std::max(1, round.fight.displaytime)) {
                phase_ = Phase::Fight;
                // MUGEN's round-start sound: `fight.snd = 1,0` in fight.def.
                if (engine() != nullptr) {
                    engine()->mixer().playFrom(game_.fightSounds(), 1, 0);
                }
                phaseTicks_ = 0;
            }
            break;

        case Phase::Fight: {
            // The clock. `framespercount` is how many ticks make one displayed
            // second, so a screenpack can run a fast or slow timer. A running
            // pause or super flash holds the clock with the rest of the world.
            const bool worldPaused = entities_.pauseInfo().ticks > 0;
            if (!worldPaused &&
                ++timerTicks_ >= std::max(1, game_.fight().time.framesPerCount)) {
                timerTicks_ = 0;
                if (timer_ > 0) --timer_;
            }

            if (engineTest_) {
                // The placeholder fighters have no AI and no moves, so the
                // attack buttons stand in for hitting: the left three hurt the
                // right fighter and vice versa. This is what makes the lifebars,
                // the KO banner and the win icons reachable on a device.
                if (input.wasPressed(Button::kA) || input.wasPressed(Button::kB) ||
                    input.wasPressed(Button::kC)) {
                    fighters_[1].takeHit(kTestDamage, 1);
                }
                if (input.wasPressed(Button::kX) || input.wasPressed(Button::kY) ||
                    input.wasPressed(Button::kZ)) {
                    fighters_[0].takeHit(kTestDamage, -1);
                }
            } else {
                // The view the machines will read against, before they step.
                entities_.setStageView(camera_, floorY_);
                // Player one on the pad; the other side is the CPU, which is
                // every mode for the moment because one shared pad drives one
                // fighter. A side with no character (a half-loaded roster) gets
                // no steering at all: it stands and takes hits.
                fighters_[0].control(input);
                if (fighters_[1].character() != nullptr) fighters_[1].controlAI(fighters_[0]);

                fighters_[0].step(fighters_[1]);
                fighters_[1].step(fighters_[0]);

                // ------------------------------------------------ helpers
                //
                // The helpers step after the roots, so a bindtoparent written this
                // tick follows the position the parent ended the tick in. Each one's
                // opponent is the other side's root, which is where its `p2`
                // triggers and its enemy redirects point. The count is snapshotted:
                // a helper that spawns a helper starts *that* one next tick, which
                // is MUGEN's reading of "created during this frame".
                const size_t helpersAtTickStart = helpers_.size();
                for (size_t i = 0; i < helpersAtTickStart; ++i) {
                    Fighter& helper = *helpers_[i];
                    const int side = helper.machine().side() == 0 ? 0 : 1;
                    if (!helper.machine().doomed()) {
                        // keyctrl helpers hear the pad their side's player holds;
                        // the rest hear nothing, which is what makes them run on
                        // their states alone.
                        InputState helperInput;
                        if (helper.machine().keyCtrl() && side == 0) helperInput = input;
                        helper.control(helperInput);
                        helper.step(fighters_[side == 0 ? 1 : 0]);
                    }
                }

                // Overlapping fighters shove each other apart before the hit
                // check, so a point-blank punch reads from the boxes rather
                // than from the fighters occupying the same pixels.
                Fighter::separate(fighters_[0], fighters_[1]);
                // `inguarddist`, refreshed every tick: whether the other fighter is
                // swinging from close enough that holding back would block it. It has
                // to be recomputed before the hit check rather than during it, because
                // it is what decides whether the *defender* was blocking at all.
                Fighter::updateGuardDistances(fighters_[0], fighters_[1], entities_.projectiles(),
                                              floorY_);

                // Both attacks are resolved in the same tick, and *which* one wins is
                // decided by the `priority` the two HitDefs declare rather than by the
                // order this loop happens to check them in. Without that, the first
                // side listed won every exchange in the game -- a coin that always
                // lands the same way. Equal priorities trade: both fighters get hit,
                // which is what makes a clash of two equally-good moves read as a
                // clash instead of as a winner.
                const bool p1Hits = fighters_[0].wouldHit(fighters_[1], floorY_);
                const bool p2Hits = fighters_[1].wouldHit(fighters_[0], floorY_);
                bool landed1 = false;
                bool landed2 = false;
                if (p1Hits && p2Hits) {
                    const int priority1 = fighters_[0].attackPriority();
                    const int priority2 = fighters_[1].attackPriority();
                    // The number decides unless the two are equal, and then the
                    // *class* does -- `priority = hit_prior, hit_type`, whose purpose
                    // the reference gives outright: "the priority class determines the
                    // tiebreaking behavior when P1 and P2 hit each other simultaneously
                    // with equal priorities". The six-row table is `clashLands`, asked
                    // of each side in turn.
                    //
                    // A pair that produces no hits leaves both HitDefs armed -- the
                    // reference is explicit that they "stay enabled" -- so a side that
                    // loses the tie is not sent to `tryHit` at all, which is also what
                    // keeps its armed hit for the next tick.
                    const StateMachine::PriorityClass class1 = fighters_[0].attackPriorityClass();
                    const StateMachine::PriorityClass class2 = fighters_[1].attackPriorityClass();
                    if (priority1 > priority2) {
                        fighters_[0].tryHit(fighters_[1], floorY_, &landed1);
                    } else if (priority2 > priority1) {
                        fighters_[1].tryHit(fighters_[0], floorY_, &landed2);
                    } else {
                        if (StateMachine::clashLands(class1, class2)) {
                            fighters_[0].tryHit(fighters_[1], floorY_, &landed1);
                        }
                        if (StateMachine::clashLands(class2, class1)) {
                            fighters_[1].tryHit(fighters_[0], floorY_, &landed2);
                        }
                    }
                } else if (p1Hits) {
                    fighters_[0].tryHit(fighters_[1], floorY_, &landed1);
                } else if (p2Hits) {
                    fighters_[1].tryHit(fighters_[0], floorY_, &landed2);
                }

                // The helpers fight too. A helper's HitDef meets the other side's
                // root first and its helpers second, which is the order MUGEN's own
                // player list walks in a 1v1 match. And a root whose swing missed
                // the other root may still catch a helper standing in it -- the
                // family on the other side is the target set, not just its root.
                for (auto& helper : helpers_) {
                    if (helper->machine().doomed() || !helper->machine().hasArmedHitDef()) {
                        continue;
                    }
                    const int side = helper->machine().side() == 0 ? 0 : 1;
                    const int enemySide = side == 0 ? 1 : 0;
                    bool helperLanded = false;
                    if (helper->tryHit(fighters_[enemySide], floorY_, &helperLanded)) {
                        if (helperLanded) (side == 0 ? landed1 : landed2) = true;
                        continue;
                    }
                    for (auto& victim : helpers_) {
                        if (victim->machine().side() != enemySide) continue;
                        if (helper->tryHit(*victim, floorY_, &helperLanded)) {
                            if (helperLanded) (side == 0 ? landed1 : landed2) = true;
                            break;
                        }
                    }
                }
                for (int side = 0; side < 2; ++side) {
                    bool& landed = side == 0 ? landed1 : landed2;
                    if (landed) continue;  // already connected with the other root
                    for (auto& victim : helpers_) {
                        if (victim->machine().doomed()) continue;
                        if (victim->machine().side() != (side == 0 ? 1 : 0)) continue;
                        if (fighters_[side].tryHit(*victim, floorY_, &landed)) break;
                    }
                }

                // ------------------------------- player attack vs projectile
                //
                // The reference's first collision pass (`0x46F3D0`, §4) runs
                // before the projectile-vs-player pass: an active attack's Clsn1
                // boxes can shoot down the *opponent's* flying projectile, using
                // that projectile's Clsn2 boxes. It is state 1 only -- once a
                // beam is in a removal state an attack passes through it.
                //
                // The attribute half is the armed HitDef's `affectteam` mask
                // against the projectile's side/role, with the low-two-bit swap
                // when the player's side field is 2 (see
                // `projectileAttackAttributeMatches`). An omitted `affectteam`
                // parses to `Enemy`; the role is the projectile's `ownerSide` in
                // MUGEN's 1/2 numbering.
                //
                // The attack is applied immediately, unlike a player contact a
                // projectile records (§3.2): `hits--` and either state 2
                // (`projremanim`) or `pause = pausetime`. It deliberately uses
                // the state-2 path, not the state-3 cancel of a projectile duel.
                //
                // Approximation: this pass runs after the melee resolution above,
                // and the engine's armed HitDef is a single shared record. A move
                // whose `hitcount` was spent on a fighter is therefore disarmed
                // and cannot also shoot a beam down on that tick. The reference's
                // passes are independent; keeping this engine's one-hitdef model
                // is the honest, narrower reading, and avoids hitting a player
                // and a projectile off the same spent attack.
                //
                // A projectile shot down here is state 2 before the
                // projectile-vs-projectile pass below, whose `isFlying()` gate
                // then refuses it for the rest of the tick -- no double hit.
                {
                    // Roots and helpers both count: a helper runs a player's
                    // states and its armed HitDef is a player attack in MUGEN's
                    // player list just as a root's is.
                    std::vector<Fighter*> attackers;
                    attackers.reserve(2 + helpers_.size());
                    for (int side = 0; side < 2; ++side) {
                        if (fighters_[side].machine().hasArmedHitDef()) {
                            attackers.push_back(&fighters_[side]);
                        }
                    }
                    for (auto& helper : helpers_) {
                        if (!helper->machine().doomed() &&
                            helper->machine().hasArmedHitDef()) {
                            attackers.push_back(helper.get());
                        }
                    }
                    for (Fighter* attacker : attackers) {
                        const int side = attacker->machine().side() == 0 ? 0 : 1;
                        // `pausetime`'s attacker component, the number the
                        // reference copies into the surviving projectile.
                        const StateMachine::HitDef& attack =
                            attacker->machine().armedHitDef();
                        const int attackPause = attack.pause[0];
                        const int attackAffectTeam = attack.affectTeam;
                        for (Projectile& beam : entities_.projectiles()) {
                            if (beam.gone) continue;
                            if (!projectileHittableByAttack(beam.state)) continue;
                            // §4 walks the opponent's manager only.
                            if (beam.ownerSide == side) continue;
                            if (!projectileAttackAttributeMatches(
                                    attackAffectTeam, mugenSideField(side),
                                    mugenProjectileRole(beam.ownerSide))) {
                                continue;
                            }
                            // The projectile's hurt boxes come from the same
                            // flight animation it is drawn with; `projscale`
                            // stays out of it (§7.2).
                            const Animation* animation =
                                beam.animFromFightFx
                                    ? game_.fightFxAnimations().find(beam.anim)
                                    : (beam.owner != nullptr
                                           ? beam.owner->animation(beam.anim)
                                           : nullptr);
                            if (animation == nullptr) continue;
                            const int frame = animation->frameAt(beam.clock);
                            if (frame < 0 ||
                                frame >= static_cast<int>(animation->hurtBoxes.size())) {
                                continue;
                            }
                            const std::vector<CollisionBox>& hurtBoxes =
                                animation->hurtBoxes[static_cast<size_t>(frame)];
                            if (hurtBoxes.empty()) continue;
                            bool touches = false;
                            for (const CollisionBox& hurt : hurtBoxes) {
                                if (attacker->attackOverlapsEntityBox(
                                        hurt, beam.facing, beam.x, beam.y, floorY_)) {
                                    touches = true;
                                    break;
                                }
                            }
                            if (!touches) continue;

                            // Contact bookkeeping. §4 does not document a code
                            // of its own; the only cancellation code the
                            // reference exhibits is 3, so an attack contact
                            // records that. Recorded even when the projectile
                            // survives the hit (it pauses rather than leaving),
                            // because the player-side triple records the
                            // contact, not the removal.
                            entities_.noteProjectileContact(
                                beam.ownerSide, beam.id, kProjectileContactCancelled);

                            const ProjectileAttackOutcome outcome =
                                applyPlayerAttackToProjectile(
                                    beam.hitsLeft, beam.removeOnHit, attackPause);
                            beam.hitsLeft = outcome.hitsLeft;
                            if (outcome.enterRemoval) {
                                entities_.removeProjectileByAttack(beam);
                            } else {
                                beam.pause = outcome.pause;
                            }
                        }
                    }
                }

                // ------------------------------------------------ the beams
                //
                // A projectile's boxes are its own animation's, drawn from where the
                // projectile is rather than from a fighter, and the reaction it gives is
                // the HitDef its owner had armed when it was fired. It connects with
                // whatever is on the *other* side, and it can connect several times if it
                // was written to.
                for (Projectile& beam : entities_.projectiles()) {
                    // States 1 and 2 are the ones a player can still run into: a
                    // projectile already playing its bound/age removal animation stays
                    // dangerous until its animation ends (§3.1). States 3 and 4 -- a
                    // cancellation and an exhausted removable hit -- never touch a
                    // player.
                    if (beam.gone ||
                        !(beam.isFlying() || beam.state == ProjectileState::Removal)) {
                        continue;
                    }
                    // Its own hitstop skips the pass, and so does the cooldown from the
                    // previous contact. `projhits <= 0` means it can never hit a player,
                    // which is the whole point of `projhits = 0` (§3.1).
                    if (beam.pause > 0 || beam.missTicks > 0 || beam.hitsLeft <= 0) continue;
                    // The boxes come from whatever animation is current: the flight
                    // animation while flying, and the removal one once state 2 has
                    // selected it. Its file follows the same `F` prefix rule everything
                    // else does, and its boxes come out of that file too: a projectile
                    // the screenpack draws is a projectile the screenpack has to give
                    // boxes to.
                    const Animation* animation =
                        beam.animFromFightFx
                            ? game_.fightFxAnimations().find(beam.anim)
                            : (beam.owner != nullptr ? beam.owner->animation(beam.anim)
                                                     : nullptr);
                    if (animation == nullptr) continue;
                    const int frame = animation->frameAt(beam.clock);
                    if (frame < 0 || frame >= static_cast<int>(animation->attackBoxes.size())) {
                        continue;
                    }
                    // The other side is the target: its root first, then its helpers,
                    // which stand in the same space and are just as hittable.
                    const int enemySide = beam.ownerSide == 0 ? 1 : 0;
                    Fighter* victim = nullptr;
                    Fighter* candidates[1] = {&fighters_[enemySide]};
                    // No `projscale`: collision is always against the raw position and
                    // the unscaled Clsn record (§7.2).
                    auto touches = [&](Fighter& f) {
                        for (const CollisionBox& box :
                             animation->attackBoxes[static_cast<size_t>(frame)]) {
                            if (f.overlapsEntityBox(box, beam.facing, beam.x, beam.y,
                                                    floorY_)) {
                                return true;
                            }
                        }
                        return false;
                    };
                    if (touches(*candidates[0])) victim = candidates[0];
                    if (victim == nullptr) {
                        for (auto& helper : helpers_) {
                            if (helper->machine().doomed() ||
                                helper->machine().side() != enemySide) {
                                continue;
                            }
                            if (touches(*helper)) {
                                victim = helper.get();
                                break;
                            }
                        }
                    }
                    if (victim == nullptr) continue;
                    // The same hitflag gate as a melee hit: a default beam passes
                    // through a fighter it has already knocked down.
                    if (!StateMachine::hitMayReach(beam.hit, victim->machine())) continue;

                    const StateMachine::HitInfo info = StateMachine::hitInfoFor(
                        beam.hit, beam.facing, victim->machine(), beam.owner);
                    if (!victim->machine().applyHit(info)) continue;
                    // The reference's player-side contact triple: code 1 for a
                    // hit, 2 for a guard. It is written at contact, not when
                    // the pending hit settles, so the trigger sees it on the
                    // next tick even if the projectile is about to be freed.
                    entities_.noteProjectileContact(
                        beam.ownerSide, beam.id,
                        info.guarded ? kProjectileContactGuarded
                                     : kProjectileContactHit);
                    if (!info.guarded) projectileLanded_[beam.ownerSide] = true;
                    // The contact is recorded, not applied (§3.2). `EntityLayer::update`
                    // runs later in this same tick and settles it at the start of the
                    // projectile's step, which is where the hit count, cooldown and pause
                    // change and where state 4 is entered. The timing helper encodes the
                    // table's one special case: the last hit of a removable projectile
                    // schedules neither a pause nor a cooldown.
                    const ProjectilePendingTiming timing = projectilePendingHitTiming(
                        beam.hitsLeft, beam.removeOnHit, beam.hit.pause[0], beam.missTime);
                    beam.pendingHit = true;
                    beam.pendingPause = timing.pause;
                    beam.pendingCooldown = timing.cooldown;
                    // The beam's own hitstop, on the fighter that fired it, so a beam that
                    // connects feels like a move that connected.
                    fighters_[beam.ownerSide].machine().setPause(info.attackerPause);
                }

                // ------------------------------------- projectile vs projectile
                //
                // Both projectiles must still be flying; this pass ignores
                // `projremove`, the re-hit cooldown, the projectile's own pause and
                // even `projhits` -- a projectile written with `projhits = 0` still
                // cancels another one (§2). The exchange is the exact three-comparison
                // rule from §2.4, and the loser enters state 3, not state 2.
                //
                // **Attribute gate approximation.** The reference tests attribute bits
                // as well as the projectile's role (§2.2), but this engine's collision
                // boxes carry no attribute mask and the role-bit semantics are
                // UNRESOLVED. The unambiguous half is kept: only projectiles owned by
                // opposite sides collide. Recorded in the parity notes rather than
                // guessed at.
                {
                    std::vector<Projectile>& beams = entities_.projectiles();
                    // The current animation's attack boxes, or null when the projectile
                    // is not flying or has no boxed frame this tick.
                    auto attackBoxesOf = [&](const Projectile& p)
                        -> const std::vector<CollisionBox>* {
                        if (p.gone || !p.isFlying()) return nullptr;
                        const Animation* animation =
                            p.animFromFightFx
                                ? game_.fightFxAnimations().find(p.anim)
                                : (p.owner != nullptr ? p.owner->animation(p.anim)
                                                      : nullptr);
                        if (animation == nullptr) return nullptr;
                        const int frame = animation->frameAt(p.clock);
                        if (frame < 0 ||
                            frame >= static_cast<int>(animation->attackBoxes.size())) {
                            return nullptr;
                        }
                        return &animation->attackBoxes[static_cast<size_t>(frame)];
                    };
                    for (size_t i = 0; i < beams.size(); ++i) {
                        Projectile& a = beams[i];
                        if (a.gone || !a.isFlying()) continue;
                        const std::vector<CollisionBox>* boxesA = attackBoxesOf(a);
                        if (boxesA == nullptr || boxesA->empty()) continue;
                        for (size_t j = i + 1; j < beams.size(); ++j) {
                            Projectile& b = beams[j];
                            if (b.gone || !b.isFlying()) continue;
                            if (a.ownerSide == b.ownerSide) continue;
                            const std::vector<CollisionBox>* boxesB = attackBoxesOf(b);
                            if (boxesB == nullptr || boxesB->empty()) continue;
                            bool touches = false;
                            for (const CollisionBox& boxA : *boxesA) {
                                const ProjectileRect worldA = projectileBoxToWorld(
                                    boxA.x1, boxA.y1, boxA.x2, boxA.y2, a.facing, a.x, a.y,
                                    floorY_);
                                for (const CollisionBox& boxB : *boxesB) {
                                    const ProjectileRect worldB = projectileBoxToWorld(
                                        boxB.x1, boxB.y1, boxB.x2, boxB.y2, b.facing, b.x,
                                        b.y, floorY_);
                                    if (projectileRectsOverlap(worldA, worldB)) {
                                        touches = true;
                                        break;
                                    }
                                }
                                if (touches) break;
                            }
                            if (!touches) continue;

                            const ProjectileDuelOutcome outcome = resolveProjectileDuel(
                                a.hitsLeft, a.priority, b.hitsLeft, b.priority);
                            a.hitsLeft = outcome.aHits;
                            a.priority = outcome.aPriority;
                            b.hitsLeft = outcome.bHits;
                            b.priority = outcome.bPriority;
                            // §2.5: a survivor pauses for its own HitDef's pausetime;
                            // an exhausted one is cancelled regardless of `projremove`
                            // -- and `projhits = 0` cancels here just like a spent hit.
                            // A cancellation is recorded on the owner of the
                            // cancelled projectile (`+0x260 = 3`, §2.5). That
                            // side's `ProjCancel`-family state survives the
                            // slot, which is why the record is taken before
                            // `cancelProjectile` may free it.
                            if (outcome.aCancelled) {
                                entities_.noteProjectileContact(
                                    a.ownerSide, a.id, kProjectileContactCancelled);
                                entities_.cancelProjectile(a);
                            } else {
                                a.pause = a.hit.pause[0];
                            }
                            if (outcome.bCancelled) {
                                entities_.noteProjectileContact(
                                    b.ownerSide, b.id, kProjectileContactCancelled);
                                entities_.cancelProjectile(b);
                            } else {
                                b.pause = b.hit.pause[0];
                            }
                            if (!a.isFlying()) break;
                        }
                    }
                }

                // The dead go: a helper whose DestroySelf ran this tick is swept
                // before the count triggers read the family.
                sweepHelpers();

                // The entities themselves: explods animate and drift, projectiles travel,
                // and the shake decays. Stepped after the fighters so that an aura spawned
                // this tick is drawn at the position the fighter ended the tick in.
                entities_.update(&fighters_[0].machine(), &fighters_[1].machine());

                // ------------------------------------------- the gauges' bookkeeping
                //
                // A hit that lands gives the *other* side a combo to count, and the
                // damage it did is read off the victim's own record of the last hit.
                // A block counts as a hit for the counter but not as damage, which is
                // what makes a blocked string read as "0 dmg." rather than as nothing.
                const bool landed[2] = {landed1 || projectileLanded_[0],
                                        landed2 || projectileLanded_[1]};
                projectileLanded_[0] = projectileLanded_[1] = false;
                for (int side = 0; side < 2; ++side) {
                    if (!landed[side]) continue;
                    const Fighter& victim = fighters_[side == 0 ? 1 : 0];
                    ++comboHits_[side];
                    comboDamage_[side] += victim.machine().lastHit().damage;
                    comboTicks_[side] = 90;
                }
                if (!landed1 && !landed2) {
                    for (int side = 0; side < 2; ++side) {
                        if (comboTicks_[side] > 0) --comboTicks_[side];
                    }
                }

                // The health bar's two extra quantities. Losing life puts it into the
                // yellow trail, which is eaten away over the next second; the part of
                // it that was taken *behind a guard* is also recoverable, and drains
                // back into the bar if the fighter is not hit again before it goes.
                for (int side = 0; side < 2; ++side) {
                    const int lifeNow = fighters_[side].life();
                    if (lifeNow < lastLife_[side]) {
                        const int lost = lastLife_[side] - lifeNow;
                        lifeTrail_[side] += lost;
                        if (fighters_[side].machine().lastHit().guarded) recoverable_[side] += lost;
                    }
                    lastLife_[side] = lifeNow;

                    // The trail drains at about a hundredth of a life bar a tick, so a
                    // big hit takes about a second and a half to settle.
                    const int trailStep = std::max(1, fighters_[side].maxLife() / 100);
                    lifeTrail_[side] = std::max(0, lifeTrail_[side] - trailStep);

                    // And what is recoverable comes back once nothing has landed for a
                    // moment -- the delay is what stops a fighter healing mid-combo.
                    if (recoverable_[side] > 0 && comboTicks_[side == 0 ? 1 : 0] <= 0) {
                        const int heal = std::min(recoverable_[side], std::max(1, fighters_[side].maxLife() / 200));
                        fighters_[side].heal(heal);
                        recoverable_[side] -= heal;
                        lastLife_[side] = fighters_[side].life();
                    }
                }
            }

            if (fighters_[0].life() <= 0 || fighters_[1].life() <= 0) {
                const bool doubleKo = fighters_[0].life() <= 0 && fighters_[1].life() <= 0;
                endRound(fighters_[0].life() <= 0 ? (fighters_[1].life() <= 0 ? -1 : 1) : 0);
                phase_ = Phase::KnockOut;
                // fight.def's KO.snd is 2,0 and DKO.snd is 2,1.
                if (engine() != nullptr) {
                    engine()->mixer().playFrom(game_.fightSounds(), 2, doubleKo ? 1 : 0);
                }
                phaseTicks_ = 0;
            } else if (timer_ <= 0) {
                const float p1 = static_cast<float>(fighters_[0].life()) /
                                 static_cast<float>(std::max(1, fighters_[0].maxLife()));
                const float p2 = static_cast<float>(fighters_[1].life()) /
                                 static_cast<float>(std::max(1, fighters_[1].maxLife()));
                endRound(p1 == p2 ? -1 : (p1 > p2 ? 0 : 1));
                phase_ = Phase::TimeOver;
                // fight.def's TO.snd is 2,2.
                if (engine() != nullptr) {
                    engine()->mixer().playFrom(game_.fightSounds(), 2, 2);
                }
                phaseTicks_ = 0;
            }
            break;
        }

        case Phase::KnockOut:
            // The loser keeps stepping through knockdown while the banner
            // shows, so the fight visibly settles rather than freezing.
            fighters_[0].step(fighters_[1]);
            fighters_[1].step(fighters_[0]);
            if (phaseTicks_ >= std::max(1, round.ko.displaytime)) {
                phase_ = winner_ < 0 ? Phase::DrawGame : Phase::MatchOver;
                phaseTicks_ = 0;
            }
            break;

        case Phase::TimeOver:
            if (phaseTicks_ >= std::max(1, round.timeOver.displaytime)) {
                phase_ = winner_ < 0 ? Phase::DrawGame : Phase::MatchOver;
                phaseTicks_ = 0;
            }
            break;

        case Phase::MatchOver:
            if (phaseTicks_ >= std::max(1, round.winTime)) {
                if (matchDecided_) {
                    advanceOrLeave();
                    return;
                }
                ++round_;
                startRound();
            }
            break;

        case Phase::Victory:
            // The card owns the screen while this phase is set, so there is no
            // fight simulation to advance underneath it. The pop handler at the
            // top of update() is what moves on from here.
            break;

        case Phase::Win:
            // Likewise the run-cleared card: there is nothing to simulate under
            // it, and the pop handler above is the only way out.
            break;

        case Phase::GameOver:
        case Phase::DefaultEnding:
        case Phase::EndCredits:
            // The storyboard owns the screen while its phase is set, so the
            // fight underneath must not tick. The pop handler above is the only
            // way out.
            break;

        case Phase::Continue:
            // The prompt owns the screen while this phase is set, so the fight
            // underneath must not tick. The pop handler above is the only way
            // out.
            break;

        case Phase::DrawGame:
            if (phaseTicks_ >= std::max(1, round.win.displaytime)) {
                if (engine() != nullptr) engine()->popScreen();
                return;
            }
            break;

        case Phase::ArcadeClear:
            if (phaseTicks_ >= kArcadeClearTicks) {
                if (engine() != nullptr) engine()->popScreen();
                return;
            }
            break;
    }

    // Camera: between the fighters, clamped to the stage's ends. It stops for
    // the round's end banners so the frame holds still for the finish.
    if (phase_ == Phase::Fight) {
        const float midpoint = (fighters_[0].x() + fighters_[1].x()) * 0.5f;
        const auto [low, high] = cameraBounds();
        camera_ = std::clamp(midpoint, low, high);
    }

    // Start always abandons the match and goes back, so the test can never trap
    // the player on this screen.
    if (input.wasPressed(Button::kStart) && engine() != nullptr) {
        engine()->popScreen();
    }
}

// ---------------------------------------------------------------------- stage

void FightScreen::advanceOrLeave() {
    // Side 0 is always the player, in every mode: the pad is wired to it and
    // the CPU drives side 1.
    const bool playerWon = roundsWon_[0] > roundsWon_[1];

    // The victory card is an arcade screen: it is shown between ladder matches
    // and before the end-of-run hold. An exhibition or engine-test fight has no
    // ladder, no next opponent and no arcade ending to introduce, so it exits
    // directly as it always has. A lost match never shows a card: every element
    // on it describes the winner, and side 0 is the only side this engine could
    // fill in honestly.
    const bool showVictory = playerWon && !ladder_.opponents.empty() &&
                             game_.system().victory.enabled && engine() != nullptr;

    if (showVictory) {
        // The ladder move waits until the card pops; see `AfterVictory` and the
        // handler at the top of update(). Pushing here is safe because the
        // engine defers stack changes to the end of the tick, exactly as the VS
        // card does.
        afterVictory_ = ladderStep_ + 1 < ladder_.opponents.size()
                            ? AfterVictory::NextOpponent
                            : AfterVictory::ArcadeClear;
        phase_ = Phase::Victory;
        phaseTicks_ = 0;
        engine()->pushScreen(std::make_unique<VictoryScreenView>(
            game_, characters_[winner_], &fighters_[winner_], matchNo()));
        return;
    }

    if (!ladder_.opponents.empty()) {
        if (playerWon && ladderStep_ + 1 < ladder_.opponents.size()) {
            ++ladderStep_;
            AF_LOGI("arcade: match won, opponent %zu of %zu", ladderStep_ + 1,
                    ladder_.opponents.size());
            setupMatch();
            return;
        }
        if (playerWon) {
            AF_LOGI("arcade: cleared the ladder after %zu opponent(s)", ladder_.opponents.size());
            finishArcadeRun();
            return;
        }
        AF_LOGI("arcade: run over after %zu of %zu -- lost", ladderStep_ + 1,
                ladder_.opponents.size());

        // A lost arcade match offers the continue prompt, if the screenpack
        // enabled it. `continueOffered_` is belt-and-braces: the phase guard
        // already stops a second push, but this also keeps a stray call on the
        // decided-match path from reopening a prompt that was already answered.
        if (game_.system().continueScreen.enabled && engine() != nullptr && !continueOffered_) {
            continueOffered_ = true;
            // A prompt that never writes a result (an unexpected teardown) must
            // end the run rather than resume it.
            continueResult_ = ContinueResult::Declined;
            phase_ = Phase::Continue;
            phaseTicks_ = 0;
            engine()->pushScreen(std::make_unique<ContinueScreenView>(game_, &continueResult_));
            return;
        }

        endArcadeRun();
        return;
    }

    // An exhibition or engine-test loss keeps its old exit exactly: there is no
    // ladder and no arcade run to end, so the fight screen simply pops.
    if (engine() != nullptr) engine()->popScreen();
}

void FightScreen::endArcadeRun() {
    // SCREEN-FLOW-RE §2.7 and §3.2 steps 5b/6b: a declined or expired continue
    // reaches the Game Over gate, and the gate is a storyboard switch. When the
    // screenpack enables it and the storyboard loads, the Game Over scenes own
    // the screen and this fight waits underneath until they pop. Every failure
    // path keeps the old log-and-return behaviour: an enabled switch with a
    // missing or unplayable file must not trap the player on a black screen.
    const StoryboardSwitch& gameOver = game_.system().gameOver;
    if (gameOver.enabled) {
        if (gameOver.storyboard.empty()) {
            AF_LOGW("game over: section enabled with no storyboard -- returning to title");
        } else if (pushStoryboard(gameOver, Phase::GameOver)) {
            return;
        }
    } else {
        AF_LOGI("arcade: run over -- returning to title");
    }
    returnToTitle();
}

bool FightScreen::pushStoryboard(const StoryboardSwitch& sw, Phase phase) {
    if (engine() == nullptr) return false;
    // The loader is eager so this can answer before anything is pushed. A
    // storyboard that reports not-ready has already logged why.
    auto screen = std::make_unique<StoryboardScreen>(engine()->files(), game_, sw.storyboard);
    if (!screen->ready()) {
        AF_LOGW("storyboard \"%s\" unavailable: %s", sw.storyboard.c_str(),
                screen->error().c_str());
        return false;
    }
    phase_ = phase;
    phaseTicks_ = 0;
    engine()->pushScreen(std::move(screen));
    return true;
}

void FightScreen::advanceEndSequence(bool allowDefaultEnding) {
    // `[Default Ending]` is the fallback for a character with no ending of its
    // own. This engine has no per-character ending records at all, so every
    // character is in that case and the switch alone decides. It is shown at
    // most once: the DefaultEnding phase re-enters here with `false`.
    if (allowDefaultEnding) {
        const StoryboardSwitch& ending = game_.system().defaultEnding;
        if (ending.enabled) {
            if (ending.storyboard.empty()) {
                AF_LOGW("default ending: section enabled with no storyboard -- skipping");
            } else if (pushStoryboard(ending, Phase::DefaultEnding)) {
                return;
            }
        }
    }

    // `[End Credits]` comes after the ending, or directly after the Win card
    // when no ending is shown.
    const StoryboardSwitch& credits = game_.system().endCredits;
    if (credits.enabled) {
        if (credits.storyboard.empty()) {
            AF_LOGW("end credits: section enabled with no storyboard -- skipping");
        } else if (pushStoryboard(credits, Phase::EndCredits)) {
            return;
        }
    }

    // Nothing left to play. The existing end-of-run banner is the last thing
    // before the screen pops.
    phase_ = Phase::ArcadeClear;
    phaseTicks_ = 0;
}

void FightScreen::restartMatch() {
    // Accepted continue: the same ladder step, so `matchno` does not move, and
    // the same opponent. `setupMatch(false)` is the smallest honest reuse --
    // it already resets the round count, the decided flag and the winner, and
    // rebuilds both fighters and the stage; the only thing a restart must not
    // do is replay the VS card, which is exactly what the flag suppresses.
    //
    // Do not copy that body here: every reset it performs exists because a new
    // match needs it, and a second copy would drift the first time either one
    // changes. Nothing in it reads ladderStep_, so not advancing the step is
    // all "same match" requires.
    AF_LOGI("arcade: continue accepted, restarting match %d", matchNo());
    setupMatch(false);
}

void FightScreen::returnToTitle() {
    // This engine's stack is title -> select -> fight for an arcade run, and the
    // Engine has no "pop to root" verb, so returning to the title is two pops
    // issued together. The engine defers both to the end of the tick and pops
    // the top each time, which lands on the title screen. A stack without the
    // select screen (should one ever grow another route here) would pop one
    // screen too many; arcade is the only caller today and the only route that
    // reaches this path.
    if (engine() == nullptr) return;
    engine()->popScreen();  // the fight
    engine()->popScreen();  // the select screen underneath it
}

void FightScreen::finishArcadeRun() {
    // `[Win Screen]` is once per cleared run and is a separate record from
    // `[Victory Screen]` (SCREEN-FLOW-RE §2.5, §3.2). If the screenpack enabled
    // it, the card is pushed now and `Phase::Win` holds the end-of-run hold
    // back until it pops; otherwise the run ends exactly as it did before this
    // screen existed.
    if (game_.system().win.enabled && engine() != nullptr) {
        phase_ = Phase::Win;
        phaseTicks_ = 0;
        engine()->pushScreen(
            std::make_unique<WinScreenView>(game_, characters_[winner_], &fighters_[winner_]));
        return;
    }

    // No Win card. The run can still continue into its ending and credits; the
    // Win phase's pop handler runs the same chain when the card was shown.
    advanceEndSequence(true);
}

std::pair<float, float> FightScreen::cameraBounds() const {
    // A stage writes its bounds relative to the middle of the arena, because
    // that is where MUGEN's camera origin is. This screen's coordinates put 0
    // at the left edge of the playfield instead, so the offset is the half
    // width -- a conversion kept in one place rather than spread over every
    // use of the camera.
    const float middle = kStageOriginX;
    if (stage_ == nullptr) return {kCameraMin, kCameraMax};

    const StageDef& def = stage_->def();
    float low = middle + def.boundLeft * stageScale_.x;
    float high = middle + def.boundRight * stageScale_.x;
    // A stage that names no bounds, or names them backwards, would otherwise
    // weld the camera in place or let it wander off the art.
    if (!(high > low)) return {middle - 80.0f, middle + 80.0f};
    return {low, high};
}

void FightScreen::drawStage(Renderer& renderer, float scrollX) {
    if (stage_ != nullptr) {
        // The stage's own layers, scrolled by the camera. `start` in a stage .def
        // is measured from the top centre of the screen, so the placement maps
        // the stage's top centre onto the middle of the fight window and scales
        // its coordinate space into this window's units. `camera` is already in
        // screen pixels, which is what `delta` -- a number of background pixels
        // per camera pixel -- is written against.
        const StageDef& def = stage_->def();
        BackgroundPlacement placement;
        placement.origin = Vec2{renderer.virtualWidth() * 0.5f, 0.0f};
        placement.anchor = Vec2{def.localCoord.x * 0.5f, 0.0f};
        placement.scale = stageScale_;
        drawBackground(renderer, def.background, stage_->sprites(), {}, tick_,
                       Vec2{scrollX - kStageOriginX, 0.0f}, placement);
        return;
    }

    const float width = renderer.virtualWidth();
    const float height = renderer.virtualHeight();

    // There is no stage file yet, so the arena is drawn procedurally. The sky is
    // a two-stop gradient made of bands, and the floor is a lit strip with a
    // horizon line -- enough to read the camera and the fighters' footing, which
    // is all this stage is for.
    constexpr int kBands = 24;
    for (int i = 0; i < kBands; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kBands - 1);
        const float y = (floorY_ + 10.0f) * static_cast<float>(i) / static_cast<float>(kBands);
        const float h = (floorY_ + 10.0f) / static_cast<float>(kBands) + 1.0f;
        renderer.fillRect(0.0f, y, width, h,
                          Color{kSkyTop.r + (kSkyBottom.r - kSkyTop.r) * t,
                                kSkyTop.g + (kSkyBottom.g - kSkyTop.g) * t,
                                kSkyTop.b + (kSkyBottom.b - kSkyTop.b) * t, 1.0f});
    }

    const float horizonTop = floorY_ - 10.0f;
    renderer.fillRect(0.0f, horizonTop, width, height - horizonTop, kGroundTop);
    renderer.fillRect(0.0f, horizonTop, width, 1.5f, Color::rgba8(120, 140, 190));
    renderer.fillRect(0.0f, floorY_, width, height - floorY_, kGroundBottom);

    // Perspective lines on the floor, scrolling with the camera, so it is
    // obvious when the camera moves and when it does not. They are spaced in
    // *stage* coordinates, so the offset includes the camera's own shift.
    const float spacing = 32.0f;
    const float originX = width * 0.5f - scrollX;
    const float offset = std::fmod(originX, spacing);
    for (float x = offset - spacing; x < width + spacing; x += spacing) {
        renderer.fillRect(x, floorY_, 1.0f, height - floorY_, Color{1.0f, 1.0f, 1.0f, 0.06f});
    }

    if (engineTest_ || stage_ == nullptr) {
        if (Font* font = game_.fontTable(true).get(1)) {
            // This is a caption on the stage, and the stage is drawn before the
            // HUD, so it has to clear both bands that draw over it: the HUD's
            // lifebars, powerbars and counters at the top, and the on-screen pad
            // at the bottom. What is left is the strip just under the HUD.
            font->draw(renderer, 4.0f, 50.0f, "NO STAGE FILE - TEST ARENA", 0, TextAlign::Left,
                       Color::rgba8(150, 160, 180));
        }
    }
}

bool FightScreen::anyGlobalNoShadow() const {
    for (const Fighter& fighter : fighters_) {
        if (fighter.machine().asserted(StateMachine::kAssertGlobalNoShadow)) return true;
    }
    for (const auto& helper : helpers_) {
        if (helper->machine().asserted(StateMachine::kAssertGlobalNoShadow)) return true;
    }
    return false;
}

void FightScreen::drawFighters(Renderer& renderer, float scrollX, bool drawShadows) {
    // Stage coordinates to screen coordinates: the camera's position becomes the
    // centre of the view.
    const float originX = renderer.virtualWidth() * 0.5f - scrollX;

    // Helpers with a negative sprite priority go under the fighters, the way an
    // explod with one goes under its owner: a portal on the floor is a portal on
    // the floor, not a thing standing in front of it.
    auto drawHelpers = [&](bool behind) {
        for (const auto& helper : helpers_) {
            if (helper->machine().doomed()) continue;
            const bool helperBehind = helper->machine().spritePriority() < 0;
            if (helperBehind != behind) continue;
            helper->draw(renderer, originX, floorY_, stageShadow_, drawShadows);
        }
    };
    drawHelpers(true);

    for (int i = 0; i < 2; ++i) {
        const Fighter& fighter = fighters_[i];
        const bool knockedOut = fighter.phase() == Fighter::Phase::KO;
        // A KO'd character stays on the floor in its knockdown art.
        if (knockedOut && fighter.character() == nullptr) continue;

        if (fighter.character() != nullptr) {
            // Draws its own shadow and its current animation frame.
            fighter.draw(renderer, originX, floorY_, stageShadow_, drawShadows);
            continue;
        }
        if (fighter.life() <= 0 && phase_ != Phase::KnockOut) continue;

        const float screenX = fighter.x() + originX;
        // The shadow goes down first, so the box stands on the floor rather
        // than floating in front of it. This is the stand-in body used when a
        // side has no character loaded; it obeys the same global gates.
        if (drawShadows) {
            renderer.fillRect(screenX - kBoxWidth * 0.7f, floorY_ - 2.0f,
                              kBoxWidth * 1.4f, 4.0f, kShadow);
        }

        const float left = screenX - kBoxWidth * 0.5f;
        const float top = floorY_ - kBoxHeight;
        renderer.fillRect(left, top, kBoxWidth, kBoxHeight, fighter.bodyColor());
        renderer.fillRect(left, top, kBoxWidth, 8.0f, fighter.trimColor());
        renderer.fillRect(left + 4.0f, top + 16.0f, kBoxWidth - 8.0f, 3.0f, fighter.trimColor());
    }

    // The rest of the helpers over the fighters, which is where a summoned double
    // or a beam core wants to be seen.
    drawHelpers(false);
}

// ------------------------------------------------------------------------ HUD

// The screenpack's own lifebars and powerbars, as fight.def describes them.
//
// **No longer called.** The fight screen draws Shin Budokai 2's gauges instead, and
// those are laid out in code rather than read from `fight.def` -- which is why the
// `[LifeBar]` and `[PowerBar]` sections of a screenpack now have no effect. This is
// kept rather than deleted because it is the MUGEN-compatible half of the HUD and
// nothing in the repository's history has it: it is here for a screenpack that wants
// its own bars back.
void FightScreen::drawBars(Renderer& renderer, const BarSide& p1, const BarSide& p2, int valueP1,
                           int valueP2, int maxP1, int maxP2, bool showCounters) {
    const FightDef& fight = game_.fight();
    SpriteBank& sprites = game_.fightSprites();
    const FontTable fonts = game_.fontTable(true);

    struct Side {
        const BarSide* bar;
        int value;
        int max;
    };
    const Side sides[2] = {{&p1, valueP1, maxP1}, {&p2, valueP2, maxP2}};

    for (const Side& side : sides) {
        const BarSide& bar = *side.bar;
        if (!bar.present) continue;

        const float x = bar.pos.x;
        const float y = bar.pos.y;
        const float fraction =
            static_cast<float>(side.value) / static_cast<float>(std::max(1, side.max));
        const float span = bar.rangeX.y;
        const int facing = barFacing(bar);

        // MUGEN's layer roles, which are not interchangeable. bg0 and bg1 are
        // the backing -- the channel you see once the bar has emptied -- and are
        // drawn whole. `front` is the bar itself, and is the layer that drains.
        drawElement(renderer, bar.bg0, sprites, fight.animations, fonts, tick_, x, y);
        drawElement(renderer, bar.bg1, sprites, fight.animations, fonts, tick_, x, y);

        if (bar.front.sprite) {
            drawSpriteFraction(renderer, sprites, bar.front.sprite->first, bar.front.sprite->second,
                               x, y, span, fraction, facing, bar.front.scale.x, bar.front.scale.y,
                               Color{}, bar.front.trans);
        } else {
            // An animated fill would need the life rect as a scissor rectangle.
            // The screenpack this ships with uses a plain sprite, and a .spr is
            // what every bar in the wild uses, so the anim case is left out
            // rather than drawn wrongly.
            drawElement(renderer, bar.front, sprites, fight.animations, fonts, tick_, x, y);
        }

        if (showCounters && bar.counter.present) {
            char text[16];
            std::snprintf(text, sizeof(text), "%d", side.value);
            drawElement(renderer, bar.counter, sprites, fight.animations, fonts, tick_, x, y, text);
        }
    }
}

// The between-round lettering, in Shin Budokai 2's voice.
//
// That game announces a round with a big italic line across the middle of the screen --
// "PERFECT!!" for an untouched round, "YOU WIN!!" for a won one, "K.O." at the end -- and
// nothing else: no bars, no markers, no counter. The screenpack's own banner elements are
// therefore not used here either, for the same reason the win icons are not.
//
// The screenpack still gets to say *when* something happened -- its phase timings are
// what decide the pacing -- but the words are the game's.
void FightScreen::drawBanners(Renderer& renderer) {
    Font* font = game_.fontTable(true).get(1);
    if (font == nullptr) return;

    const float middleX = renderer.virtualWidth() * 0.5f;
    const float middleY = renderer.virtualHeight() * 0.40f;

    const char* text = nullptr;
    Color colour = Color::rgba8(255, 255, 255);
    switch (phase_) {
        case Phase::RoundIntro:
            text = "ROUND";
            colour = Color::rgba8(230, 225, 255);
            break;
        case Phase::FightIntro:
            text = "FIGHT!";
            colour = Color::rgba8(255, 240, 90);
            break;
        case Phase::Fight:
            return;
        case Phase::Victory:
            // The victory card is a screen of its own and covers this one; when
            // it pops this phase has already moved on, so there is no banner.
            return;
        case Phase::Win:
            // Same for the run-cleared card: it is a screen of its own.
            return;
        case Phase::Continue:
            // And the continue prompt: it covers this screen entirely.
            return;
        case Phase::GameOver:
        case Phase::DefaultEnding:
        case Phase::EndCredits:
            // The storyboard is a screen of its own and covers this one.
            return;
        case Phase::KnockOut:
            text = "K.O.";
            colour = Color::rgba8(230, 40, 40);
            break;
        case Phase::TimeOver:
            text = "TIME UP";
            colour = Color::rgba8(230, 225, 255);
            break;
        case Phase::MatchOver:
            if (winner_ < 0) {
                text = "DRAW GAME";
                colour = Color::rgba8(200, 200, 235);
            } else if (winMethod_ == "perfect") {
                // The one word the game is most emphatic about, and the reason the
                // method is worth tracking at all.
                text = "PERFECT!!";
                colour = Color::rgba8(235, 60, 60);
            } else {
                text = "YOU WIN!!";
                colour = Color::rgba8(255, 200, 60);
            }
            break;
        case Phase::DrawGame:
            text = "DRAW GAME";
            colour = Color::rgba8(200, 200, 235);
            break;
        case Phase::ArcadeClear:
            text = "ARCADE CLEAR";
            colour = Color::rgba8(255, 220, 90);
            break;
    }
    if (text == nullptr) return;

    // A dark band behind the words, so they read over any stage. Comic lettering sits on
    // its own field in the game being copied; this is the same idea in one rectangle.
    const float bandHeight = 30.0f;
    renderer.fillRect(0.0f, middleY - bandHeight * 0.5f, renderer.virtualWidth(), bandHeight,
                      Color::rgba8(10, 10, 20, 190));

    char line[64];
    if (phase_ == Phase::RoundIntro) {
        std::snprintf(line, sizeof(line), "%s %d", text, round_);
    } else {
        std::snprintf(line, sizeof(line), "%s", text);
    }
    font->draw(renderer, middleX, middleY - 8.0f, line, 0, TextAlign::Center, colour, 1.8f);

    if (phase_ == Phase::MatchOver && winner_ >= 0) {
        font->draw(renderer, middleX, middleY + 14.0f, fighters_[winner_].name().c_str(), 0,
                   TextAlign::Center, Color::rgba8(255, 255, 255), 1.1f);
    }
}

// ------------------------------------------------------------------------ HUD
//
// The fight screen's gauges, laid out the way Shin Budokai 2 lays its out.
//
// Every number below comes from measurements taken off that game's own promo
// screenshots, scaled from the PSP's 480x272 to this screenpack's 320x240 -- so a
// position here is where the element actually is in the game being copied, not an
// arrangement in that style. The shape of it is one mirrored cluster per fighter in
// the top corner, occupying only the top fifth of the screen:
//
//   health plate     a slanted plate carrying a row of seven small squares and the
//                    health bar under them. The squares are how many *health bars*
//                    are left; the bar itself is one continuous run that is eaten
//                    from the centre end while its outer end stays put.
//   ki gauge         a smaller plate at the outer corner of the cluster holding
//                    seven diagonal slots and the count of how many are filled.
//   portrait, name   the character's own face art and name, between the two.
//   guard bar        a thin bar under the cluster: white outline, teal fill, draining
//                    from the centre end as blocks are made.
//   timer            a capsule at the top centre, between the two plates.
//
// Two things MUGEN has no equivalent of are drawn because this bar is drawn against
// them, and both are real quantities rather than decoration: the yellow trail of
// damage just taken, eaten away over the next second, and the red part of it that a
// fighter who blocked can still recover. And there are deliberately **no round-win
// markers**: Shin Budokai has none anywhere on its HUD, so adding pips would be
// inventing rather than copying.
namespace {

// The HUD's own palette, sampled from the same screenshots.
constexpr Color kPlateFill = Color::rgba8(30, 34, 108, 205);
constexpr Color kPlateEdge = Color::rgba8(130, 220, 220);
constexpr Color kLifeGreen = Color::rgba8(0, 211, 33);
constexpr Color kLifeEdge = Color::rgba8(17, 186, 43);
constexpr Color kLifeTrail = Color::rgba8(255, 240, 60);
constexpr Color kLifeRed = Color::rgba8(230, 40, 40);
constexpr Color kLifeSlot = Color::rgba8(10, 34, 16);
constexpr Color kSquareLit = Color::rgba8(44, 208, 49);
constexpr Color kSquareDim = Color::rgba8(58, 68, 32);
constexpr Color kKiFull = Color::rgba8(255, 240, 40);
constexpr Color kKiMax = Color::rgba8(190, 190, 250);
constexpr Color kKiEmpty = Color::rgba8(8, 8, 14);
constexpr Color kGuardFill = Color::rgba8(100, 200, 215);
constexpr Color kTimerRim = Color::rgba8(224, 44, 44);
constexpr Color kInk = Color::rgba8(255, 255, 255);
constexpr Color kMuted = Color::rgba8(200, 200, 235);

// How far a plate's vertical edge leans toward the middle of the screen, and how many
// health bars a character is taken to have. Seven squares is what the game draws;
// three lit is its own default for a three-bar fighter, which is what most of this
// roster's 3000-life characters are.
constexpr float kHudSlant = 3.0f;
constexpr int kHealthBarsMax = 7;
constexpr int kKiSlots = 7;

// The health bar's fill, drawn from the outer end inward with the yellow trail and the
// red recoverable sliver between the true life and the empty slot.
void drawLifeBar(Renderer& renderer, float outerX, float innerX, float y, float height,
                 int life, int maxLife, int trail, int recoverable, bool mirrored) {
    const float span = std::fabs(innerX - outerX);
    const float full = static_cast<float>(std::max(1, maxLife));
    const float permanent = std::clamp(static_cast<float>(life - recoverable) / full, 0.0f, 1.0f);
    const float red = std::clamp(static_cast<float>(recoverable) / full, 0.0f, 1.0f - permanent);
    const float yellow = std::clamp(static_cast<float>(trail) / full, 0.0f, 1.0f - permanent - red);

    // Each band is laid end to end from the outer edge, so the bar always reads as one
    // run of colour with the damage at the centre-facing end.
    float at = outerX;
    const float direction = mirrored ? -1.0f : 1.0f;
    auto band = [&](float fraction, Color color) {
        if (fraction <= 0.0f) return;
        const float width = span * fraction;
        renderer.fillShearedBar(mirrored ? at - width : at, y, width, height,
                                direction * kHudSlant, color);
        at += direction * width;
    };
    band(permanent, kLifeGreen);
    band(red, kLifeRed);
    band(yellow, kLifeTrail);
}

}  // namespace

void FightScreen::drawHud(Renderer& renderer) {
    const FontTable fonts = game_.fontTable(true);
    Font* font = fonts.get(1);
    const float width = renderer.virtualWidth();

    // Every horizontal position below is a fraction of the *window*, and every vertical
    // one is a coordinate in it. That split is what makes the HUD responsive: the fight's
    // window is given the device's own aspect (see enter()), so its width varies from
    // phone to phone while its height is always 240, and a gauge pinned to a percentage
    // of the width sits at the screen's edge on all of them. The fractions are the ones
    // measured off Shin Budokai 2's screenshots, taken as proportions of its 480-pixel
    // screen.
    const float plateOuter = 0.160f * width;
    const float plateInner = 0.473f * width;
    const float squareStart = 0.169f * width;
    const float squarePitch = 0.015625f * width;
    const float squareSize = 0.00625f * width;
    const float barOuter = 0.167f * width;
    const float barInner = 0.444f * width;
    const float kiOuter = 0.019f * width;
    const float kiInner = 0.163f * width;
    const float kiSlashStart = 0.0167f * width;
    const float kiSlashPitch = 0.0125f * width;
    const float kiSlashSize = 0.0083f * width;
    const float numeralX = 0.131f * width;
    const float portraitOuter = 0.173f * width;
    const float portraitInner = 0.246f * width;
    const float nameX = 0.302f * width;
    const float guardOuter = 0.0104f * width;
    const float guardInner = 0.258f * width;
    const float timerHalf = 0.052f * width;

    // The two sides are mirror images about the middle of the window, so everything
    // below is computed once in left-hand coordinates and flipped for player two.
    for (int side = 0; side < 2; ++side) {
        const bool mirrored = side == 1;
        const Fighter& fighter = fighters_[side];
        const int maxLife = std::max(1, fighter.maxLife());
        const int life = std::max(0, fighter.life());
        const float lean = mirrored ? kHudSlant : -kHudSlant;
        auto flip = [&](float x) { return mirrored ? width - x : x; };
        auto leftOf = [&](float outer, float inner) { return mirrored ? flip(inner) : outer; };

        // --- the health plate, its squares and the bar -----------------------------
        const float plateWidth = plateInner - plateOuter;
        renderer.fillShearedBar(leftOf(plateOuter, plateInner), 8.8f, plateWidth, 13.3f, lean,
                                kPlateFill);
        renderer.fillShearedBar(leftOf(plateOuter, plateInner), 8.8f, plateWidth, 1.2f, lean,
                                kPlateEdge);

        // The squares: seven slots along the plate's outer end, of which the lit ones are
        // the health bars the fighter has left. Three lit is the game's own default for a
        // three-bar fighter, which is what a 3000-life character here is.
        const int bars = std::clamp(
            static_cast<int>(std::lround(static_cast<float>(maxLife) / 1000.0f)), 1,
            kHealthBarsMax);
        const int barsLeft = std::clamp(
            static_cast<int>(std::ceil(static_cast<float>(life) /
                                       (static_cast<float>(maxLife) / static_cast<float>(bars)))),
            0, bars);
        for (int slot = 0; slot < kHealthBarsMax; ++slot) {
            const float x = squareStart + static_cast<float>(slot) * squarePitch;
            renderer.fillRect(flip(mirrored ? x + squareSize : x), 10.6f, squareSize, 2.6f,
                              slot < barsLeft ? kSquareLit : kSquareDim);
        }

        // The bar's slot, then the bar: its outer end is fixed and it is eaten from the
        // centre end, which is the opposite of the usual arrangement and is what the game
        // does.
        const float barWidth = barInner - barOuter;
        renderer.fillShearedBar(leftOf(barOuter, barInner), 15.9f, barWidth, 4.4f, lean,
                                kLifeSlot);
        drawLifeBar(renderer, flip(barOuter), flip(barInner), 15.9f, 4.4f, life, maxLife,
                    lifeTrail_[side], recoverable_[side], mirrored);
        renderer.fillShearedBar(leftOf(barOuter, barInner), 20.1f, barWidth, 0.6f, lean,
                                kLifeEdge);

        // --- the ki gauge ----------------------------------------------------------
        renderer.fillShearedBar(leftOf(kiOuter, kiInner), 23.8f, kiInner - kiOuter, 13.2f, lean,
                                kPlateFill);
        const int filled = std::clamp(
            static_cast<int>(std::lround(static_cast<float>(fighter.power()) /
                                          (static_cast<float>(kPowerMax) / kKiSlots))),
            0, kKiSlots);
        const bool maxed = filled >= kKiSlots;
        for (int slot = 0; slot < kKiSlots; ++slot) {
            // Diagonal slashes rather than blocks: the slot leans the same way the plates
            // do, which is what gives the gauge its "//" look.
            const float x = kiSlashStart + static_cast<float>(slot) * kiSlashPitch;
            renderer.fillShearedBar(flip(mirrored ? x + kiSlashSize : x), 26.0f, kiSlashSize,
                                    8.8f, lean,
                                    slot < filled ? (maxed ? kKiMax : kKiFull) : kKiEmpty);
        }

        if (font != nullptr) {
            // The count of filled bars, at the gauge's inner end: lavender, or red when
            // there is nothing left to spend.
            char count[4];
            std::snprintf(count, sizeof(count), "%d", filled);
            font->draw(renderer, flip(numeralX), 26.6f, count, 0, TextAlign::Center,
                       filled > 0 ? kMuted : kLifeRed, 1.5f);

            // The name, between the gauges. The portrait sits in front of it.
            font->draw(renderer, flip(nameX), 26.6f, fighter.name().c_str(), 0,
                       mirrored ? TextAlign::Right : TextAlign::Left, kInk, 1.1f);
        }

        // The portrait: the character's own face art, clipped into its box so that a
        // large portrait cannot spill over the gauges.
        const FaceSide& face = side == 0 ? game_.fight().faceP1 : game_.fight().faceP2;
        if (characters_[side] != nullptr && face.present && face.face.sprite.has_value()) {
            const float boxLeft = mirrored ? flip(portraitInner) : portraitOuter;
            renderer.pushClip(boxLeft, 23.8f, boxLeft + (portraitInner - portraitOuter), 37.0f);
            const DisplayElement& element = face.face;
            const float dx = boxLeft + 2.0f - (face.pos.x + element.offset.x);
            const float dy = 25.0f - (face.pos.y + element.offset.y);
            drawSpriteAt(renderer, characters_[side]->sprites(), element.sprite->first,
                         element.sprite->second, face.pos.x + element.offset.x + dx,
                         face.pos.y + element.offset.y + dy, element.facing, element.scale.x,
                         element.scale.y, Color{}, element.trans);
            renderer.popClip();
        }

        // --- the guard bar ---------------------------------------------------------
        // Thin, white-outlined, teal, draining from its centre end as blocks are made --
        // which is what makes relentless pressure worth more than patience.
        const float guardLeft = leftOf(guardOuter, guardInner);
        const float guardWidth = guardInner - guardOuter;
        renderer.fillRect(guardLeft, 38.8f, guardWidth, 0.8f, kInk);
        renderer.fillRect(guardLeft, 48.6f, guardWidth, 0.8f, kInk);
        renderer.fillRect(guardLeft, 38.8f, 0.8f, 10.6f, kInk);
        renderer.fillRect(guardLeft + guardWidth - 0.8f, 38.8f, 0.8f, 10.6f, kInk);
        const float guardFraction =
            static_cast<float>(fighter.machine().guardEnergy()) / StateMachine::kGuardEnergyMax;
        const float guardFill = guardWidth * std::clamp(guardFraction, 0.0f, 1.0f);
        renderer.fillRect(mirrored ? guardInner - guardFill : guardOuter, 39.6f, guardFill, 9.0f,
                          kGuardFill);
    }

    // --- the timer ---------------------------------------------------------------
    {
        const float capsuleLeft = width * 0.5f - timerHalf;
        renderer.fillRect(capsuleLeft, 7.1f, timerHalf * 2.0f, 16.7f, Color::rgba8(18, 18, 30, 220));
        renderer.fillRect(capsuleLeft, 7.1f, timerHalf * 2.0f, 1.2f, kTimerRim);
        renderer.fillRect(capsuleLeft, 22.6f, timerHalf * 2.0f, 1.2f, kTimerRim);
        renderer.fillRect(capsuleLeft, 7.1f, 1.2f, 16.7f, kTimerRim);
        renderer.fillRect(capsuleLeft + timerHalf * 2.0f - 1.2f, 7.1f, 1.2f, 16.7f, kTimerRim);
        if (font != nullptr) {
            char text[8];
            std::snprintf(text, sizeof(text), "%d", std::max(0, timer_));
            font->draw(renderer, width * 0.5f, 10.8f, text, 0, TextAlign::Center, kMuted, 1.4f);
        }
    }

    // --- the damage counter -------------------------------------------------------
    // "N HITS!" and "N dmg.", on the side of whoever is doing the hitting, floating in the
    // open screen below the gauges where nothing else is drawn.
    if (font != nullptr) {
        for (int side = 0; side < 2; ++side) {
            if (comboTicks_[side] <= 0 || comboHits_[side] < 2) continue;
            const float x = side == 0 ? width * 0.22f : width * 0.78f;
            char hits[24];
            char damage[24];
            std::snprintf(hits, sizeof(hits), "%d HITS!", comboHits_[side]);
            std::snprintf(damage, sizeof(damage), "%d dmg.", comboDamage_[side]);
            font->draw(renderer, x, 62.0f, hits, 0, TextAlign::Center, kLifeTrail, 1.3f);
            font->draw(renderer, x, 74.0f, damage, 0, TextAlign::Center, kInk, 1.1f);
        }
    }
}

void FightScreen::drawWinIcons(Renderer& renderer) {
    const FightDef& fight = game_.fight();
    SpriteBank& sprites = game_.fightSprites();
    const FontTable fonts = game_.fontTable(true);

    struct WinSideRef {
        const WinIconSide* side;
        int wins;
    };
    const WinSideRef sides[2] = {{&fight.winIconP1, roundsWon_[0]}, {&fight.winIconP2, roundsWon_[1]}};

    for (const WinSideRef& side : sides) {
        if (!side.side->present) continue;

        // The first icon shows how the round was actually won; the rest are
        // plain wins, because only the newest icon carries the method.
        const auto fallback = side.side->icons.find("n");
        if (fallback == side.side->icons.end()) continue;

        const int shown = std::min(side.wins, std::max(0, fight.winIconUseUpTo));
        for (int i = 0; i < shown; ++i) {
            const DisplayElement* icon = &fallback->second;
            if (i == 0) {
                const auto it = side.side->icons.find(winMethod_);
                if (it != side.side->icons.end()) icon = &it->second;
            }

            drawElement(renderer, *icon, sprites, fight.animations, fonts, tick_,
                        side.side->pos.x + static_cast<float>(i) * side.side->iconOffset.x,
                        side.side->pos.y + static_cast<float>(i) * side.side->iconOffset.y);
        }
    }
}

void FightScreen::draw(Renderer& renderer) {
    // The shake is applied to the world and not to the HUD: MUGEN's EnvShake moves the
    // fight, and a screen shake that moved the life bars would be the wrong thing
    // entirely. It is a drawn offset, so the fighters' real positions are untouched.
    const float shakeX = entities_.shakeOffsetX();
    const float shakeY = entities_.shakeOffsetY();

    if (shakeX != 0.0f || shakeY != 0.0f) {
        renderer.pushTranslate(shakeX, shakeY);
    }

    // The fight's two global palette grades, in the renderer's space. BGPalFX grades the
    // background layer; AllPalFX grades everything. The stage carries both when both are
    // live (BGPalFX first, then AllPalFX over it), and everything after the stage carries
    // AllPalFX alone -- that is the layering MUGEN's three-slot model describes. The
    // grade rides the palette path, so it moves the stage sprites and the fighters'
    // indexed art; the procedural test arena's solid rectangles, the colour washes and
    // the HUD's true-colour glyphs are outside it. See Renderer::setGlobalPalFx.
    const StateMachine::PalFxState& bgSlot = entities_.bgPalFx();
    const StateMachine::PalFxState& allSlot = entities_.allPalFx();
    const bool bgOn = bgSlot.active;
    const bool allOn = allSlot.active;
    const PalFxParams bgGrade = StateMachine::palFxParams(bgSlot);
    const PalFxParams allGrade = StateMachine::palFxParams(allSlot);
    {
        const PalFxParams stageGrade =
            Renderer::composePalFx(bgOn ? bgGrade : PalFxParams{}, allGrade);
        if (!stageGrade.identity()) renderer.setGlobalPalFx(stageGrade);
    }

    drawStage(renderer, camera_);

    // From the bottom of the stage down, the whole screen is AllPalFX's: fighters,
    // effects, HUD and banners all draw with it in force. BGPalFX is deliberately not
    // reapplied here -- it is a background grade and the background has been drawn.
    if (allOn) {
        renderer.setGlobalPalFx(allGrade);
    } else {
        renderer.clearGlobalPalFx();
    }

    // `under` washes go behind the fighters, over the stage.
    const auto env = entities_.envColorState();
    if (env.ticks > 0 && env.under) {
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), kWindowHeight,
                          Color{env.r, env.g, env.b, 0.5f});
    }
    // A running super flash dims the world behind its actors: MUGEN's
    // `darken = 1`. The HUD is drawn after and stays bright.
    const PauseInfo pause = entities_.pauseInfo();
    if (pause.ticks > 0 && pause.darken) {
        // Draw between the stage and the fighters so the flash's actors stand
        // out of the dark: the dim is backdrop-deep, not world-deep.
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), kWindowHeight,
                          Color{0.0f, 0.0f, 0.0f, 0.45f});
    }
    // Shadows are stage-owned and match-wide except for one per-machine assert:
    // `globalnoshadow` is engine-global in the reference, so either fighter's
    // (or any helper's) assertion silences every shadow pass.
    const bool drawShadows = game_.config().drawShadows && !anyGlobalNoShadow();
    entities_.setShadowStyle(stageShadow_, drawShadows);

    // Auras and dust go under the fighters, sparks and beams over them.
    entities_.drawBehind(renderer, camera_, floorY_);
    drawFighters(renderer, camera_, drawShadows);
    entities_.drawFront(renderer, camera_, floorY_);
    // And the `ontop` explods over all of it, inside the shake translate because
    // they are part of the world that shakes. See `EntityLayer::drawTop` for the
    // one judgement this makes -- how far up "over all other sprites" reaches --
    // and why it is recorded rather than silent.
    entities_.drawTop(renderer, camera_, floorY_);
    // And the common case: the wash over everything, which is what the red flash
    // on a killing blow is.
    if (env.ticks > 0 && !env.under) {
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), kWindowHeight,
                          Color{env.r, env.g, env.b, 0.5f});
    }
    if (shakeX != 0.0f || shakeY != 0.0f) {
        renderer.popTranslate();
    }

    drawHud(renderer);
    drawBanners(renderer);
    // The touch pad is drawn by the engine after this screen, in its own pixel space.
    // It is not part of the fight, so it must not wear the fight's grade: clear it now
    // rather than relying on the next beginFrame.
    renderer.clearGlobalPalFx();
}
}  // namespace af