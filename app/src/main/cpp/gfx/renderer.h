// A batched quad renderer for GLES 3.0.
//
// MUGEN draws nothing but axis-aligned sprites, so the entire graphics engine
// is "draw quads, batch adjacent ones that share a texture, flush on change".
// Everything renders into a virtual coordinate space (MUGEN's `localcoord`,
// canonically 320x240) that is letterboxed onto whatever the device reports.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace af {

// MUGEN's per-draw blend modes.
enum class Blend {
    Normal,      // src over dst, using the sprite's alpha
    Additive,    // dst + src
    Subtractive, // dst - src
};

// One draw's transparency, which MUGEN writes as a *pair* of alphas rather than
// a single opacity: `src` scales what the sprite contributes and `dst` scales
// how much of the scene behind it survives. The two are the numbers in
// `trans = addalpha, 256, 0` and in the `.air` field `AS128D128`, and MUGEN's own
// documentation of the latter gives the arithmetic in as many words -- "Mix 50%
// source with 50% dest", and "AS256D256 is equivalent to just A".
//
// MUGEN writes both on a 0..256 scale where 256 is full; here they are 0..1.
// `srcAlpha` is applied through the sprite's own alpha channel (they multiply),
// and `dstAlpha` through the blender's constant alpha -- see `blendStateFor` for
// why the two factors cannot be the same mechanism.
struct TransParams {
    Blend blend = Blend::Normal;
    float srcAlpha = 1.0f;
    float dstAlpha = 1.0f;

    constexpr TransParams() = default;
    // Deliberately implicit, so a caller with only a blend mode to say -- which
    // is most of them, and every caller from before `dstAlpha` existed -- writes
    // `frame.blend` and gets MUGEN's full-destination default.
    constexpr TransParams(Blend mode, float src = 1.0f, float dst = 1.0f)
        : blend(mode), srcAlpha(src), dstAlpha(dst) {}

    bool operator==(const TransParams&) const = default;
};

// The blend state one `TransParams` turns into, in GL's own enum numbers.
//
// A table rather than a `switch` inside `applyBlend`, because the interesting
// part of it -- that `add` and `add1` differ only in the destination factor --
// is a fact about MUGEN that nothing else in the engine can be asked, and a host
// test that links no GL can still read this. The numbers are GLES 3.0's:
//
//   GL_FUNC_ADD              0x8006      GL_SRC_ALPHA          0x0302
//   GL_FUNC_REVERSE_SUBTRACT 0x800B      GL_ONE                0x0001
//                                        GL_ONE_MINUS_SRC_ALPHA 0x0303
//                                        GL_CONSTANT_ALPHA     0x8004
struct BlendState {
    unsigned equation = 0x8006;
    unsigned source = 0x0302;
    unsigned destination = 0x0303;
    // The destination alpha to hand `glBlendColor`, and *absent* for every state
    // whose destination factor does not read it. The number is carried rather
    // than a flag because it is the part a host test with no GL can still be
    // asked about, and because a state that says `GL_CONSTANT_ALPHA` without
    // saying which constant is not a state at all.
    std::optional<float> constantAlpha;

    bool operator==(const BlendState&) const = default;
};

[[nodiscard]] constexpr BlendState blendStateFor(Blend blend, float dstAlpha) {
    // A full destination factor is `GL_ONE` rather than a constant of 1, which
    // is the same arithmetic and keeps the common case -- every `add` and `sub`
    // in the format, and every draw that predates them -- off `glBlendColor`
    // entirely.
    const bool full = dstAlpha >= 1.0f;

    switch (blend) {
        case Blend::Normal:
            // MUGEN has no destination alpha for a normal draw: the sprite
            // simply covers what is behind it in proportion to its own alpha.
            return BlendState{0x8006, 0x0302, 0x0303, std::nullopt};
        case Blend::Additive:
            return full ? BlendState{0x8006, 0x0302, 0x0001u, std::nullopt}
                        : BlendState{0x8006, 0x0302, 0x8004u, dstAlpha};
        case Blend::Subtractive:
            return full ? BlendState{0x800B, 0x0302, 0x0001u, std::nullopt}
                        : BlendState{0x800B, 0x0302, 0x8004u, dstAlpha};
    }
    return BlendState{0x8006, 0x0302, 0x0303, std::nullopt};
}

