#!/bin/sh
# Builds and runs every host-side test under tools/hosttest, with the host
# clang++ -- no gradle, no CMake, no device, so it is safe to run while an
# Android build is in flight.
#
# This is the whole-suite runner. tools/hosttest/run_expr_cmd_tests.sh is the
# focused one and builds only test_expr_cmd.cpp; it stays as it is for the
# common case of iterating on the expression grammar.
#
# Tests fall into three groups:
#
#   pure logic   expr, cmd, cns and the small codecs. No fixture, no GL: they
#                run on every clone and are the ones that must never go red.
#   screenpack   select.def / mugen.cfg reading, against the generated
#                screenpack under tools/screenpack/data, which the repo ships.
#   fixtures     the SFF/SND decoders and the state machine read real MUGEN
#                files -- a KFM sprite bank, a character's .def. Eleventh-hour
#                copyrighted assets that are not in the tree, so those tests
#                are SKIPped with a note when no fixture is present rather than
#                failing the run. Point the search at your own files with
#                ANIFIGHT_KFM_SFF, ANIFIGHT_SND and ANIFIGHT_CHARACTER.
#
# A nonzero exit means at least one test failed; the per-test FAIL lines name
# the scenario. Asserts the sources mark BUG document intended behaviour the
# engine does not yet have, so a failure there is a real bug report.

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
CXX=${CXX:-clang++}
OBJDIR="${TMPDIR:-/tmp}/anifight_hosttests"
FLAGS="-std=c++20 -O1 -Wall -Wextra -I $ROOT/app/src/main/cpp -I $ROOT/tools/hosttest/shim"

mkdir -p "$OBJDIR"

failed=0
ran=0
skipped=0

# build <name> <sources...>
build() {
    name=$1
    shift
    # shellcheck disable=SC2086
    if ! "$CXX" $FLAGS -o "$OBJDIR/$name" "$@" 2>"$OBJDIR/$name.build.log"; then
        printf 'BUILD FAIL %s\n' "$name"
        sed 's/^/    /' "$OBJDIR/$name.build.log"
        failed=$((failed + 1))
        return 1
    fi
    return 0
}

# run <name> [args...]
run() {
    name=$1
    shift
    if "$OBJDIR/$name" "$@" >"$OBJDIR/$name.out" 2>&1; then
        printf 'PASS      %s\n' "$name"
    else
        printf 'FAIL      %s\n' "$name"
        sed 's/^/    /' "$OBJDIR/$name.out"
        failed=$((failed + 1))
    fi
    ran=$((ran + 1))
}

skip() {
    printf 'SKIP      %s (%s)\n' "$1" "$2"
    skipped=$((skipped + 1))
}

