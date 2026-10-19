#include "engine/game.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

#include "core/ini.h"
#include "core/log.h"

namespace af {

namespace {

// Reads a file and parses it as INI in one step. Every config file the engine
// opens is small and read whole, so there is nothing to gain from separating the
// two halves.
std::optional<IniFile> readIni(FileSource& files, const std::string& path) {
    std::unique_ptr<FileData> data = files.read(path);
    if (!data) return std::nullopt;
    return IniFile::parse(std::string_view(reinterpret_cast<const char*>(data->data()), data->size()));
}

std::string directoryOf(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return {};
    return path.substr(0, slash + 1);
}

}  // namespace

bool Game::beginLoad(FileSource& files) {
    files_ = &files;

    // --------------------------------------------------------------- mugen.cfg
    //
    // This is the entry point: it names the screenpack, so nothing else can be
    // opened until it has been read. A copy at the data root is also accepted,
    // since that is where a hand-assembled game folder usually puts it.
    std::optional<IniFile> cfg = readIni(files, "data/mugen.cfg");
    if (!cfg) cfg = readIni(files, "mugen.cfg");

    if (cfg) {
        config_ = parseConfig(*cfg);
    } else {
        AF_LOGW("no mugen.cfg found; running on built-in defaults");
    }

    // -------------------------------------------------------------- system.def
    motifPath_ = config_.motif.empty() ? std::string("data/system.def") : config_.motif;

    std::optional<IniFile> motif = readIni(files, motifPath_);
    if (!motif) {
        const std::string alt = "data/" + motifPath_;
        motif = readIni(files, alt);
        if (motif) motifPath_ = alt;
    }
    if (!motif) {
        AF_LOGE("screenpack not found: %s", motifPath_.c_str());
        return false;
    }

    // Everything the screenpack names is resolved against the folder holding the
    // screenpack itself, so one that keeps its assets in a subdirectory works
    // without any further configuration.
    root_ = directoryOf(motifPath_);
    system_ = parseSystemDef(*motif);

    AF_LOGI("screenpack '%s' by %s, localcoord %.0fx%.0f, %zu title bg layers",
            system_.name.empty() ? "(unnamed)" : system_.name.c_str(),
            system_.author.empty() ? "(unknown)" : system_.author.c_str(), system_.localCoord.x,
            system_.localCoord.y, system_.titleBg.elements.size());

    // ------------------------------------------------------------------ assets
    //
    // None of the remaining files are load-bearing for start-up. A screenpack
    // missing its sprite file should still reach the menus -- drawing the layout
    // with no art is a far more useful failure than refusing to launch, and it
    // keeps a partially-built screenpack debuggable.
    loadSprites(systemSprites_, system_.spriteFile);
    loadFonts(system_.fonts, false);

    // The menu sounds. MUGEN's system.def names its own bank, and the cursors
    // and confirms on the title and select screens ask it for group 100 rather
    // than borrowing fight.snd, which uses the same numbers for other things.
    if (!system_.soundFile.empty()) {
        const std::string path = root_ + normaliseFileRef(system_.soundFile);
        if (std::unique_ptr<FileData> data = files.read(path)) {
            std::string error;
            if (systemSounds_.load(*data, &error)) {
                AF_LOGI("screenpack sounds: %zu loaded into the system bank from %s",
                        systemSounds_.size(), path.c_str());
            } else {
                AF_LOGW("screenpack sound file %s failed: %s", path.c_str(), error.c_str());
            }
        } else {
            AF_LOGW("screenpack sound file %s not found", path.c_str());
        }
    }

    // ------------------------------------------------- select.def and fight.def
    //
    // Both are parsed here rather than in a step, because both are read for
    // their *layout* -- where the roster's cells go, where the lifebars go -- and
    // a screen drawn without them would be drawn wrong. The characters and
    // stages they name are what takes the time, and those are stepped.
    if (std::optional<IniFile> selectFile = readIni(files, resolve(system_.selectFile))) {
        select_ = parseSelectDef(*selectFile);
        AF_LOGI("roster: %zu character slots, %zu extra stages", select_.characters.size(),
                select_.extraStages.size());
    } else {
        AF_LOGW("select.def not found: %s", resolve(system_.selectFile).c_str());
    }

    if (std::optional<IniFile> fightFile = readIni(files, resolve(system_.fightFile))) {
        fight_ = parseFightDef(*fightFile);
        loadSprites(fightSprites_, fight_.spriteFile);
        loadFonts(fight_.fonts, true);
        // The hit sparks, which live in their own pair of files: `fightfx.sff` holds the
        // art and `fightfx.air` the animations that use it. A HitDef's plain `sparkno`
        // refers to the *animations* here, which is why every character's hits can share
        // one spark without any of them owning it.
        loadSprites(fightFxSprites_, fight_.fightFxSpriteFile);
        if (std::unique_ptr<FileData> air = files.read(resolve(fight_.fightFxAirFile))) {
            parseAirText(std::string_view(reinterpret_cast<const char*>(air->data()), air->size()),
                         fightFxAnimations_);
            AF_LOGI("fightfx: %zu animations from %s", fightFxAnimations_.size(),
                    resolve(fight_.fightFxAirFile).c_str());
        } else {
            AF_LOGW("fightfx animations not found: %s (hits will land without a spark)",
                    resolve(fight_.fightFxAirFile).c_str());
        }
        // The screenpack's sounds. A fight.def names two files and they are not
        // the same thing: `snd` is the fight screen's own (the round start, the
        // knock-out), and `common.snd` is the bank every character's
        // `hitsound = 5, 0` refers into.
        // The screenpack's sounds, named by fight.def the way MUGEN names them:
        // `snd` is the fight screen's own -- the round start, the knock-out --
        // and `common.snd` is the bank every character's `hitsound = 5, 0`
        // refers into. Both land in one bank because a lookup does not care
        // which file answered it.
        static constexpr std::array<std::pair<const char*, const char*>, 2> kSoundKeys = {{
            {"snd", "fight.snd"},
            {"common.snd", "common.snd"},
        }};
        for (const auto& key : kSoundKeys) {
            // Two banks, not one. `snd` is the fight screen's own and
            // `common.snd` is what characters share, and they use the same
            // numbers for different things: group 0 sample 0 is a menu blip in
            // the first and a character's opening swing in the second.
            const bool isFightBank = std::strcmp(key.first, "snd") == 0;
            SndBank& bank = isFightBank ? fightSounds_ : commonSounds_;
            const IniSection* filesSection = fightFile->section("files");
            const std::string name =
                filesSection == nullptr
                    ? std::string()
                    : normaliseFileRef(IniFile::get(*filesSection, key.first, key.second));
            if (name.empty()) continue;
            const std::string path = root_ + name;
            auto data = files.read(path);
            if (!data) {
                AF_LOGW("screenpack sound file %s not found", path.c_str());
                continue;
            }
            std::string error;
            if (bank.load(*data, &error)) {
                AF_LOGI("screenpack sounds: %zu loaded into the %s bank from %s", bank.size(),
                        isFightBank ? "fight" : "common", path.c_str());
                fightSoundPaths_.push_back(path);
            } else {
                AF_LOGW("screenpack sound file %s failed: %s", path.c_str(), error.c_str());
            }
        }
    } else {
        AF_LOGW("fight.def not found: %s", resolve(system_.fightFile).c_str());
    }

    // Everything that is left is the roster and the stages, and it is handed to
    // the load pool. See the header for why it is not done here.
    roster_.clear();
    roster_.reserve(select_.characters.size());
    rosterFailed_ = 0;
    stages_.clear();
    gatherStages();

    // The pool writes each result into its own slot, so both vectors are sized
    // once, here, before any thread starts. `pendingRoster_` is the cell order:
    // a hidden entry takes no cell, and a random cell takes one that stands for
    // whichever playable character the confirm rolls.
    pendingRoster_.clear();
    for (const RosterEntry& entry : select_.characters) {
        if (entry.isRandomCell || entry.hidden == 0) pendingRoster_.push_back(&entry);
    }
    readyStages_.clear();
    readyStages_.resize(pendingStages_.size());
    readyRoster_.clear();
    readyRoster_.resize(pendingRoster_.size());

    // Hidden entries do no work, so they are not in the total: a bar that could
    // never reach its own end would read as a hang.
    loadTotal_ = static_cast<int>(pendingStages_.size() + pendingRoster_.size());
    loadDone_.store(0);
    loadFinished_.store(false);
    loadPhase_ = LoadPhase::Stages;

    if (loadTotal_ == 0) {
        loadPhase_ = LoadPhase::Done;
        loaded_ = true;
        return true;
    }

    worker_ = std::thread([this] { runLoadWorker(); });
    return true;
}

Game::~Game() {
    // A game destroyed while the worker is still reading would leave the thread
    // writing into containers that no longer exist.
    if (worker_.joinable()) worker_.join();
}


void Game::gatherStages() {
    pendingStages_.clear();
    // Extra stages come first so that they are the fallback a match reaches for
    // when a character names none of its own.
    std::vector<std::string> wanted = select_.extraStages;
    for (const RosterEntry& entry : select_.characters) {
        for (const std::string& stage : entry.stages) wanted.push_back(stage);
    }
    for (const std::string& entry : wanted) {
        const std::string path = stageDefPath(entry);
        // A stage named by several characters is still one stage, and one that
        // failed once does not need reporting again.
        if (std::find(pendingStages_.begin(), pendingStages_.end(), path) != pendingStages_.end()) {
            continue;
        }
        pendingStages_.push_back(path);
    }
}

int Game::loadDoneCount() const { return loadDone_.load(); }

int Game::loadTotalCount() const { return loadTotal_; }

std::string Game::loadItem() const {
    std::lock_guard<std::mutex> lock(loadMutex_);
    return loadItem_;
}

void Game::runLoadWorker() {
    // One thread per core up to four. The pool is only ever entered from the
    // worker this class already owns, and every help thread is joined below
    // before `loadFinished_` is raised, so `~Game` still joins exactly one
    // thread and still cannot outlive its containers.
    const unsigned cores = std::thread::hardware_concurrency();
    const int poolSize = std::clamp<int>(cores == 0 ? 2 : static_cast<int>(cores), 1, 4);
    // The slice is only worth claiming if a device log can prove it ran wide.
    AF_LOGI("asset load pool: %d thread(s) of %u core(s)", poolSize, cores);

    // Runs `body(0..count)` over `poolSize` threads, handing out indices
    // through `next`. Each body call owns exactly one result slot, which is why
    // the shared vectors need no lock: no two threads write the same element,
    // and neither vector is resized while they run.
    auto drain = [poolSize](size_t count, std::atomic<size_t>& next, auto&& body) {
        auto run = [&] {
            for (size_t i = next.fetch_add(1); i < count; i = next.fetch_add(1)) body(i);
        };
        std::vector<std::thread> pool;
        pool.reserve(static_cast<size_t>(poolSize - 1));
        for (int t = 1; t < poolSize; ++t) pool.emplace_back(run);
        run();
        for (std::thread& thread : pool) thread.join();
    };

    std::atomic<size_t> nextStage{0};
    drain(pendingStages_.size(), nextStage, [this](size_t i) {
        const std::string& path = pendingStages_[i];
        {
            std::lock_guard<std::mutex> lock(loadMutex_);
            loadItem_ = path;
        }
        auto stage = std::make_unique<Stage>();
        std::string error;
        if (!stage->load(*files_, path, &error)) {
            AF_LOGW("stage '%s' skipped: %s", path.c_str(), error.c_str());
        } else {
            readyStages_[i] = std::move(stage);
        }
        loadDone_.fetch_add(1);
    });
    AF_LOGI("stages loaded: %d of %zu named", loadDone_.load(), pendingStages_.size());

    std::atomic<size_t> nextCell{0};
    drain(pendingRoster_.size(), nextCell, [this](size_t i) {
        const RosterEntry& entry = *pendingRoster_[i];
        {
            std::lock_guard<std::mutex> lock(loadMutex_);
            loadItem_ = entry.defPath;
        }

        RosterCell& cell = readyRoster_[i];
        if (entry.isRandomCell) {
            // A random cell takes its place on the grid and rolls a playable
            // character when the player confirms; there is nothing to load.
            cell.random = true;
        } else {
            cell.stages = entry.stages;
            const std::string path = characterDefPath(entry.defPath);
            auto character = std::make_unique<Character>();
            std::string error;
            if (!character->load(*files_, path, &error)) {
                AF_LOGW("roster entry '%s' skipped: %s", entry.defPath.c_str(), error.c_str());
                ++rosterFailed_;
                // The cell stays, empty. Dropping it would renumber the grid
                // and slide every later character into the wrong slot, which
                // makes a single missing character look like a scrambled
                // roster.
            } else {
                cell.character = std::move(character);
            }
        }
        loadDone_.fetch_add(1);
    });

    loadFinished_.store(true);
}

void Game::publishLoad() {
    std::vector<std::unique_ptr<Stage>> stages;
    std::vector<RosterCell> cells;
    {
        std::lock_guard<std::mutex> lock(loadMutex_);
        stages.swap(readyStages_);
        cells.swap(readyRoster_);
    }
    for (std::unique_ptr<Stage>& stage : stages) {
        // A slot is empty when that stage failed to load. It is dropped rather
        // than stored: a match that names it falls back to another stage, which
        // is what an absent stage means everywhere else.
        if (stage != nullptr) stages_.push_back(std::move(stage));
    }
    for (RosterCell& cell : cells) {
        roster_.push_back(std::move(cell));
    }
}

bool Game::stepLoad() {
    if (loadPhase_ == LoadPhase::Done) return false;

    // Nothing is published while the pool is still filling slots. The loading
    // screen reads only the counters, and the roster is read in cell order, so
    // a partial publish would be a grid with holes in the middle of it.
    if (!loadFinished_.load()) return true;

    // The pool has stopped. Take what it made, join it, and say so.
    if (worker_.joinable()) worker_.join();
    publishLoad();
    loadPhase_ = LoadPhase::Done;
    loaded_ = true;
    {
        std::lock_guard<std::mutex> lock(loadMutex_);
        loadItem_.clear();
    }

    size_t playable = 0;
    for (const RosterCell& cell : roster_) {
        if (cell.character != nullptr) ++playable;
    }
    AF_LOGI("roster loaded: %zu cells, %zu playable, %d failed", roster_.size(), playable,
            rosterFailed_.load());
    return false;
}

bool Game::load(FileSource& files) {
    if (!beginLoad(files)) return false;
    // Spins on the worker. Only used by callers that want the whole thing done
    // before they continue -- the engine drives `stepLoad` from a screen.
    while (!loadDone()) stepLoad();
    return true;
}


Character* Game::rosterCharacter(int cell) const {
    if (cell < 0 || cell >= static_cast<int>(roster_.size())) return nullptr;
    return roster_[static_cast<size_t>(cell)].character.get();
}

std::string Game::characterDefPath(const std::string& defPath) const {
    // select.def entries are relative to the game's chars/ folder. Three
    // spellings all mean the same file: "kfm" (a bare folder name, which
    // names the folder's own def), "kfm/kfm.def", and "chars/kfm/kfm.def".
    if (defPath.empty()) return {};
    if (defPath.rfind("chars/", 0) == 0) return defPath;
    if (defPath.find(".def") != std::string::npos) return "chars/" + defPath;
    return "chars/" + defPath + "/" + defPath + ".def";
}


Stage* Game::stageForMatch(const Character* fighter) const {
    if (fighter != nullptr) {
        for (const RosterCell& cell : roster_) {
            if (cell.character.get() != fighter) continue;
            for (const std::string& wanted : cell.stages) {
                const std::string path = stageDefPath(wanted);
                for (const auto& stage : stages_) {
                    if (stage->defPath() == path) return stage.get();
                }
            }
            break;
        }
    }
    return stages_.empty() ? nullptr : stages_.front().get();
}

void Game::loadSprites(SpriteBank& bank, const std::string& file) {
    const std::string path = resolve(file);

    std::string error;
    if (!bank.load(*files_, path, &error)) {
        AF_LOGE("sprite file %s failed to load: %s", path.c_str(), error.c_str());
        return;
    }

    AF_LOGI("loaded %zu sprites from %s (SFF v%d)", bank.sprites().size(), path.c_str(),
            bank.version());
    if (bank.decodeFailures() > 0) {
        // Worth surfacing rather than hiding: a screenpack that mostly works but
        // renders some sprites as gaps is otherwise a mystery to debug.
        AF_LOGW("%d sprite(s) in %s could not be decoded", bank.decodeFailures(), path.c_str());
    }
}

std::string Game::resolve(const std::string& relative) const {
    if (relative.empty()) return {};
    if (relative[0] == '/') return relative;  // already absolute
    return root_ + relative;
}

void Game::loadFonts(const std::array<std::string, 11>& table, bool fightDef) {
    std::array<std::unique_ptr<Font>, 11>& slots = fightDef ? fightFonts_ : systemFonts_;
    for (size_t i = 1; i < table.size(); ++i) {
        if (table[i].empty()) continue;

        const std::string path = resolve(table[i]);
        auto font = std::make_unique<Font>();
        std::string error;
        if (font->load(*files_, path, &error)) {
            slots[i] = std::move(font);
        } else {
            AF_LOGW("font %zu failed to load (%s): %s", i, path.c_str(), error.c_str());
        }
    }
}

Font* Game::font(int index, bool fightDef) {
    if (index < 1 || index > 10) return nullptr;
    return (fightDef ? fightFonts_ : systemFonts_)[static_cast<size_t>(index)].get();
}

FontTable Game::fontTable(bool fightDef) const {
    const std::array<std::unique_ptr<Font>, 11>& slots = fightDef ? fightFonts_ : systemFonts_;
    FontTable table;
    for (size_t i = 0; i < slots.size(); ++i) {
        table.slots[i] = slots[i].get();
    }
    return table;
}

}  // namespace af