// The vertex colour a draw's transparency asks for: the source alpha rides the
// tint's alpha, which the fragment shader multiplies into the sampled texel, and
// which `GL_SRC_ALPHA` then reads back out. Doing it here rather than at each
// call site is what keeps the one place the source half of the pair is applied
// reviewable -- the destination half is `blendStateFor`'s business above.

struct Color {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    static constexpr Color rgba8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return Color{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }
    static constexpr Color rgb(uint32_t hex) {
        return rgba8(static_cast<uint8_t>((hex >> 16) & 0xFF),
                     static_cast<uint8_t>((hex >> 8) & 0xFF),
                     static_cast<uint8_t>(hex & 0xFF));
    }
};

[[nodiscard]] constexpr Color transTint(Color tint, const TransParams& trans) {
    tint.a *= trans.srcAlpha;
    return tint;
}

// An RGBA8 texture on the GPU, or -- for palette-indexed sprites -- an R8
// texture whose red channel IS the MUGEN palette index (0 is the transparent
// key, a convention of every 8-bit MUGEN source format: PCX, the SFF v2 RLE/LZ5
// block family, and PNG8 alike).
//
// An indexed texture cannot mean anything on its own: it needs a 256x1 LUT to
// turn indices into colours. Sprites come decoloured and get their colours at
// draw time so that MUGEN's whole palette machinery -- palno selection,
// RemapPal, PalFX -- can re-point or grade them without re-uploading pixels.
class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Uploads tight RGBA8 pixels (top-left origin, which is how MUGEN stores
    // them). Returns false and leaves the texture empty on failure.
    bool uploadRgba8(const uint8_t* pixels, int width, int height);

    // Uploads tight 8-bit palette indices as a GL_R8 texture. Marks the texture
    // indexed(); the renderer then routes draws through the LUT path.
    bool uploadRed8(const uint8_t* pixels, int width, int height);

    // Uploads 768 bytes of RGB palette triples as a 256x1 RGB8 texture: entry N
    // sits at texel N, sampled at ((N + 0.5) / 256, 0.5).
    bool uploadLut256(const uint8_t* rgb768);

    [[nodiscard]] bool valid() const { return id_ != 0; }
    [[nodiscard]] unsigned id() const { return id_; }
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    // Whether this texture holds palette indices rather than RGBA. Draws of
    // indexed textures must supply a LUT: the caller's when it has a palette
    // opinion (a fighter's RemapPal selection), the defaultLut() otherwise.
    [[nodiscard]] bool indexed() const { return indexed_; }
    void setDefaultLut(const Texture* lut) { defaultLut_ = lut; }
    [[nodiscard]] const Texture* defaultLut() const { return defaultLut_; }

    void release();

private:
    unsigned id_ = 0;
    int width_ = 0;
    int height_ = 0;
    bool indexed_ = false;
    // Weak; owned by the SpriteBank alongside this texture. A texture never
    // outlives its bank, so neither pointers nor LUTs dangle.
    const Texture* defaultLut_ = nullptr;
};

// Whether a GL context is currently current on this thread.
//
// Texture destruction has to call glDeleteTextures, but when the app is torn
// down Android may have already destroyed the context, and deleting GL objects
// without a current context is undefined behaviour. The engine flips this off
// before letting anything be destroyed in that situation, so destruction
// degrades to "drop the handle" -- the GPU memory dies with the context anyway.
void setGlContextAlive(bool alive);
[[nodiscard]] bool glContextAlive();

// MUGEN's PalFX colour grade, expressed in the 0..1 space the shader works in:
//
//   rgb = lut * mul + add          (MUGEN writes mul on a 0..256 scale, add on
//                                   0..255; the caller divides before arriving)
//   rgb = mix(grey(rgb), rgb, color)   colour factor: 0 is all grey, 1 full
//   if invert: rgb = 1 - rgb       invertall
//
// The identity (default-constructed) values leave the palette untouched.
struct PalFxParams {
    std::array<float, 3> mul{1.0f, 1.0f, 1.0f};
    std::array<float, 3> add{0.0f, 0.0f, 0.0f};
    float color = 1.0f;
    bool invert = false;

