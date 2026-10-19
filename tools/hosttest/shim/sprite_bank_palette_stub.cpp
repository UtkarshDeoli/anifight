// A sprite-bank shim that keeps a real palette *table*, for the host test of
// `Character::paletteChoices` / `palNoForChoice` / `defaultPaletteChoice`.
//
// It is a sibling of shim/sprite_bank_stub.cpp, not a replacement, and the one
// difference is the whole reason it exists. That shim's `addPalette` returns -1
// for every palette it is offered -- the honest answer for a bank with no art,
// and all test_palette_flip needs, because it only cares about the *bytes*
// offered (see palette_recorder.h). But `Character` records every accepted
// palette's table index in actPalettes_, and treats a negative index as "this
// palN names no .act". Under the no-op shim every index is -1, so a character
// always looks palette-less and the choice mapping can never be exercised. The
// three functions under test here take their input straight from actPalettes_
// and from the bank's paletteCount(), so a test of them needs a bank that
// actually accepts palettes and reports a count.
//
// This shim therefore does what the real SpriteBank::addPalette does with no GL
// in the picture: it appends the 768 bytes to palettes_ and returns the new
// index, 0-based. Nothing here uploads, draws or reads pixels -- same no-op
// story as the other shim for every other entry point -- so it stays a pure
// host build. `addPalette` is a SpriteBank member, so it reaches the private
// palettes_ vector and the private nested Palette type directly.
//
// It is a separate file rather than a flag on the shared shim on purpose:
// test_statemachine links the shared shim and another session may be working in
// that suite, so the shared shim is left exactly as it is.
//
// Build line (see tools/hosttest/test_palette_choice.cpp):
//   clang++ ... tools/hosttest/shim/sprite_bank_palette_stub.cpp
#include <cstring>

#include "gfx/sprite_bank.h"

namespace af {

SpriteBank::~SpriteBank() = default;

bool SpriteBank::load(FileSource& files, const std::string& path, std::string* errorOut) {
    (void)files;
    (void)path;
    if (errorOut != nullptr) *errorOut = "sprite loading is not linked into this test";
    return false;
}

// A loaded texture's destructor releases a GL object; no bank here is ever
// loaded, so the bank's own destructor just has to name the type.
Texture::~Texture() = default;

// `Character` measures its standing art the moment it loads. An empty bank is
// the honest answer for a test that loads no art.
const Sprite* SpriteBank::find(int group, int image) const {
    (void)group;
    (void)image;
    return nullptr;
}

// The real thing, minus the GL: append and hand back the new index. This is
// what makes `paletteCount()` non-zero, and what lets `Character` record a
// non-negative actPalettes_ entry for each .act it reads.
int SpriteBank::addPalette(const uint8_t* rgb768) {
    if (rgb768 == nullptr) return -1;
    Palette palette;
    std::memcpy(palette.rgb.data(), rgb768, palette.rgb.size());
    palette.colors = 256;
    palettes_.push_back(palette);
    // Parallel to palettes_, as the real bank keeps it; an .act has no palette
    // node identity.
    paletteKeys_.emplace_back();
    return static_cast<int>(palettes_.size()) - 1;
}

// Uploads the draw path reaches for. All need a GL context and return null
// without one; declared because the bank is linked whole, never called.
const Texture* SpriteBank::texture(int group, int image) {
    (void)group;
    (void)image;
    return nullptr;
}

const Texture* SpriteBank::lutTexture(int index) {
    (void)index;
    return nullptr;
}

int SpriteBank::spritePalette(int group, int image) const {
    (void)group;
    (void)image;
    return -1;
}

// Drawing, for the sake of a test whose subject is a number, not a picture.
void drawSpriteAt(Renderer& renderer, SpriteBank& bank, int group, int image, float x, float y,
                  int facing, float scaleX, float scaleY, Color tint, TransParams trans,
                  const PaletteBind* palette, bool flipV) {
    (void)renderer;
    (void)bank;
    (void)group;
    (void)image;
    (void)x;
    (void)y;
    (void)facing;
    (void)scaleX;
    (void)scaleY;
    (void)tint;
    (void)trans;
    (void)palette;
    (void)flipV;
}

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
