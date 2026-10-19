// Checks the two select-screen settings a roster change depends on, by running
// the engine's own parsers over the shipped screenpack files.
//
// Neither file is generated, so nothing else in the build would catch a
// regression in them: the roster would just quietly stop matching the grid.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//         -o /tmp/test_select_config tools/hosttest/test_select_config.cpp \
//         app/src/main/cpp/engine/defs.cpp app/src/main/cpp/core/ini.cpp \
//         app/src/main/cpp/engine/anim.cpp
//     /tmp/test_select_config tools/screenpack/data

#include <cmath>
#include <cstdio>
#include <string>

#include "core/ini.h"
#include "engine/defs.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++failures;
}

// The engine reads its text files through a file source; on the host a plain
// read is the same thing.
std::string slurp(const std::string& path) {
    std::string text;
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
        char buffer[4096];
        size_t n = 0;
        while ((n = std::fread(buffer, 1, sizeof buffer, f)) > 0) text.append(buffer, n);
        std::fclose(f);
    }
    return text;
}

af::IniFile parseFile(const std::string& path) {
    const std::string text = slurp(path);
    if (text.empty()) std::fprintf(stderr, "FAIL: cannot read %s\n", path.c_str());
    return af::IniFile::parse(text);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dataDir = argc > 1 ? argv[1] : "tools/screenpack/data";

    const af::SystemDef sys = af::parseSystemDef(parseFile(dataDir + "/system.def"));

    // The grid is what caps the roster: select.def entries past rows*columns
    // never get a cell the cursor can reach.
    const int cells = sys.select.rows * sys.select.columns;
    std::printf("grid: %d rows x %d columns = %d cells\n",
                sys.select.rows, sys.select.columns, cells);

    // The cursor must not stop on a cell whose character folder is absent. The
    // community entries are slots, so on a fresh clone most of the grid has no
    // character in it; landing on those cells means walking through blanks and
    // confirming on one does nothing.
    check(!sys.select.moveOverEmptyBoxes,
          "system.def sets moveoveremptyboxes = 0 (cursor skips absent characters)");

    const af::SelectDef roster = af::parseSelectDef(parseFile(dataDir + "/select.def"));

    std::printf("roster: %zu entries, %d cells\n", roster.characters.size(), cells);

    check(!roster.characters.empty(), "select.def yields at least one entry");
    check(roster.characters.size() <= static_cast<size_t>(cells),
          "select.def does not overflow the grid (entries <= rows*columns)");

    // Kung Fu Man has to be first. It is the one character a stock install is
    // likely to have, and everything else in select.def is a slot that is
    // empty until its folder appears -- with no playable cell at all the
    // cursor has nowhere to land and the select screen cannot start a match.
    if (!roster.characters.empty()) {
        check(roster.characters.front().defPath == "kfm/kfm.def",
              "the first entry is Kung Fu Man, the one character a stock install has");
    }

    // Every entry must name a folder-qualified .def. A bare `kfm` would resolve
    // to a path the loader cannot open, and read as an empty cell forever.
    for (const af::RosterEntry& entry : roster.characters) {
        if (entry.isRandomCell) continue;
        check(!entry.defPath.empty() && std::string(entry.defPath).find('/') != std::string::npos,
              "entry '" + std::string(entry.defPath) + "' is folder-qualified");
        check(entry.hidden == 0,
              "entry '" + std::string(entry.defPath) + "' is visible on the grid");
    }

    // ---------------------------------------------------------------- trans
    // The shipped screenpack writes no `trans` at all, so these cases come from
    // a synthetic document. What each one pins is the *destination* half, which
    // a bare blend mode used to lose: `add1` is the form `mugen1`'s and `big`'s
    // system.def actually use, and it is only half as strong as `add`.
    {
        const af::IniFile doc = af::IniFile::parse(
            "[LifeBar]\n"
            "p1.pos = 20, 20\n"
            "p1.bg0.spr = 1, 1\n"
            "p1.bg0.trans = add1\n"
            "p1.bg1.spr = 1, 2\n"
            "p1.bg1.trans = add\n"
            "p1.mid.spr = 1, 3\n"
            "p1.mid.trans = sub\n"
            "p1.front.spr = 1, 4\n"
            "p1.front.trans = addalpha\n"
            "p1.front.alpha = 128, 64\n"
            "p1.counter.font = 1, 0, 0\n"
            "p1.counter.trans = subalpha, 256, 0\n"
            "p2.pos = 300, 20\n"
            "p2.bg0.spr = 1, 5\n"
            "p2.bg0.trans = bogus\n"
            "p2.bg1.spr = 1, 6\n"
            "p2.mid.spr = 1, 8\n"
            "p2.mid.trans = add\n"
            "p2.mid.alpha = 0.5, 0\n"
            "[Face]\n"
            "p1.pos = 10, 20\n"
            "p1.face.spr = 1, 7\n"
            "p1.face.window = 0, 0, 64, 96\n");
        const af::FightDef fight = af::parseFightDef(doc);
        const af::BarSide& p1 = fight.lifebarP1;
        const af::BarSide& p2 = fight.lifebarP2;

        check(p1.bg0.trans.blend == af::Blend::Additive && p1.bg0.trans.srcAlpha == 1.0f &&
                  std::fabs(p1.bg0.trans.dstAlpha - 0.5f) < 0.001f,
              "trans = add1 is additive at 256,128 (the AIR shorthand AS256D128)");
        check(p1.bg1.trans.blend == af::Blend::Additive && p1.bg1.trans.dstAlpha == 1.0f,
              "trans = add keeps a full destination");
        check(p1.mid.trans.blend == af::Blend::Subtractive && p1.mid.trans.dstAlpha == 1.0f,
              "trans = sub is subtractive at a full destination");
        check(p1.front.trans.blend == af::Blend::Additive &&
                  std::fabs(p1.front.trans.srcAlpha - 128.0f / 256.0f) < 0.001f &&
                  std::fabs(p1.front.trans.dstAlpha - 64.0f / 256.0f) < 0.001f,
              "trans = addalpha takes its pair from the alpha key, in MUGEN's 0..256 units");
        check(p1.counter.trans.blend == af::Blend::Subtractive &&
                  p1.counter.trans.srcAlpha == 1.0f && p1.counter.trans.dstAlpha == 0.0f,
              "an inline pair after the type wins: subalpha, 256, 0");
        check(p2.bg0.trans.blend == af::Blend::Normal && p2.bg0.trans.dstAlpha == 1.0f,
              "an unrecognised trans is dropped, not guessed at");
        check(p2.bg1.trans.blend == af::Blend::Normal && p2.bg1.trans.dstAlpha == 1.0f,
              "an element with no trans draws normally");
        check(p2.mid.trans.blend == af::Blend::Additive &&
                  std::fabs(p2.mid.trans.srcAlpha - 0.5f) < 0.001f &&
                  p2.mid.trans.dstAlpha == 0.0f,
              "an explicit alpha pair applies to add as well, overriding its full destination");

        // An element's window is a sub-key too, and the corpus writes it that
        // way for the portraits (`p1.face.window`). Read unprefixed it was
        // always absent, so a large portrait spilled over its box.
        const std::optional<af::Rect4>& window = fight.faceP1.face.window;
        check(window.has_value() && (*window)[2] == 64.0f && (*window)[3] == 96.0f,
              "a face's window is read from its prefixed key");
    }

    // A background layer shares the parser; its `trans` and `alpha` keys are the
    // same pair, and a title screen's `add1` layer is the visible reason this
    // matters.
    {
        const af::IniFile doc = af::IniFile::parse(
            "[TitleBG 0]\n"
            "type = normal\n"
            "spriteno = 1, 1\n"
            "trans = add1\n"
            "[SelectBG 0]\n"
            "type = normal\n"
            "spriteno = 1, 2\n"
            "trans = addalpha\n"
            "alpha = 0.5, 0.25\n");
        const af::SystemDef system = af::parseSystemDef(doc);
        check(system.titleBg.elements.size() == 1 &&
                  system.titleBg.elements[0].trans.blend == af::Blend::Additive &&
                  std::fabs(system.titleBg.elements[0].trans.dstAlpha - 0.5f) < 0.001f,
              "a title background layer keeps the destination half of add1");
        check(system.selectBg.elements.size() == 1 &&
                  std::fabs(system.selectBg.elements[0].trans.srcAlpha - 0.5f) < 0.001f &&
                  std::fabs(system.selectBg.elements[0].trans.dstAlpha - 0.25f) < 0.001f,
              "and a normalised alpha pair is accepted as well as MUGEN units");
    }

    // ------------------------------------------------ post-match sections
    // The screen-flow slice parses these records inertly: nothing draws them
    // yet, so the only thing to prove is that the keys the shipped motifs use
    // reach the structs with the documented defaults and geometry. The snippet
    // spells the sections exactly as `data/system.def` and `data/big/system.def`
    // do, including the p1/wintext/continue element prefixes and a BGdef.
    {
        const af::IniFile doc = af::IniFile::parse(
            "[Music]\n"
            "title.bgm = sound/title.mp3\n"
            "title.bgm.volume = 80\n"
            "title.bgm.loop = 1\n"
            "title.bgm.loopstart = 120\n"
            "title.bgm.loopend = 480\n"
            "select.bgm = \"sound/select.mp3\"\n"
            "select.bgm.loop = 0\n"
            "vs.bgm =\n"
            "victory.bgm = sound/win.ogg\n"
            "victory.bgm.loop = 1\n"
            "[Demo Mode]\n"
            "enabled = 0\n"
            "select.enabled = 1\n"
            "title.waittime = 42\n"
            "fight.stopbgm = 0\n"
            "[Continue Screen]\n"
            "enabled = 1\n"
            "pos = 640,240\n"
            "continue.text = \"CONTINUE?\"\n"
            "continue.font = 1,0,0\n"
            "yes.text = \"YES\"\n"
            "yes.font = 1,0,0\n"
            "yes.offset = -70,50\n"
            "yes.active.text = \"YES\"\n"
            "yes.active.font = 1,3,0\n"
            "no.text = \"NO\"\n"
            "no.font = 1,0,0\n"
            "storyboard = continue.def\n"
            "[Game Over Screen]\n"
            "enabled = 1\n"
            "storyboard = gameover.def\n"
            "[Victory Screen]\n"
            "enabled = 1\n"
            "time = 300\n"
            "fadein.time = 8\n"
            "fadeout.time = 15\n"
            "p1.spr = 9000,2\n"
            "p1.offset = 100,20\n"
            "p1.window = 0,0,319,160\n"
            "p1.name.offset = 20,180\n"
            "p1.name.font = 3,0,1\n"
            "winquote.text = \"Winner!\"\n"
            "winquote.offset = 20,192\n"
            "winquote.font = 2,0,1\n"
            "winquote.window = 18,171,301,228\n"
            "winquote.textwrap = w\n"
            "[VictoryBGdef]\n"
            "bgclearcolor = 10,20,30\n"
            "[VictoryBG 1]\n"
            "type = normal\n"
            "spriteno = 100,0\n"
            "start = 0,0\n"
            "tile = 1,1\n"
            "velocity = 0,1.5\n"
            "[VictoryBG 2]\n"
            "type = normal\n"
            "spriteno = 100,1\n"
            "start = 20,13\n"
            "tile = 1,1\n"
            "trans = sub\n"
            "window = 0,19,319,160\n"
            "velocity = -2,0\n"
            "[Win Screen]\n"
            "enabled = 1\n"
            "wintext.text = \"Congratulations!\"\n"
            "wintext.font = 2,0,0\n"
            "wintext.offset = 159,70\n"
            "wintext.displaytime = -1\n"
            "wintext.layerno = 2\n"
            "fadein.time = 32\n"
            "pose.time = 300\n"
            "fadeout.time = 64\n"
            "[Survival Results Screen]\n"
            "enabled = 1\n"
            "winstext.text = \"Rounds survived: %i\"\n"
            "fadein.time = 32\n"
            "show.time = 300\n"
            "fadeout.time = 64\n"
            "roundstowin = 5\n"
            "[Default Ending]\n"
            "enabled = 0\n"
            "storyboard = ending.def\n"
            "[End Credits]\n"
            "enabled = 1\n"
            "storyboard = credits.def\n"
            "[Option Info]\n"
            "title.offset = 640,38\n"
            "title.font = 4,0,0\n"
            "cursor.move.snd = 100,0\n"
            "cursor.done.snd = 101,1\n"
            "cancel.snd = 102,2\n"
            "[OptionBGdef]\n"
            "[OptionBG 1]\n"
            "type = normal\n"
            "spriteno = 100,0\n"
            "start = 0,0\n"
            "tile = 1,1\n"
            "velocity = -1,-1\n");
        const af::SystemDef system = af::parseSystemDef(doc);

        check(system.victory.enabled, "[Victory Screen] enabled is parsed");
        check(system.victory.time == 300, "[Victory Screen] time is parsed");
        check(system.victory.fadeInTime == 8 && system.victory.fadeOutTime == 15,
              "[Victory Screen] fade times are parsed");
        check(system.victory.portrait.sprite.has_value() &&
                  system.victory.portrait.sprite->first == 9000 &&
                  system.victory.portrait.sprite->second == 2,
              "[Victory Screen] p1.spr is the character portrait 9000,2");
        check(system.victory.portrait.window.has_value() &&
                  (*system.victory.portrait.window)[2] == 319.0f &&
                  (*system.victory.portrait.window)[3] == 160.0f,
              "[Victory Screen] p1.window clips the portrait");
        check(system.victory.name.font.has_value() &&
                  system.victory.name.font->font == 3 &&
                  system.victory.name.font->align == 1,
              "[Victory Screen] p1.name.font keeps its alignment");
        check(system.victory.winQuote.text == "Winner!", "[Victory Screen] winquote text is parsed");
        check(system.victory.winQuoteTextwrap == "w", "[Victory Screen] textwrap is parsed");

        check(system.victoryBg.elements.size() == 2, "[VictoryBGdef] yields its two layers");
        check(system.victoryBg.elements[1].trans.blend == af::Blend::Subtractive,
              "[VictoryBG 2] keeps trans = sub");
        check(system.victoryBg.elements[1].velocity.x == -2.0f,
              "[VictoryBG 2] keeps its velocity");

        check(system.win.enabled, "[Win Screen] enabled is parsed");
        check(system.win.poseTime == 300 && system.win.fadeInTime == 32 &&
                  system.win.fadeOutTime == 64,
              "[Win Screen] timing keys are parsed");
        check(system.win.winText.text == "Congratulations!",
              "[Win Screen] wintext.text is parsed");
        check(system.win.winText.font.has_value() && system.win.winText.font->font == 2,
              "[Win Screen] wintext.font is parsed");
        check(system.win.winText.offset.x == 159.0f && system.win.winText.offset.y == 70.0f &&
                  system.win.winText.displaytime == -1 && system.win.winText.layerno == 2,
              "[Win Screen] wintext geometry is parsed");

        check(system.continueScreen.enabled, "[Continue Screen] enabled is parsed");
        check(system.continueScreen.pos.x == 640.0f &&
                  system.continueScreen.pos.y == 240.0f,
              "[Continue Screen] pos is parsed");
        check(system.continueScreen.prompt.text == "CONTINUE?",
              "[Continue Screen] continue.text is parsed");
        check(system.continueScreen.prompt.font.has_value() &&
                  system.continueScreen.prompt.font->font == 1 &&
                  system.continueScreen.prompt.font->align == 0,
              "[Continue Screen] continue.font is parsed");
        check(system.continueScreen.yes.offset.x == -70.0f &&
                  system.continueScreen.no.text == "NO",
              "[Continue Screen] yes/no elements are parsed");
        check(system.continueScreen.yesActive.text == "YES" &&
                  system.continueScreen.yesActive.font.has_value() &&
                  system.continueScreen.yesActive.font->bank == 3,
              "[Continue Screen] yes.active keeps its own text and font");
        // The default motif writes only `enabled` and is documented as
        // text-less, so an unwritten element must stay empty rather than
        // borrowing the big motif's labels.
        check(!system.continueScreen.noActive.font.has_value() &&
                  system.continueScreen.noActive.text.empty(),
              "[Continue Screen] an omitted no.active stays unset");
        check(system.continueScreen.storyboard == "continue.def",
              "[Continue Screen] the loader-only storyboard key is kept");

        check(system.demo.enabled == false && system.demo.selectEnabled == true &&
                  system.demo.fightStopBgm == false && system.demo.titleWaitTime == 42,
              "[Demo Mode] keys are parsed");
        check(system.demo.fightEndTime == 1500,
              "[Demo Mode] an omitted key keeps the corpus default");

        check(system.survival.enabled && system.survival.roundsToWin == 5 &&
                  system.survival.showTime == 300 && system.survival.fadeInTime == 32 &&
                  system.survival.fadeOutTime == 64,
              "[Survival Results Screen] keys are parsed");
        check(system.survival.winText.text == "Rounds survived: %i",
              "[Survival Results Screen] winstext.text is parsed");

        check(system.gameOver.enabled && system.gameOver.storyboard == "gameover.def",
              "[Game Over Screen] enabled and storyboard are parsed");
        check(!system.defaultEnding.enabled && system.defaultEnding.storyboard == "ending.def",
              "[Default Ending] enabled and storyboard are parsed");
        check(system.endCredits.enabled && system.endCredits.storyboard == "credits.def",
              "[End Credits] enabled and storyboard are parsed");

        check(system.music.title.file == "sound/title.mp3" && system.music.title.volume == 80 &&
                  system.music.title.loop && system.music.title.loopStart.has_value() &&
                  *system.music.title.loopStart == 120 && system.music.title.loopEnd == 480,
              "[Music] title.bgm and its modifiers are parsed");
        check(system.music.select.file == "sound/select.mp3" && !system.music.select.loop,
              "[Music] quoted select.bgm is unquoted");
        check(system.music.vs.file.empty() && !system.music.vs.loop,
              "[Music] an empty vs.bgm leaves no file behind");
        check(system.music.victory.file == "sound/win.ogg" && system.music.victory.loop,
              "[Music] victory.bgm is parsed");
        check(system.music.victory.loopStart.has_value() == false,
              "[Music] an omitted loopstart stays unset rather than zero");

        check(system.option.cursorMoveSound == std::make_pair(100, 0) &&
                  system.option.cursorDoneSound == std::make_pair(101, 1) &&
                  system.option.cancelSound == std::make_pair(102, 2),
              "[Option Info] the three sound keys are parsed");
        check(system.option.title.font.has_value() && system.option.title.font->font == 4 &&
                  system.option.title.offset.x == 640.0f,
              "[Option Info] the title element is parsed");
        check(system.optionBg.elements.size() == 1 &&
                  system.optionBg.elements[0].sprite == std::make_pair(100, 0) &&
                  system.optionBg.elements[0].velocity.x == -1.0f,
              "[OptionBGdef] yields its one layer");
    }

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}