    [[nodiscard]] bool identity() const { return *this == PalFxParams{}; }
    bool operator==(const PalFxParams&) const = default;
};

// The palette side of one draw: which 256-entry LUT colours the sprite's
// indices, and what the PalFX grade does to them on the way out. Null to the
// draw calls means "no opinion" -- an indexed sprite then draws with the LUT it
// was decoded against and no grade, which is what every palette-blind caller
// (screenpacks, backgrounds, fonts) wants.
struct PaletteBind {
    const Texture* lut = nullptr;
    PalFxParams fx;
};

// A source rectangle within a texture, in texels.
struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
};

// How the virtual (MUGEN localcoord) space is mapped onto the device screen.
//
// Classic MUGEN stretches a 320x240 screenpack across whatever it is given,
// which is why MUGEN on a widescreen monitor looks horizontally squashed. On a
// phone that is far worse, so we letterbox by default and let a game opt back
// into the original behaviour from mugen.cfg.
enum class AspectMode {
    Fit,      // uniform scale, black bars on the two short sides
    Stretch,  // classic MUGEN: fill the screen, distorting the aspect
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Creates programs and buffers. Requires a current GLES3 context.
    bool init(const char** errorOut);
    void shutdown();

    // The device surface changed size.
    void setFramebufferSize(int width, int height);

    // The virtual (MUGEN localcoord) space the game draws in.
    void setVirtualSize(float width, float height);
    void setAspectMode(AspectMode mode);
    [[nodiscard]] AspectMode aspectMode() const { return aspect_; }

    // Converts a device-space point (pixels, top-left origin) into the virtual
    // space. Used to turn touch positions into game coordinates.
    void framebufferToVirtual(float fbX, float fbY, float* outX, float* outY) const;

    // Begins a frame: clears to `clear` and resets the batch.
    void beginFrame(Color clear);
    void endFrame();

    // Switches to a space of `width` x `height` *pixels*, mapped one unit to one
    // device pixel, until the matching popSpace().
    //
    // The game draws in its screenpack's coordinate space and lets the renderer
    // letterbox it. The on-screen pad must not: a control needs to be the same size
    // under the thumb whatever the game's aspect ratio is, so it is laid out as
    // fractions of the screen itself -- which is what PPSSPP's control format does,
    // and what makes a saved layout survive a rotation or a different phone.
    //
    // The batch is flushed first because the transform is uploaded once a frame:
    // everything already pushed was pushed in the old space, and re-uploading under
    // it would move it.
    void pushPixelSpace(float width, float height);
    void popSpace();

    // Shifts everything drawn afterwards by (dx, dy) virtual units, until the matching
    // popTranslate(). This is the screen shake: MUGEN's `EnvShake` moves the fight and not
    // the gauges, and a shake that moved the life bars would be the wrong thing entirely.
    void pushTranslate(float dx, float dy);
    void popTranslate();

    // The fight's global palette grade, applied on top of every indexed sprite drawn
    // until it is cleared. This is how `BGPalFX` and `AllPalFX` reach the screen: the
    // renderer has no per-layer render pass to hang them on -- how MUGEN decides which
    // draw layer a PalFX applies to is not understood -- so the fight sets the grade
    // around the draws it wants graded and clears it after. The stage is graded under
    // BGPalFX alone; everything after it is graded under AllPalFX.
    //
    // It rides the palette path, so -- exactly like a fighter's own grade -- it moves
    // indexed (LUT) art only. True-colour sprites (PNG24/32) and the font glyphs are
    // not reached; a whole-frame post-process would be needed to grade those, and this
    // engine has none. A cleared frame begins with no global grade.
    void setGlobalPalFx(const PalFxParams& fx) {
        globalPalFxOn_ = true;
        globalPalFx_ = fx;
    }
    void clearGlobalPalFx() { globalPalFxOn_ = false; }

