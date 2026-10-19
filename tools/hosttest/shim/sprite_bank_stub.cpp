// The drawing-half symbols a behaviour-only host test needs.
//
// `Character` owns a sprite bank, and `Fighter` is linked whole by
// test_statemachine, so the linker asks for the sprite bank and renderer
// entry points even in a test that never touches art. Linking the real
// sprite decoder instead would pull the drawing half of the engine into a
// test that has no display, for no benefit: the subject there is the state
// machine, and it does not read a single pixel.
//
// Build line (see tools/hosttest/test_statemachine.cpp):
//   clang++ ... tools/hosttest/shim/sprite_bank_stub.cpp
//
// addPalette also drives the opt-in palette recording that test_palette_flip
// reads; see palette_recorder.h for why it exists. Recording is off unless a
// test turns it on, so this file is otherwise the same no-op shim as before.
#include <cstring>

#include "draw_recorder.h"
#include "gfx/sprite_bank.h"
#include "palette_recorder.h"

namespace af {

namespace {
// The recorder's storage. File-local, so nothing outside this translation unit
// can reach it except through palette_recorder.h's own functions and the
// addPalette below, which live here too.
bool g_recording = false;
std::vector<std::array<uint8_t, 768>> g_recorded;
}  // namespace

namespace palette_stub {
void record(bool enabled) { g_recording = enabled; }
const std::vector<std::array<uint8_t, 768>>& recorded() { return g_recorded; }
void clear() { g_recorded.clear(); }
}  // namespace palette_stub

namespace {
// The draw recorder's storage, on the same terms: see draw_recorder.h.
bool g_drawRecording = false;
std::vector<draw_stub::DrawCall> g_draws;
}  // namespace

namespace draw_stub {
void record(bool enabled) { g_drawRecording = enabled; }
void push(const DrawCall& call) {
    if (g_drawRecording) g_draws.push_back(call);
}
const std::vector<DrawCall>& recorded() { return g_draws; }
void clear() { g_draws.clear(); }
}  // namespace draw_stub

SpriteBank::~SpriteBank() = default;

bool SpriteBank::load(FileSource& files, const std::string& path, std::string* errorOut) {
    (void)files;
    (void)path;
    if (errorOut != nullptr) *errorOut = "sprite loading is not linked into this test";
    return false;
}

// A loaded texture's destructor releases a GL object. No bank is ever loaded
// here, so no texture is ever created and there is nothing to release -- but
// the bank's destructor still has to name it.
Texture::~Texture() = default;

// `Character` measures its standing art the moment it loads, so the lookup has
// to exist. An empty bank is the honest answer for a test that loads no art:
// the character reports no measurable height and the fighter is left at its
// localcoord scale.
const Sprite* SpriteBank::find(int group, int image) const {
    (void)group;
    (void)image;
    return nullptr;
}

// Drawing, for the sake of a test that steps and collides two fighters without
// ever putting one on screen. `Fighter::draw` and `EntityLayer::drawExplod` are
// the callers. The parameters are recorded rather than dropped, so a test that
// has no display can still assert on *what would have been drawn* -- the
// vertical mirror an explod's `vfacing` asks for, for instance, is a fact about
// this call and nothing else in the engine. See `draw_recorder.h`.
void drawSpriteAt(Renderer& renderer, SpriteBank& bank, int group, int image, float x, float y,
                  int facing, float scaleX, float scaleY, Color tint, TransParams trans,
                  const PaletteBind* palette, bool flipV) {
    (void)renderer;
    (void)bank;
    (void)palette;
    draw_stub::push({group, image, x, y, facing, scaleX, scaleY, tint, trans, flipV});
}

// Palette loading: `Character::load` offers every palN of the .def to the bank
// even in a test with no art. A -1 for each is what a bank with no art in it
// answers, and paletteIndexFor() then falls back to the empty SFF table.
//
// The bytes are recorded when recording is on, so a test can assert on what the
// character actually handed the bank -- the palette-flip regression test is the
// one caller, and palette_recorder.h explains why it needs to see them.
int SpriteBank::addPalette(const uint8_t* rgb768) {
    if (rgb768 != nullptr && g_recording) {
        std::array<uint8_t, 768> copy{};
        std::memcpy(copy.data(), rgb768, copy.size());
        g_recorded.push_back(copy);
    }
    return -1;
}

// The uploads `Fighter::draw` reaches for. Both need a GL context and return
// null without one; no bank is loaded here, so null is also what an empty bank
// answers. Declared because the fighter is linked whole, never called.
const Texture* SpriteBank::texture(int group, int image) {
    (void)group;
    (void)image;
    return nullptr;
}

const Texture* SpriteBank::lutTexture(int index) {
    (void)index;
    return nullptr;
}

// `Fighter::draw` asks which palette a sprite decoded against to decide whether
// a fighter palette can replace it. -1 is "no known palette", the right answer
// for a bank that has never decoded anything.
int SpriteBank::spritePalette(int group, int image) const {
    (void)group;
    (void)image;
    return -1;
}

// A Renderer is constructible in a test -- its default constructor is defaulted
// and it owns nothing but vectors and plain numbers -- but its destructor lives
// in the GL half of the drawing code, which is not linked here. Defaulting it is
// not a stand-in for anything: renderer.cpp defines it exactly this way, because
// there is no GL object to release. A test that wants to check *what the entity
// layer asked to be drawn* has to hold one of these, which is the only reason
// the symbol is needed at all. See draw_recorder.h.
Renderer::~Renderer() = default;

// A rotated draw, reached only from `Fighter::draw`'s shadow path. Same story as
// drawSpriteAt above: linked whole, never called.
void Renderer::drawSpriteRotated(const Texture& tex, float pivotX, float pivotY, float x, float y,
                                 float w, float h, const Rect& src, float degrees, Color tint,
                                 bool flipX, bool flipY, TransParams trans,
                                 const PaletteBind* palette) {
    (void)tex;
    (void)pivotX;
    (void)pivotY;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)src;
    (void)degrees;
    (void)tint;
    (void)flipX;
    (void)flipY;
    (void)trans;
    (void)palette;
}

void Renderer::fillRect(float x, float y, float width, float height, Color color, Blend blend) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    (void)blend;
}

}  // namespace af


