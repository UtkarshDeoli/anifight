// The loaded game: everything the engine reads off disk, in one place.
//
// Loading is deliberately a single up-front step rather than lazy per-screen
// loading. A screenpack is a few hundred kilobytes and the whole point of
// loading it eagerly is that a broken one fails loudly at boot, with a specific
// message, instead of leaving the player on a black screen wondering which
// screen broke.
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "core/file_source.h"
#include "engine/character.h"
#include "engine/anim.h"
#include "engine/defs.h"
#include "engine/motif.h"
#include "audio/snd_bank.h"
#include "engine/stage.h"
#include "gfx/font.h"
#include "gfx/sprite_bank.h"

namespace af {

// One select-grid cell: either a loaded character, the `randomselect` slot,
// or an entry whose folder failed -- in which case the grid still shows a
// cell, because dropping it would renumber the roster and slide every later
// character into the wrong slot.
struct RosterCell {
    bool random = false;
    std::unique_ptr<Character> character;
    // The stages this entry names, in preference order. Empty for a character
    // that is happy anywhere, which is most of them.
    std::vector<std::string> stages;
};

class Game {
public:
    Game() = default;
    // Stops the load worker. A game destroyed mid-load would otherwise leave a
    // thread writing into a container that no longer exists.
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Reads mugen.cfg, follows its `motif` to the screenpack, and loads the
    // screenpack's sprites and fonts. Returns false only when there is nothing
    // to run at all; a missing optional file is logged and skipped.
    bool load(FileSource& files);

    // Loading, in pieces.
    //
    // A real roster is minutes of work -- a character with a thousand sprites
    // takes seconds, and forty of them do not fit in a frame -- so the engine
    // drives it one item at a time and draws a progress bar between items
    // instead of showing a black screen until it is over.
    //
    // `beginLoad` does everything that has to happen before anything can be
    // drawn: mugen.cfg, the screenpack, its sprites and fonts, select.def and
    // fight.def. It also works out how much is left. `stepLoad` does one piece
    // of what remains and returns true while there is more to do.
    //
    // `beginLoad` reads the screenpack and its fonts on the GL thread, because a
    // font uploads its glyph strip as it is read and there is no context to
    // upload in anywhere else. Everything after that -- the characters and the
    // stages -- happens on a worker thread, and `stepLoad` is the GL thread
    // collecting what it has finished.
    //
    // The split is not decoration. Decoding a character's sprites is seconds of
    // work, and doing it on the GL thread means the loading screen cannot
    // repaint while it happens: a bar that jumps ten seconds at a time looks
    // like a crash. Off the GL thread the bar moves every frame, which is the
    // whole point of having one.
    //
    // It is safe because decoding touches no GL: a sprite bank turns bytes into
    // RGBA rows, and the texture is made on the GL thread when the sprite is
    // first *drawn*. The file source is safe to share too -- every read opens
    // and closes its own handle.
    bool beginLoad(FileSource& files);
    bool stepLoad();
    [[nodiscard]] bool loadDone() const { return loadPhase_ == LoadPhase::Done; }
    [[nodiscard]] int loadDoneCount() const;
    [[nodiscard]] int loadTotalCount() const;
    // What is being loaded right now, for the loading screen to name. Returned
    // by value: the worker thread writes it, so it is read under the same lock
    // that guards everything else in flight.
    [[nodiscard]] std::string loadItem() const;

    [[nodiscard]] bool loaded() const { return loaded_; }
    [[nodiscard]] const std::string& root() const { return root_; }

    [[nodiscard]] const Config& config() const { return config_; }
    [[nodiscard]] const SystemDef& system() const { return system_; }
    [[nodiscard]] const FightDef& fight() const { return fight_; }
    [[nodiscard]] const SelectDef& select() const { return select_; }

    // The select grid, in select.def order. Empty until load() has run.
    // Non-const for the same reason systemSprites() is: drawing a character
    // uploads its textures.
    [[nodiscard]] std::vector<RosterCell>& roster() { return roster_; }
    // The k-th roster cell's character, or nullptr when that cell is a random
    // slot or its folder failed to load. Non-const Character because drawing
    // from its sprite bank uploads textures.
    [[nodiscard]] Character* rosterCharacter(int cell) const;

    // Every stage select.def names, in the order it names them: the extra
    // stages first, then whatever a roster entry brings with it. Duplicates are
    // loaded once, so two characters naming the same stage share a copy.
    [[nodiscard]] const std::vector<std::unique_ptr<Stage>>& stages() const { return stages_; }

    // The stage a match should be fought in for this fighter: the first of its
    // own stages that loaded, else the screenpack's extra stages. Null only
    // when no stage loaded at all, which is what keeps the engine test's
    // procedural arena reachable.
    [[nodiscard]] Stage* stageForMatch(const Character* fighter) const;

    [[nodiscard]] SpriteBank& systemSprites() { return systemSprites_; }
    [[nodiscard]] SpriteBank& fightSprites() { return fightSprites_; }
    // The hit sparks and the other fight effects: the screenpack's `fightfx` pair, which
    // is where a HitDef's plain `sparkno` points. Kept apart from the fight screen's own
    // art because they are two different files with two different jobs -- one draws the
    // gauges, the other draws the impacts.
    [[nodiscard]] SpriteBank& fightFxSprites() { return fightFxSprites_; }
    [[nodiscard]] const AnimationSet& fightFxAnimations() const { return fightFxAnimations_; }

