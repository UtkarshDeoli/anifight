// Stages: the arena a fight happens in.
//
// A stage .def is a `[BGdef]` block naming a sprite file and a list of `[BG n]`
// layers, plus a handful of numbers that are not decoration at all -- where the
// floor is, how far the camera may travel, and where the fighters start. Those
// numbers are what make a stage a place rather than a picture, so they are
// parsed here rather than left to the fight screen to invent.
//
// The layer format is the same one system.def uses for its own backgrounds, so
// the parsing is too: see parseBackground().
#pragma once

#include <memory>
#include <string>

#include "core/file_source.h"
#include "core/ini.h"
#include "engine/defs.h"
#include "gfx/renderer.h"
#include "gfx/sprite_bank.h"

namespace af {

struct StageDef {
    std::string name;
    // `[Info] displayname`, which MUGEN documents as the name to show in the
    // stage select list. It defaults to `name` when the stage omits it, so a
    // caller only has to read this one field. (`bgs.html`, Info Group.)
    std::string displayName;
    std::string author;
    // The coordinate space this stage was authored in. Every coordinate in the
    // rest of the stage is in these units: a 1280x720 stage writes its floor at
    // y=670, not y=223, and a 320x240 stage writes it at y=190. Reading the
    // numbers without translating them is what puts a hi-res stage's floor
    // below the screen (`bgs.html`, StageInfo Group; `coordspace.html`).
    Vec2 localCoord{320.0f, 240.0f};

    // `[StageInfo] xscale` / `yscale`: an extra drawing scale the stage asks
    // for on top of the coordinate translation (MUGEN 1.0; `history.html`).
    // Defaults are 1,1, so only stages that opted in move.
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    std::string spriteFile;  // [BGdef] spr

    // The floor, in stage coordinates. A fighter's feet land here, which is
    // why MUGEN calls it the stage's z-offset: the whole stage is positioned
    // relative to it, and moving it moves the arena up or down behind the
    // fighters. Use stageTranslation() to get the fight's units.
    float zOffset = 190.0f;

    // How far the camera may travel either side of the stage's origin. A
    // screenpack that leaves these out gets a camera that barely moves, which
    // is the safe reading: the camera then never shows past the art.
    float boundLeft = -160.0f;
    float boundRight = 160.0f;

    // How this stage wants the fighters' shadows drawn. A stage is entitled to
    // say, which is why it is read rather than assumed.
    ShadowStyle shadow;

    // Where the camera sits before the fighters move it, and where each of them
    // starts, all in stage coordinates.
    Vec2 cameraStart;
    Vec2 p1Start{-70.0f, 0.0f};
    Vec2 p2Start{70.0f, 0.0f};

    // `[PlayerInfo] leftbound` / `rightbound`: the hard x limits players may
    // walk to, independent of where the camera can go. The default matches the
    // widest value stages written for MUGEN use; a stage that means to pin its
    // fighters closer says so.
    float playerLeftBound = -4000.0f;
    float playerRightBound = 4000.0f;

    // `[Bound] screenleft` / `screenright`: how close a player may walk to the
    // screen's edge when the camera has stopped at one of its bounds. The 15
    // here is what stock MUGEN stages write; omitting the section is not meant
    // to let fighters fall off the view.
    float screenLeft = 15.0f;
    float screenRight = 15.0f;

    BgDef background;
};

// Converts a stage's local coordinate space into the fight's units.
//
// MUGEN translates stage vectors into the game coordinate space by the ratio of
// the two widths (`coordspace.html`: "the ratio of the target coordinate
// space's width and the source coordinate space's width"). The equivalent ratio
// here is vertical: the fight window's height is fixed at the screenpack's
// height while its width follows the device, so the height ratio is the one
// that keeps `zoffset` at the same fraction of the screen the stage author drew
// it at -- which is exactly what zoffset is ("the vertical distance of the
// ground level from the top of the screen"). For a stage whose aspect matches
// the window the two ratios coincide. x and y are kept equal unless the stage
// asked for a separate `[StageInfo] xscale`/`yscale`, because scaling the axes
// differently would shear the art.
struct StageTranslation {
    float x = 1.0f;
    float y = 1.0f;

    [[nodiscard]] Vec2 apply(Vec2 value) const { return Vec2{value.x * x, value.y * y}; }
};

// `viewportHeight` is the fight window's height in its own units -- the height
// a 240p stage is authored against.
[[nodiscard]] StageTranslation stageTranslation(const StageDef& def, float viewportHeight);


StageDef parseStageDef(const IniFile& document);

// The file a select.def stage entry names. Entries are written either as a path
// to the .def or as a folder under stages/, and both spellings are common.
[[nodiscard]] std::string stageDefPath(const std::string& entry);

// Resolves a file reference inside the folder a stage .def lives in.
//
// MUGEN resolves `spr` from the data root, so a stage in `stages/` may write
// either `spr = x.sff` or the fully-qualified `spr = stages/x.sff`, and both
// are legal. Prepending the folder to the second spelling looks for
// `stages/stages/x.sff`, which is exactly how three stages in the local corpus
// lost their art. An absolute reference is returned unchanged.
[[nodiscard]] std::string resolveStageFile(const std::string& folder, const std::string& file);

class Stage {
public:
    Stage() = default;
    ~Stage() = default;
    Stage(const Stage&) = delete;
    Stage& operator=(const Stage&) = delete;

    bool load(FileSource& files, const std::string& defPath, std::string* errorOut);

    [[nodiscard]] bool loaded() const { return loaded_; }
    [[nodiscard]] const StageDef& def() const { return def_; }
    [[nodiscard]] const std::string& defPath() const { return defPath_; }

    // Mutable because a sprite bank uploads its textures on first draw.
    [[nodiscard]] SpriteBank& sprites() { return sprites_; }

private:
    StageDef def_;
    SpriteBank sprites_;
    std::string defPath_;
    bool loaded_ = false;
};

}  // namespace af
