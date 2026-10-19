cmake_minimum_required(VERSION 3.22.1)

project(anifight LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# The engine is split so that `core/`, `gfx/` and `engine/` never touch Android
# APIs -- only `platform/` and `jni/` do. That keeps the MUGEN format loaders,
# the screen flow and (later) the physics portable, so the same sources can be
# built for desktop to debug a screenpack without waiting on an emulator.
# ---------------------------------------------------------------------------
set(ANIFIGHT_SOURCES
    jni/bridge.cpp

    platform/asset_file_source.cpp
    platform/audio_out.cpp

    audio/mixer.cpp
    audio/snd_bank.cpp

    core/ini.cpp

    engine/anim.cpp
    engine/character.cpp
    engine/cmd.cpp
    engine/cns.cpp
    engine/continue_flow.cpp
    engine/defs.cpp
    engine/entities.cpp
    engine/engine.cpp
    engine/expr.cpp
    engine/fighter.cpp
    engine/game.cpp
    engine/motif.cpp
    engine/projectile_rules.cpp
    engine/select_grid.cpp
    engine/stage.cpp
    engine/statemachine.cpp
    engine/storyboard.cpp
    engine/touch_controls.cpp
    engine/touch_layout.cpp
    engine/victory_flow.cpp
    engine/win_flow.cpp

    engine/screens/continue_screen.cpp
    engine/screens/fight_screen.cpp
    engine/screens/layout_screen.cpp
    engine/screens/select_screen.cpp
    engine/screens/splash_screen.cpp
    engine/screens/stage_select_screen.cpp
    engine/screens/storyboard_screen.cpp
    engine/screens/title_screen.cpp
    engine/screens/versus_screen.cpp
    engine/screens/victory_screen.cpp
    engine/screens/win_screen.cpp

    gfx/renderer.cpp
    gfx/pcx_codec.cpp
    gfx/sff_codecs.cpp
    gfx/sprite_bank.cpp
    gfx/font.cpp
)

add_library(anifight SHARED ${ANIFIGHT_SOURCES})

target_include_directories(anifight PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(anifight PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

find_library(log_lib log)
find_library(android_lib android)
find_library(gles_lib GLESv3)
find_library(z_lib z)

target_link_libraries(anifight
    ${log_lib}
    ${android_lib}
    ${gles_lib}
    # zlib, for the inflate step of the SFF v2 PNG sprite formats
    # (gfx/sff_codecs.cpp writes the PNG parser itself).
    ${z_lib}
    # OpenSL ES, not AAudio: AAudio arrived in API 26 and this app's minSdk is
    # 24. Deprecated, present everywhere, and a buffer queue player is the same
    # forty lines it always was.
    OpenSLES
)