    // The screenpack's menu sounds, from system.def's `snd` key. MUGEN keeps
    // these at group 100 -- 0 cursor move, 1 confirm, 2 cancel -- which is why
    // they are a bank of their own rather than fight.snd's group 0.
    [[nodiscard]] const SndBank& systemSounds() const { return systemSounds_; }

    // The fight screen's own sounds, from fight.def's `snd` key: the menu blips,
    // the round start, the knock-out.
    //
    // Kept apart from `commonSounds` deliberately. Group 0 sample 0 is a menu
    // blip in this file and a character's first swing in its own, and one list
    // covering both would answer a punch with a click.
    [[nodiscard]] const SndBank& fightSounds() const { return fightSounds_; }

    // The sounds every *character* shares, from fight.def's `common.snd` key:
    // the hit and guard groups. Character-specific sounds come from the
    // character's own bank when it ships one; there are no synthesised
    // fallbacks, so a pair that neither bank carries is silent.
    [[nodiscard]] const SndBank& commonSounds() const { return commonSounds_; }

    // Font slots are numbered 1..10, matching MUGEN. Returns nullptr for an
    // unset slot or one that failed to load. `fightDef` selects fight.def's
    // font table rather than system.def's.
    Font* font(int index, bool fightDef = false);

    // The whole table at once, for handing to the drawing helpers.
    [[nodiscard]] FontTable fontTable(bool fightDef = false) const;

private:
    // Resolves a name against the directory holding the screenpack, so that a
    // screenpack keeping its assets in a subfolder works unchanged.
    [[nodiscard]] std::string resolve(const std::string& relative) const;

    void loadFonts(const std::array<std::string, 11>& table, bool fightDef);

    // Where the load is. `Stages` is only ever a starting state: the worker
    // walks the stages and then the roster in one go, and the GL thread does not
    // need to know which of the two it is on.
    enum class LoadPhase { Idle, Stages, Done };

    // Collects the stages select.def names, deduplicated, without loading them.
    void gatherStages();

    // Resolves a select.def entry ("kfm/kfm.def") against the game root's
    // chars/ folder, where MUGEN puts character folders.
    [[nodiscard]] std::string characterDefPath(const std::string& defPath) const;

    // Shared by the system and fight sprite files, which are loaded the same
    // way. A failure here is logged and skipped rather than propagated: art is
    // not what makes the game runnable.
    void loadSprites(SpriteBank& bank, const std::string& file);

    FileSource* files_ = nullptr;
    std::string root_;        // directory of the screenpack, with trailing '/'
    std::string motifPath_;   // the motif itself, relative to the game root

    Config config_;
    SystemDef system_;
    FightDef fight_;
    SelectDef select_;

    LoadPhase loadPhase_ = LoadPhase::Idle;
    // The stages select.def names, deduplicated and in the order they are
    // reached. Built once, then loaded by the pool.
    std::vector<std::string> pendingStages_;
    // The select.def entries that get a cell, in cell order. Hidden entries do
    // not take one: the grid is what the player walks, and a hidden character
    // is one the game may use but the cursor cannot land on. Kept as pointers
    // into `select_`, which does not change while the load runs.
    std::vector<const RosterEntry*> pendingRoster_;

    // ------------------------------------------------------- the load worker
    //
    // The pool fills `readyStages_`/`readyRoster_` *in place*, one slot per
    // item, and never touches the containers the rest of the game reads;
    // `publishLoad` moves them across under the lock once the last item is in.
    // In-place slots are what keep cell order without any locking around the
    // containers themselves, and the single publish is why a half-built
    // character is never visible: nothing is published until it is complete.
    //
    // One thread per core up to four: loading a pack is parsing (def, air, cns,
    // cmd, sprite index) rather than waiting on I/O, so a phone's other cores
    // are the cheapest speed-up the boot has.
    void runLoadWorker();
    void publishLoad();

    std::thread worker_;
    mutable std::mutex loadMutex_;
    std::string loadItem_;
    std::vector<std::unique_ptr<Stage>> readyStages_;
    std::vector<RosterCell> readyRoster_;
    std::atomic<int> loadDone_{0};
    std::atomic<bool> loadFinished_{false};
    int loadTotal_ = 0;
    std::atomic<int> rosterFailed_{0};

    SpriteBank systemSprites_;
    SpriteBank fightSprites_;
    SpriteBank fightFxSprites_;
    AnimationSet fightFxAnimations_;
    SndBank systemSounds_;
    SndBank fightSounds_;
    SndBank commonSounds_;
    // Where the screenpack's sounds came from, for the log.
    std::vector<std::string> fightSoundPaths_;

    // Owned here rather than by the screens: a character's sprite bank is the
    // same object the select screen draws a portrait from and the fight screen
    // draws a fighter from, and loading it twice would mean two copies of every
    // texture on the device.
    std::vector<RosterCell> roster_;

    // Stages are loaded even when they are never drawn, because a stage that
    // failed to load is a fight with no floor, and discovering that at match
    // start is worse than discovering it at boot.
    std::vector<std::unique_ptr<Stage>> stages_;

    std::array<std::unique_ptr<Font>, 11> systemFonts_;
    std::array<std::unique_ptr<Font>, 11> fightFonts_;

    bool loaded_ = false;
};

}  // namespace af