# Finds the first existing path among the candidates given, or nothing. A
# leading candidate that a human set through the environment wins.
first_of() {
    for path in "$@"; do
        [ -n "$path" ] || continue
        case "$path" in
            /*) [ -f "$path" ] && { printf '%s\n' "$path"; return; } ;;
            *)  [ -f "$ROOT/$path" ] && { printf '%s\n' "$ROOT/$path"; return; } ;;
        esac
    done
}

# --------------------------------------------------------------- pure logic

if build test_expr_cmd "$ROOT/tools/hosttest/test_expr_cmd.cpp" \
        "$ROOT/app/src/main/cpp/engine/expr.cpp" \
        "$ROOT/app/src/main/cpp/engine/cmd.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_expr_cmd
fi

if build test_expr "$ROOT/tools/hosttest/test_expr.cpp" \
        "$ROOT/app/src/main/cpp/engine/expr.cpp"; then
    run test_expr
fi

if build test_cns_states "$ROOT/tools/hosttest/test_cns_states.cpp" \
        "$ROOT/app/src/main/cpp/engine/cns.cpp" \
        "$ROOT/app/src/main/cpp/engine/expr.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_cns_states
fi

if build test_pcx_codec "$ROOT/tools/hosttest/test_pcx_codec.cpp" \
        "$ROOT/app/src/main/cpp/gfx/pcx_codec.cpp"; then
    run test_pcx_codec
fi

if build test_select_grid "$ROOT/tools/hosttest/test_select_grid.cpp" \
        "$ROOT/app/src/main/cpp/engine/select_grid.cpp"; then
    run test_select_grid
fi

if build test_touch_layout "$ROOT/tools/hosttest/test_touch_layout.cpp" \
        "$ROOT/app/src/main/cpp/engine/touch_layout.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_touch_layout
fi

# The victory card's pure half: the distinct `%s`/`%i` substitution, the
# character `[Quotes]` parser and selection rules, and the fade-in / hold /
# fade-out clock. Like the other pure tests it has no fixture and no GL; the
# card's drawing is deliberately not host-tested.
if build test_victory_flow "$ROOT/tools/hosttest/test_victory_flow.cpp" \
        "$ROOT/app/src/main/cpp/engine/victory_flow.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_victory_flow
fi

# The Win screen's pure half: the additive fade-in / pose / fade-out clock and
# the wintext displaytime rule. The card's drawing is deliberately not
# host-tested, as with the victory card.
if build test_win_screen "$ROOT/tools/hosttest/test_win_screen.cpp" \
        "$ROOT/app/src/main/cpp/engine/win_flow.cpp"; then
    run test_win_screen
fi

# The Continue prompt's pure half: the interpreted countdown (including the
# exact expiry tick and the no-underflow rule), YES/NO clamping, and the
# Accepted/Declined result mapping FightScreen acts on. The prompt's drawing
# is deliberately not host-tested, as with the other screen cards.
if build test_continue_screen "$ROOT/tools/hosttest/test_continue_screen.cpp" \
        "$ROOT/app/src/main/cpp/engine/continue_flow.cpp"; then
    run test_continue_screen
fi

# The storyboard format and clock: scene boundaries from `end.time`, the two
# fade ramps, `layerall.pos`/`clearcolor`/`bgm` inheritance, layer and sound
# keys, and the inline AIR actions. It also parses the shipped screenpack's
# gameover.def when the data directory is passed, so the file the app actually
# plays is covered. The screen's drawing is not host-tested.
if build test_storyboard "$ROOT/tools/hosttest/test_storyboard.cpp" \
        "$ROOT/app/src/main/cpp/engine/storyboard.cpp" \
        "$ROOT/app/src/main/cpp/engine/anim.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_storyboard "$ROOT/tools/screenpack/data"
fi

# The mixer's bank-scoped full command and the storyboard unit conversions:
# a synthetic .snd goes through the real decoder, the mixer is driven by fill()
# as the audio thread drives it, and the rendered PCM proves that volume and pan
# reach the voice and that the named bank wins over registration priority.
if build test_mixer "$ROOT/tools/hosttest/test_mixer.cpp" \
        "$ROOT/app/src/main/cpp/audio/mixer.cpp" \
        "$ROOT/app/src/main/cpp/audio/snd_bank.cpp"; then
    run test_mixer
fi

# The projectile rules the device cannot show directly: the exact priority
# exchange (equal and negative included), `projhits = 0` cancellation, the
# unscaled box conversion, the velocity-aware bounds, the removal-animation
# fallback chain and the deferred player-hit timing.
if build test_projectile_rules "$ROOT/tools/hosttest/test_projectile_rules.cpp" \
        "$ROOT/app/src/main/cpp/engine/projectile_rules.cpp"; then
    run test_projectile_rules
fi

# The stage-owned shadow model: [Shadow]/[Reflection] parsing, [Config]
# drawshadows, and the alpha rule shared by the fighter and object draw paths.
# The stage parser drags in SpriteBank::load through Stage::load even though
# only parseStageDef is called, hence the sprite shim.
if build test_stage_shadow "$ROOT/tools/hosttest/test_stage_shadow.cpp" \
        "$ROOT/tools/hosttest/shim/sprite_bank_stub.cpp" \
        "$ROOT/app/src/main/cpp/engine/stage.cpp" \
        "$ROOT/app/src/main/cpp/engine/defs.cpp" \
        "$ROOT/app/src/main/cpp/engine/anim.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_stage_shadow
fi

# The fighter/helper half of [Reflection]: the mirrored draw's axis point,
# vertical flip, alpha and gates, observed through the sprite-bank shim's draw
# recorder. Synthetic character, so it runs without any character fixture.
if build test_reflection_draw "$ROOT/tools/hosttest/test_reflection_draw.cpp" \
        "$ROOT/tools/hosttest/shim/sprite_bank_stub.cpp" \
        "$ROOT/app/src/main/cpp/engine/statemachine.cpp" \
        "$ROOT/app/src/main/cpp/engine/cns.cpp" \
        "$ROOT/app/src/main/cpp/engine/cmd.cpp" \
        "$ROOT/app/src/main/cpp/engine/expr.cpp" \
        "$ROOT/app/src/main/cpp/engine/character.cpp" \
        "$ROOT/app/src/main/cpp/engine/victory_flow.cpp" \
        "$ROOT/app/src/main/cpp/engine/anim.cpp" \
        "$ROOT/app/src/main/cpp/engine/fighter.cpp" \
        "$ROOT/app/src/main/cpp/engine/entities.cpp" \
        "$ROOT/app/src/main/cpp/engine/projectile_rules.cpp" \
        "$ROOT/app/src/main/cpp/audio/snd_bank.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_reflection_draw
fi

# The .act palette flip in Character::load. It builds its own fixture -- a
# two-line .def and two .act files -- in a temp dir, so it carries no fixture
# and always runs. It links the sprite-bank shim because `Character` owns a
# bank and offers it every palN; the shim records what it is handed so the test
# can assert on the exact bytes. See tools/hosttest/shim/palette_recorder.h.
if build test_palette_flip "$ROOT/tools/hosttest/test_palette_flip.cpp" \
        "$ROOT/tools/hosttest/shim/sprite_bank_stub.cpp" \
        "$ROOT/app/src/main/cpp/engine/character.cpp" \
        "$ROOT/app/src/main/cpp/engine/victory_flow.cpp" \
        "$ROOT/app/src/main/cpp/engine/anim.cpp" \
        "$ROOT/app/src/main/cpp/engine/cns.cpp" \
        "$ROOT/app/src/main/cpp/engine/cmd.cpp" \
        "$ROOT/app/src/main/cpp/engine/expr.cpp" \
        "$ROOT/app/src/main/cpp/audio/snd_bank.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_palette_flip
fi

# The choice mapping a select screen reads: paletteChoices / palNoForChoice /
# defaultPaletteChoice in engine/character. Also a synthetic fixture -- a .def
# and some .act files written to a temp dir -- but it links the *palette-table*
# sprite shim (shim/sprite_bank_palette_stub.cpp), not the swallowing one, because
# the mapping is only observable when the bank actually accepts palettes and
# reports a count. See that shim for why the two exist side by side.
if build test_palette_choice "$ROOT/tools/hosttest/test_palette_choice.cpp" \
        "$ROOT/tools/hosttest/shim/sprite_bank_palette_stub.cpp" \
        "$ROOT/app/src/main/cpp/engine/character.cpp" \
        "$ROOT/app/src/main/cpp/engine/victory_flow.cpp" \
        "$ROOT/app/src/main/cpp/engine/anim.cpp" \
        "$ROOT/app/src/main/cpp/engine/cns.cpp" \
        "$ROOT/app/src/main/cpp/engine/cmd.cpp" \
        "$ROOT/app/src/main/cpp/engine/expr.cpp" \
        "$ROOT/app/src/main/cpp/audio/snd_bank.cpp" \
        "$ROOT/app/src/main/cpp/core/ini.cpp"; then
    run test_palette_choice
fi

# -------------------------------------------------------------- screenpack

if [ -f "$ROOT/tools/screenpack/data/select.def" ]; then
    if build test_select_config "$ROOT/tools/hosttest/test_select_config.cpp" \
            "$ROOT/app/src/main/cpp/engine/defs.cpp" \
            "$ROOT/app/src/main/cpp/core/ini.cpp" \
            "$ROOT/app/src/main/cpp/engine/anim.cpp"; then
        # Its default data dir is relative, so give it the absolute one.
        run test_select_config "$ROOT/tools/screenpack/data"
    fi
else
    skip test_select_config "no tools/screenpack/data"
fi

# ------------------------------------------------------- SFF/SND fixtures

# Kung Fu Man only: test_sff_codecs asserts the hashes of his specific sprite
# nodes, so a different character's bank is not a stand-in.
KFM_SFF=$(first_of "$ANIFIGHT_KFM_SFF" \
    local_chars/kfm/kfm.sff \
    app/build/generated/screenpack/mugen/chars/kfm/kfm.sff \
    local_mugen/extracted/mugen/chars/kfm/kfm.sff)
if [ -n "$KFM_SFF" ]; then
    # sff_codecs.cpp uses zlib's inflate for the PNG payloads, hence -lz here
    # and nowhere else.
    if build test_sff_codecs "$ROOT/tools/hosttest/test_sff_codecs.cpp" \
            "$ROOT/app/src/main/cpp/gfx/sff_codecs.cpp" -lz; then
        run test_sff_codecs "$KFM_SFF"
    fi
else
    skip test_sff_codecs "no kfm.sff (set ANIFIGHT_KFM_SFF)"
fi

SND=$(first_of "$ANIFIGHT_SND" \
    local_chars/kfm/kfm.snd \
    app/build/screenpack-staging/fight.snd \
    local_mugen/extracted/mugen/data/fight.snd)
if [ -n "$SND" ]; then
    if build test_snd_bank "$ROOT/tools/hosttest/test_snd_bank.cpp" \
            "$ROOT/app/src/main/cpp/audio/snd_bank.cpp" \
            "$ROOT/app/src/main/cpp/core/ini.cpp"; then
        run test_snd_bank "$SND"
    fi
else
    skip test_snd_bank "no .snd (set ANIFIGHT_SND)"
fi

# -------------------------------------------------------- the state machine

# Needs a whole character. KFM is the reference one; any local_chars character
# works too, and the test names the .def on its own command line.
CHAR=$(first_of "$ANIFIGHT_CHARACTER" \
    local_chars/kfm/kfm.def \
    local_chars/dbz_gogeta/dbz_gogeta.def \
    app/build/generated/screenpack/mugen/chars/kfm/kfm.def)
if [ -n "$CHAR" ]; then
    if build test_statemachine "$ROOT/tools/hosttest/test_statemachine.cpp" \
            "$ROOT/tools/hosttest/shim/sprite_bank_stub.cpp" \
            "$ROOT/app/src/main/cpp/engine/statemachine.cpp" \
            "$ROOT/app/src/main/cpp/engine/cns.cpp" \
            "$ROOT/app/src/main/cpp/engine/cmd.cpp" \
            "$ROOT/app/src/main/cpp/engine/expr.cpp" \
            "$ROOT/app/src/main/cpp/engine/character.cpp" \
            "$ROOT/app/src/main/cpp/engine/victory_flow.cpp" \
            "$ROOT/app/src/main/cpp/engine/anim.cpp" \
            "$ROOT/app/src/main/cpp/engine/fighter.cpp" \
            "$ROOT/app/src/main/cpp/engine/entities.cpp" \
            "$ROOT/app/src/main/cpp/engine/projectile_rules.cpp" \
            "$ROOT/app/src/main/cpp/audio/snd_bank.cpp" \
            "$ROOT/app/src/main/cpp/core/ini.cpp"; then
        run test_statemachine "$CHAR"
    fi
else
    skip test_statemachine "no character .def (set ANIFIGHT_CHARACTER)"
fi

# ------------------------------------------------------------------- report

printf '\n%d ran, %d skipped, %d failed\n' "$ran" "$skipped" "$failed"
[ "$failed" -eq 0 ] && printf 'ok\n'
exit "$([ "$failed" -eq 0 ] && echo 0 || echo 1)"