    // Merges two PalFX grades -- an inner applied first, an outer applied over it --
    // into the single grade the shader can apply in one pass. The merge is exact
    // whenever either grade is the identity, which is every real case: the global grade
    // is the only one in play when it is set. Where both are non-identity it is an
    // approximation, because the greyscale blend and the invert do not commute with the
    // add and the multiply. Public so a caller stacking its own grades -- the fight
    // does, for a stage that carries both BGPalFX and AllPalFX -- lands on the same
    // arithmetic the renderer uses for the global grade.
    [[nodiscard]] static PalFxParams composePalFx(const PalFxParams& inner,
                                                  const PalFxParams& outer);

    // Restricts drawing to the given rectangle in virtual coordinates until the
    // matching popClip(). This is what implements MUGEN's `window` parameter,
    // which is how a scrolling menu hides the items above and below it. Clips
    // nest; the innermost one wins.
    void pushClip(float x1, float y1, float x2, float y2);
    void popClip();

    // Solid axis-aligned rectangle in virtual coordinates.
    void fillRect(float x, float y, float w, float h, Color color, Blend blend = Blend::Normal);

    // A filled quadrilateral with four arbitrary corners, in virtual coordinates,
    // wound top-left, top-right, bottom-right, bottom-left.
    //
    // MUGEN draws nothing but axis-aligned sprites, which is why the renderer had
    // no use for this: every sprite a character owns arrives as a rectangle. The
    // game's own chrome is not bound by that. A Shin Budokai-style lifebar is a
    // parallelogram, and it changes length every time someone is hit, so it cannot
    // be baked into a sprite either -- the slant has to survive the bar shrinking.
    struct Quad {
        float x[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float y[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };
    void fillQuad(const Quad& quad, Color color, Blend blend = Blend::Normal);

    // The common case of the above: a bar whose top and bottom edges are the same
    // length but offset from each other, so its ends are cut at an angle. A
    // positive `shear` leans the bar's top edge to the right.
    void fillShearedBar(float x, float y, float w, float h, float shear, Color color,
                        Blend blend = Blend::Normal);

    // Draws `src` texels of `tex` into a `w`x`h` box whose top-left is (x, y)
    // in virtual coordinates. `trans` is MUGEN's transparency for this draw:
    // `srcAlpha` scales the sprite's own alpha -- which is how a state's `Trans`
    // controller and an `.air` frame's `AS<n>D<m>` both reach the blender -- and
    // `dstAlpha` scales what the scene behind it keeps.
    //
    // `palette` matters only for indexed() textures: it overrides the LUT and
    // grade they would otherwise draw with (fighter palette selection, PalFX).
    void drawSprite(const Texture& tex, float x, float y, float w, float h, const Rect& src,
                    Color tint = Color{}, bool flipX = false, bool flipY = false,
                    TransParams trans = {}, const PaletteBind* palette = nullptr);

    // Convenience: draw the whole texture.
    void drawSprite(const Texture& tex, float x, float y, const Rect& src, Color tint = Color{},
                    bool flipX = false, bool flipY = false, TransParams trans = {},
                    const PaletteBind* palette = nullptr);

    // The sprite rotated by `degrees` (positive = clockwise on screen, the way
    // MUGEN counts angles) about (pivotX, pivotY), which is the character's
    // axis -- pos plus frame offsets -- so a lean pivots where the feet are.
    // (x, y) is the unrotated top-left, as drawSprite takes it.
    void drawSpriteRotated(const Texture& tex, float pivotX, float pivotY, float x, float y,
                           float w, float h, const Rect& src, float degrees,
                           Color tint = Color{}, bool flipX = false, bool flipY = false,
                           TransParams trans = {}, const PaletteBind* palette = nullptr);

    [[nodiscard]] float virtualWidth() const { return virtualWidth_; }
    [[nodiscard]] float virtualHeight() const { return virtualHeight_; }
    [[nodiscard]] int framebufferWidth() const { return fbWidth_; }
    [[nodiscard]] int framebufferHeight() const { return fbHeight_; }

    // A small soft-edged dot, used as a touch marker and for debug overlays.
    [[nodiscard]] const Texture& dot() const { return dot_; }

    // A hard-edged disc with a one-pixel feathered rim, at a size worth scaling
    // from. The round buttons of an on-screen pad and the round gauges of a
    // fighting-game HUD are both this shape, and neither can be had from the
    // soft touch marker above: scaling a 16-pixel dot up to a 140-pixel button
    // turns its one-pixel rim into nine pixels of fog.
    [[nodiscard]] const Texture& disc() const { return disc_; }

private:
    struct Vertex {
        float x, y;
        float u, v;
        float r, g, b, a;
    };

    void flush();
    void applyBlend(Blend blend, float dstAlpha);
    void applyClip();
    void pushQuad(const Texture& tex, float x, float y, float w, float h, const Rect& src,
                  Color tint, bool flipX, bool flipY, unsigned lutId = 0,
                  const PalFxParams* fx = nullptr);
    // Resolves which LUT (if any) a draw of `tex` uses: the caller's binding
    // when given, the texture's own default otherwise. Sets *lutIdOut to 0 for
    // the RGBA path, and *fxOut to the grade to apply (null = identity).
    static void resolvePalette(const Texture& tex, const PaletteBind* palette,
                               unsigned* lutIdOut, const PalFxParams** fxOut);
    // Palette state is per-batch: a change flushes what is queued, exactly like
    // a texture change, then re-arms the uniforms before the next vertices.
    void applyPaletteState(unsigned lutId, const PalFxParams* fx);

    unsigned program_ = 0;
    unsigned vao_ = 0;
    unsigned vbo_ = 0;
    int uTransformLoc_ = -1;
    int uTextureLoc_ = -1;
    int uPaletteLoc_ = -1;
    int uUsePaletteLoc_ = -1;
    int uPalMulLoc_ = -1;
    int uPalAddLoc_ = -1;
    int uPalColorLoc_ = -1;
    int uPalInvertLoc_ = -1;

    Texture white_;  // 1x1 opaque white, so fillRect reuses the sprite path
    Texture dot_;    // touch marker / debug overlay
    Texture disc_;   // round buttons and gauges

    std::vector<Vertex> vertices_;
    unsigned currentTexture_ = 0;
    // The blend state the batch is currently armed with, whole. One value rather
    // than a mode and a factor side by side, because they are one GL state: the
    // equation, the func and -- when the func reads it -- the constant. Two draws
    // that differ only in the destination alpha are two states, and comparing the
    // state itself is what makes that impossible to get wrong here; `BlendState`'s
    // own equality, which a host test checks, is the whole of the comparison.
    BlendState currentBlendState_{};

    // The batch's palette state: the LUT bound to unit 1 and the grade uniform
    // set, when the current run of quads holds indexed sprites. Off for the
    // ordinary RGBA path (unit 1 stays unbound and the uniforms never move).
    bool currentPaletteOn_ = false;
    unsigned currentLut_ = 0;
    PalFxParams currentPalFx_;

    // The fight's global grade, when one is set. Not part of the batch's palette state
    // (which is what is *uploaded*): it is merged into that state at every palette draw,
    // so a change to it is caught by the same flush-on-change test.
    bool globalPalFxOn_ = false;
    PalFxParams globalPalFx_;

    // Active clip rectangles in virtual coordinates, as x1,y1,x2,y2.
    std::vector<std::array<float, 4>> clipStack_;

    // Coordinate spaces pushed with pushPixelSpace(), innermost last.
    struct Space {
        float virtualWidth, virtualHeight;
        AspectMode aspect;
        float sx, ox, sy, oy;
        float pixelScale, pixelOffsetX, pixelOffsetY;
        std::array<float, 4> baseClip;
    };
    std::vector<Space> spaces_;

    // Translation offsets pushed with pushTranslate(), innermost last.
    std::vector<std::array<float, 2>> translates_;

    int fbWidth_ = 1;
    int fbHeight_ = 1;
    float virtualWidth_ = 320.0f;
    float virtualHeight_ = 240.0f;
    AspectMode aspect_ = AspectMode::Fit;

    // Cached transform: x' = x * sx + ox, y' = y * sy + oy  (virtual -> NDC)
    float sx_ = 1.0f, ox_ = 0.0f, sy_ = 1.0f, oy_ = 0.0f;

    // The same transform in device pixels, kept for framebufferToVirtual().
    float pixelScale_ = 1.0f;
    float pixelOffsetX_ = 0.0f;
    float pixelOffsetY_ = 0.0f;
};

}  // namespace af
