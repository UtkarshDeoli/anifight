#include "gfx/renderer.h"

#include <GLES3/gl3.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

#include "core/log.h"

namespace af {

namespace {

bool gGlContextAlive = false;

constexpr const char* kVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

// (sx, ox, sy, oy): maps virtual coordinates straight to clip space.
uniform vec4 uTransform;

out vec2 vUV;
out vec4 vColor;

void main() {
    gl_Position = vec4(aPos.x * uTransform.x + uTransform.y,
                       aPos.y * uTransform.z + uTransform.w,
                       0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)";

constexpr const char* kFragmentShader = R"(#version 300 es
precision mediump float;

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
// The palette path: bound on unit 1 for runs of indexed sprites. The sprite
// texture's red channel carries the MUGEN palette index; uPalette is the 256x1
// LUT that turns it into colour, and the uPal* uniforms are the PalFX grade
// (mul/add/colour-factor/invert), in that order.
uniform sampler2D uPalette;
uniform int uUsePalette;
uniform vec3 uPalMul;
uniform vec3 uPalAdd;
uniform float uPalColor;
uniform float uPalInvert;

out vec4 fragColor;

void main() {
    vec4 texel = texture(uTexture, vUV);
    if (uUsePalette != 0) {
        // Index 0 is the transparent key in every MUGEN 8-bit format. The
        // grade never touches alpha, so a transparent texel stays invisible
        // even under a PalFX flash.
        float index = floor(texel.r * 255.0 + 0.5);
        if (index <= 0.0) {
            texel = vec4(0.0);
        } else {
            vec3 rgb = texture(uPalette, vec2((index + 0.5) / 256.0, 0.5)).rgb;
            // MUGEN's `color` parameter blends toward greyscale first: 256 keeps
            // the colour, 0 is a full desaturation. Its luma is fixed point --
            // (5R + 7G + 4B) / 16, not Rec.601's weights -- so the grey an
            // author gets from color = 0 is the grey MUGEN makes.
            float grey = dot(rgb, vec3(5.0, 7.0, 4.0) / 16.0);
            rgb = mix(vec3(grey), rgb, clamp(uPalColor, 0.0, 1.0));
            // invertall is a per-channel NOT of the colour, and it lands before
            // the add and the multiply, not after them.
            if (uPalInvert > 0.5) rgb = vec3(1.0) - rgb;
            // add is a signed offset applied before the gain: MUGEN computes
            // (colour + add) * mul, so a mul of 0 drains a flash to black
            // whatever its add was.
            rgb = (rgb + uPalAdd) * uPalMul;
            texel = vec4(clamp(rgb, 0.0, 1.0), 1.0);
        }
    }
    fragColor = texel * vColor;
    // A fully transparent fragment is not drawn at all, which is what MUGEN's
    // colour key does and what its arithmetic assumes. It matters now that a
    // destination alpha can scale what is behind: with `dstAlpha` below 1 the
    // destination factor applies to *every* fragment the quad covers, so a
    // transparent texel left in the blend would darken the scene through the
    // sprite's empty corners instead of leaving it alone.
    if (fragColor.a <= 0.0) discard;
}
)";

unsigned compileShader(GLenum type, const char* source, const char** errorOut) {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log) - 1, nullptr, log);
        AF_LOGE("shader compile failed: %s", log);
        if (errorOut) *errorOut = "shader compile failed (see logcat)";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

}  // namespace

void setGlContextAlive(bool alive) { gGlContextAlive = alive; }

bool glContextAlive() { return gGlContextAlive; }

// --- Texture ---------------------------------------------------------------

Texture::~Texture() { release(); }

Texture::Texture(Texture&& other) noexcept
    : id_(other.id_),
      width_(other.width_),
      height_(other.height_),
      indexed_(other.indexed_),
      defaultLut_(other.defaultLut_) {
    other.id_ = 0;
    other.width_ = 0;
    other.height_ = 0;
    other.indexed_ = false;
    other.defaultLut_ = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        release();
        id_ = other.id_;
        width_ = other.width_;
        height_ = other.height_;
        indexed_ = other.indexed_;
        defaultLut_ = other.defaultLut_;
        other.id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.indexed_ = false;
        other.defaultLut_ = nullptr;
    }
    return *this;
}

namespace {

// The shared tail of every upload: create/bind, force NEAREST (MUGEN sprites
// are pixel art -- never smooth them), CLAMP_TO_EDGE so NEAREST sampling cannot
// pick up a neighbour's texels at odd scales, then the transfer.
bool uploadTexture(unsigned& id, int* widthOut, int* heightOut, GLint internalFormat,
                   GLenum format, const uint8_t* pixels, int width, int height) {
    if (pixels == nullptr || width <= 0 || height <= 0) return false;

    if (id == 0) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        if (tex == 0) return false;
        id = tex;
    }

    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Sprites repeat edge texels; without this, NEAREST sampling can pick up
    // the transparent border at odd scales.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE,
                 pixels);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        AF_LOGE("texture upload failed (%dx%d), gl error 0x%04x", width, height, err);
        return false;
    }

    *widthOut = width;
    *heightOut = height;
    return true;
}

}  // namespace

bool Texture::uploadRgba8(const uint8_t* pixels, int width, int height) {
    if (!uploadTexture(id_, &width_, &height_, GL_RGBA8, GL_RGBA, pixels, width, height)) {
        release();
        return false;
    }
    indexed_ = false;
    defaultLut_ = nullptr;
    return true;
}

bool Texture::uploadRed8(const uint8_t* pixels, int width, int height) {
    if (!uploadTexture(id_, &width_, &height_, GL_R8, GL_RED, pixels, width, height)) {
        release();
        return false;
    }
    indexed_ = true;
    return true;
}

bool Texture::uploadLut256(const uint8_t* rgb768) {
    // A palette is 256 entries wide and one texel tall; index N lands at
    // ((N + 0.5) / 256, 0.5), exactly on a texel centre with NEAREST sampling.
    if (!uploadTexture(id_, &width_, &height_, GL_RGB8, GL_RGB, rgb768, 256, 1)) {
        release();
        return false;
    }
    indexed_ = false;
    return true;
}

void Texture::release() {
    if (id_ != 0 && gGlContextAlive) {
        GLuint tex = id_;
        glDeleteTextures(1, &tex);
    }
    id_ = 0;
    width_ = 0;
    height_ = 0;
    indexed_ = false;
    defaultLut_ = nullptr;
}

// --- Renderer --------------------------------------------------------------

Renderer::~Renderer() = default;

bool Renderer::init(const char** errorOut) {
    unsigned vs = compileShader(GL_VERTEX_SHADER, kVertexShader, errorOut);
    if (vs == 0) return false;
    unsigned fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader, errorOut);
    if (fs == 0) {
        glDeleteShader(vs);
        return false;
    }

    unsigned program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[1024] = {};
        glGetProgramInfoLog(program, sizeof(log) - 1, nullptr, log);
        AF_LOGE("program link failed: %s", log);
        if (errorOut) *errorOut = "program link failed (see logcat)";
        glDeleteProgram(program);
        return false;
    }
    program_ = program;

    uTransformLoc_ = glGetUniformLocation(program_, "uTransform");
    uTextureLoc_ = glGetUniformLocation(program_, "uTexture");
    uPaletteLoc_ = glGetUniformLocation(program_, "uPalette");
    uUsePaletteLoc_ = glGetUniformLocation(program_, "uUsePalette");
    uPalMulLoc_ = glGetUniformLocation(program_, "uPalMul");
    uPalAddLoc_ = glGetUniformLocation(program_, "uPalAdd");
    uPalColorLoc_ = glGetUniformLocation(program_, "uPalColor");
    uPalInvertLoc_ = glGetUniformLocation(program_, "uPalInvert");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    const GLsizei stride = sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(4 * sizeof(float)));

    glBindVertexArray(0);

    // 1x1 white so fillRect can share the sprite path.
    const uint8_t whitePixel[4] = {255, 255, 255, 255};
    if (!white_.uploadRgba8(whitePixel, 1, 1)) {
        if (errorOut) *errorOut = "failed to create white texture";
        return false;
    }

    // Soft dot for the touch marker.
    {
        constexpr int kDotSize = 16;
        uint8_t dotPixels[kDotSize * kDotSize * 4] = {};
        const float centre = (kDotSize - 1) * 0.5f;
        for (int y = 0; y < kDotSize; ++y) {
            for (int x = 0; x < kDotSize; ++x) {
                const float dx = static_cast<float>(x) - centre;
                const float dy = static_cast<float>(y) - centre;
                const float dist = std::sqrt(dx * dx + dy * dy);
                // Solid core, one-pixel feathered rim.
                float alpha = (centre + 0.5f) - dist;
                if (alpha > 1.0f) alpha = 1.0f;
                if (alpha < 0.0f) alpha = 0.0f;
                const size_t i = (static_cast<size_t>(y) * kDotSize + x) * 4;
                dotPixels[i + 0] = 255;
                dotPixels[i + 1] = 255;
                dotPixels[i + 2] = 255;
                dotPixels[i + 3] = static_cast<uint8_t>(alpha * 255.0f);
            }
        }
        dot_.uploadRgba8(dotPixels, kDotSize, kDotSize);
    }

    vertices_.reserve(6 * 1024);

    setGlContextAlive(true);
    AF_LOGI("renderer ready (GLES %s)", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    return true;
}

void Renderer::shutdown() {
    vertices_.clear();
    white_.release();
    dot_.release();
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    // Everything that needed the context is now gone.
    setGlContextAlive(false);
}

void Renderer::setFramebufferSize(int width, int height) {
    fbWidth_ = width > 0 ? width : 1;
    fbHeight_ = height > 0 ? height : 1;
    glViewport(0, 0, fbWidth_, fbHeight_);
    setVirtualSize(virtualWidth_, virtualHeight_);  // recompute transform
}

void Renderer::setVirtualSize(float width, float height) {
    virtualWidth_ = width > 1.0f ? width : 1.0f;
    virtualHeight_ = height > 1.0f ? height : 1.0f;

    const float fbW = static_cast<float>(fbWidth_);
    const float fbH = static_cast<float>(fbHeight_);

    if (aspect_ == AspectMode::Stretch) {
        // Classic MUGEN: separate scales per axis, filling the screen exactly.
        pixelScale_ = 1.0f;  // unused in stretch; kept defined for the mapping below
        pixelOffsetX_ = 0.0f;
        pixelOffsetY_ = 0.0f;
        sx_ = 2.0f / virtualWidth_;
        ox_ = -1.0f;
        sy_ = -2.0f / virtualHeight_;
        oy_ = 1.0f;
        return;
    }

    // Uniform scale, letterboxed / pillarboxed. MUGEN never stretches sprites,
    // so neither do we.
    const float scale = std::fmin(fbW / virtualWidth_, fbH / virtualHeight_);
    pixelScale_ = scale;
    pixelOffsetX_ = (fbW - virtualWidth_ * scale) * 0.5f;
    pixelOffsetY_ = (fbH - virtualHeight_ * scale) * 0.5f;

    sx_ = (2.0f * scale) / fbW;
    ox_ = (2.0f * pixelOffsetX_) / fbW - 1.0f;

    // Screen Y grows downward; clip-space Y grows upward.
    sy_ = (-2.0f * scale) / fbH;
    oy_ = 1.0f - (2.0f * pixelOffsetY_) / fbH;
}

void Renderer::setAspectMode(AspectMode mode) {
    if (aspect_ == mode) return;
    aspect_ = mode;
    setVirtualSize(virtualWidth_, virtualHeight_);
}

void Renderer::framebufferToVirtual(float fbX, float fbY, float* outX, float* outY) const {
    if (aspect_ == AspectMode::Stretch) {
        if (outX != nullptr) *outX = fbX * virtualWidth_ / static_cast<float>(fbWidth_);
        if (outY != nullptr) *outY = fbY * virtualHeight_ / static_cast<float>(fbHeight_);
        return;
    }
    const float scale = pixelScale_ > 0.0f ? pixelScale_ : 1.0f;
    if (outX != nullptr) *outX = (fbX - pixelOffsetX_) / scale;
    if (outY != nullptr) *outY = (fbY - pixelOffsetY_) / scale;
}

void Renderer::beginFrame(Color clear) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_SCISSOR_TEST);

    // The blend cache is part of the batch, and the batch does not survive a
    // frame boundary -- so the cached state has to be re-armed here, or the first
    // draw of the next frame is compared against a state that no longer exists
    // on the GL side. `BlendState{}` is exactly what the three calls above set.
    currentBlendState_ = BlendState{};

    glClearColor(clear.r, clear.g, clear.b, clear.a);
    glClear(GL_COLOR_BUFFER_BIT);

    // The letterbox is a coordinate transform, not a viewport, so anything
    // drawn outside the virtual rectangle would land in the bars beside the
    // game. That is not hypothetical: a background layer tiles until it covers
    // the screen, and the last tile reaches past the right edge by design. The
    // base clip is what makes "off screen" mean nothing is drawn.
    clipStack_.clear();
    clipStack_.push_back({0.0f, 0.0f, virtualWidth_, virtualHeight_});
    applyClip();

    vertices_.clear();
    currentTexture_ = 0;
    currentBlendState_ = BlendState{};
    currentPaletteOn_ = false;
    currentLut_ = 0;
    currentPalFx_ = PalFxParams{};
    // Nothing has set a global grade yet this frame; the fight arms one as it draws.
    globalPalFxOn_ = false;
    globalPalFx_ = PalFxParams{};

    glUseProgram(program_);
    glUniform1i(uTextureLoc_, 0);
    glUniform1i(uPaletteLoc_, 1);
    glUniform4f(uTransformLoc_, sx_, ox_, sy_, oy_);
    glActiveTexture(GL_TEXTURE0);
    // Uniform values persist on the program across frames, so a frame that
    // ended mid-palette-run would leak its grade into the next one's first
    // batch. Re-arm the neutral state explicitly.
    glUniform1i(uUsePaletteLoc_, 0);
    glUniform3f(uPalMulLoc_, 1.0f, 1.0f, 1.0f);
    glUniform3f(uPalAddLoc_, 0.0f, 0.0f, 0.0f);
    glUniform1f(uPalColorLoc_, 1.0f);
    glUniform1f(uPalInvertLoc_, 0.0f);
}

void Renderer::pushPixelSpace(float width, float height) {
    flush();
    const std::array<float, 4> base =
        clipStack_.empty() ? std::array<float, 4>{0.0f, 0.0f, virtualWidth_, virtualHeight_}
                           : clipStack_.front();
    spaces_.push_back({virtualWidth_, virtualHeight_, aspect_, sx_, ox_, sy_, oy_, pixelScale_,
                       pixelOffsetX_, pixelOffsetY_, base});

    // One unit to one pixel: the identity mapping onto the whole surface.
    virtualWidth_ = width > 0.0f ? width : 1.0f;
    virtualHeight_ = height > 0.0f ? height : 1.0f;
    aspect_ = AspectMode::Stretch;
    sx_ = 2.0f / virtualWidth_;
    ox_ = -1.0f;
    sy_ = -2.0f / virtualHeight_;
    oy_ = 1.0f;
    pixelScale_ = 1.0f;
    pixelOffsetX_ = 0.0f;
    pixelOffsetY_ = 0.0f;

    glUseProgram(program_);
    glUniform4f(uTransformLoc_, sx_, ox_, sy_, oy_);

    // The base clip is the letterboxed screen, which is the right fence for the game
    // and the wrong one for this: a control belongs under the thumb, and on a phone
    // held sideways the letterbox bars are exactly where the thumbs are. So the base
    // clip is widened to the whole surface for as long as this space lasts, and the
    // one it replaces is put back on the way out.
    if (!clipStack_.empty()) {
        clipStack_.front() = {0.0f, 0.0f, virtualWidth_, virtualHeight_};
        applyClip();
    }
}

void Renderer::popSpace() {
    if (spaces_.empty()) return;
    flush();
    const Space& space = spaces_.back();
    virtualWidth_ = space.virtualWidth;
    virtualHeight_ = space.virtualHeight;
    aspect_ = space.aspect;
    sx_ = space.sx;
    ox_ = space.ox;
    sy_ = space.sy;
    oy_ = space.oy;
    pixelScale_ = space.pixelScale;
    pixelOffsetX_ = space.pixelOffsetX;
    pixelOffsetY_ = space.pixelOffsetY;
    spaces_.pop_back();

    if (!clipStack_.empty()) {
        clipStack_.front() = space.baseClip;
        applyClip();
    }

    glUseProgram(program_);
    glUniform4f(uTransformLoc_, sx_, ox_, sy_, oy_);
}

void Renderer::pushTranslate(float dx, float dy) {
    flush();
    translates_.push_back({dx, dy});
    // The transform is `x' = x * sx + ox`, so shifting the output by `dx` virtual units is
    // a change to the offset alone -- and it moves everything already in this space, which
    // is what a shake is.
    ox_ += dx * sx_;
    oy_ += dy * sy_;
    glUseProgram(program_);
    glUniform4f(uTransformLoc_, sx_, ox_, sy_, oy_);
}

void Renderer::popTranslate() {
    if (translates_.empty()) return;
    flush();
    const std::array<float, 2> offset = translates_.back();
    translates_.pop_back();
    ox_ -= offset[0] * sx_;
    oy_ -= offset[1] * sy_;
    glUseProgram(program_);
    glUniform4f(uTransformLoc_, sx_, ox_, sy_, oy_);
}

void Renderer::pushClip(float x1, float y1, float x2, float y2) {
    clipStack_.push_back({x1, y1, x2, y2});
    applyClip();
}

void Renderer::popClip() {
    // The bottom entry is the screen itself and is not the caller's to remove.
    if (clipStack_.size() <= 1) return;
    clipStack_.pop_back();
    applyClip();
}

void Renderer::applyClip() {
    // The batch carries no scissor state, so anything queued under the old clip
    // has to reach the GPU before the new one takes effect.
    flush();

    if (clipStack_.empty()) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    // Virtual coordinates to device pixels, the exact inverse of
    // framebufferToVirtual().
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    if (aspect_ == AspectMode::Stretch) {
        scaleX = static_cast<float>(fbWidth_) / virtualWidth_;
        scaleY = static_cast<float>(fbHeight_) / virtualHeight_;
    } else {
        scaleX = scaleY = pixelScale_;
        offsetX = pixelOffsetX_;
        offsetY = pixelOffsetY_;
    }

    // Nested clips intersect. Taking only the innermost would let a caller
    // widen the clip by pushing one, which is the opposite of what a clip is
    // for -- and it would quietly undo the base clip that keeps drawing inside
    // the screen.
    std::array<float, 4> clip = clipStack_.front();
    for (size_t i = 1; i < clipStack_.size(); ++i) {
        clip[0] = std::max(clip[0], clipStack_[i][0]);
        clip[1] = std::max(clip[1], clipStack_[i][1]);
        clip[2] = std::min(clip[2], clipStack_[i][2]);
        clip[3] = std::min(clip[3], clipStack_[i][3]);
    }
    const float left = clip[0] * scaleX + offsetX;
    const float top = clip[1] * scaleY + offsetY;
    const float right = clip[2] * scaleX + offsetX;
    const float bottom = clip[3] * scaleY + offsetY;

    const int x0 = static_cast<int>(std::floor(left));
    const int x1 = static_cast<int>(std::ceil(right));
    // GL's scissor origin is the bottom-left corner, so the Y interval flips.
    const int y0 = static_cast<int>(std::floor(static_cast<float>(fbHeight_) - bottom));
    const int y1 = static_cast<int>(std::ceil(static_cast<float>(fbHeight_) - top));

    glScissor(x0, y0, x1 - x0 > 0 ? x1 - x0 : 0, y1 - y0 > 0 ? y1 - y0 : 0);
    glEnable(GL_SCISSOR_TEST);
}

void Renderer::endFrame() {
    flush();
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::applyBlend(Blend blend, float dstAlpha) {
    const BlendState state = blendStateFor(blend, dstAlpha);
    if (state == currentBlendState_) return;
    flush();
    currentBlendState_ = state;

    glBlendEquation(static_cast<GLenum>(state.equation));
    // The constant is only consulted when the factor asks for it, and only the
    // destination factor is ever constant here: MUGEN's source alpha rides the
    // sprite's own alpha channel and is set per vertex, which is why one batch
    // can hold draws that differ in it and cannot hold draws that differ in this.
    if (state.constantAlpha.has_value()) {
        glBlendColor(0.0f, 0.0f, 0.0f, *state.constantAlpha);
    }
    glBlendFunc(static_cast<GLenum>(state.source), static_cast<GLenum>(state.destination));
}

void Renderer::flush() {
    if (vertices_.empty() || currentTexture_ == 0) {
        vertices_.clear();
        return;
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                 vertices_.data(), GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentTexture_);
    if (currentPaletteOn_ && currentLut_ != 0) {
        // The LUT sits on unit 1; unit 0 stays the sprite so the RGBA path is
        // untouched.
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, currentLut_);
        glActiveTexture(GL_TEXTURE0);
    }
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));

    vertices_.clear();
}

// Which palette state a draw of `tex` wants. A caller with no palette opinion
// (palette == nullptr) gets the texture's own default: the LUT it was decoded
// against and no grade.
void Renderer::resolvePalette(const Texture& tex, const PaletteBind* palette, unsigned* lutIdOut,
                              const PalFxParams** fxOut) {
    *lutIdOut = 0;
    *fxOut = nullptr;
    if (!tex.indexed()) return;
    const Texture* lut = palette != nullptr && palette->lut != nullptr ? palette->lut
                                                                       : tex.defaultLut();
    if (lut == nullptr || !lut->valid()) return;
    *lutIdOut = lut->id();
    if (palette != nullptr && !palette->fx.identity()) *fxOut = &palette->fx;
}

// The grade produced by applying `inner` and then `outer`. The shader runs one grade
// per draw, so two grades that both want to land on the same sprite have to be folded
// into one pair of uniforms, and that fold is exact only in what the two share:
//
//   * mul composes as a product -- the gain is applied twice, so it multiplies.
//   * add composes as a sum -- both offsets land on the colour, so they add. This is
//     the term that is approximate when both are non-identity, because the outer pass
//     would multiply the inner add by the outer mul and this merge does not.
//   * color and invert compose as they read: two desaturations multiply, and a pair of
//     inversions cancel.
//
// Every one of these collapses to the surviving grade when the other is the identity,
// which is why a single global grade on a sprite with no grade of its own is exact.
PalFxParams Renderer::composePalFx(const PalFxParams& inner, const PalFxParams& outer) {
    PalFxParams out;
    for (int c = 0; c < 3; ++c) {
        out.mul[c] = inner.mul[c] * outer.mul[c];
        out.add[c] = inner.add[c] + outer.add[c];
    }
    out.color = inner.color * outer.color;
    out.invert = inner.invert != outer.invert;
    return out;
}

void Renderer::applyPaletteState(unsigned lutId, const PalFxParams* fx) {
    const bool on = lutId != 0;
    PalFxParams wanted = fx != nullptr ? *fx : PalFxParams{};
    // The fight's global grade (BGPalFX/AllPalFX) rides on top of whatever grade the
    // sprite already carries. Two PalFX passes cannot be folded into one exactly -- the
    // greyscale blend and the invert do not commute with the add and the multiply -- so
    // the terms are merged component-wise, which is exact whenever either grade is the
    // identity. That is nearly always the case in practice: the global grade is the only
    // one in play when it is set, and a fighter's own flash is the only one otherwise.
    if (on && globalPalFxOn_) wanted = composePalFx(wanted, globalPalFx_);
    if (on == currentPaletteOn_ && (!on || (lutId == currentLut_ && wanted == currentPalFx_))) {
        return;
    }

    // Same rule as a texture change: the batch carries no palette state, so
    // everything queued under the old one reaches the GPU first.
    flush();
    currentPaletteOn_ = on;
    currentLut_ = lutId;
    currentPalFx_ = wanted;

    glUniform1i(uUsePaletteLoc_, on ? 1 : 0);
    if (on) {
        glUniform3f(uPalMulLoc_, wanted.mul[0], wanted.mul[1], wanted.mul[2]);
        glUniform3f(uPalAddLoc_, wanted.add[0], wanted.add[1], wanted.add[2]);
        glUniform1f(uPalColorLoc_, wanted.color);
        glUniform1f(uPalInvertLoc_, wanted.invert ? 1.0f : 0.0f);
    }
}

void Renderer::pushQuad(const Texture& tex, float x, float y, float w, float h, const Rect& src,
                        Color tint, bool flipX, bool flipY, unsigned lutId,
                        const PalFxParams* fx) {
    if (!tex.valid() || w == 0.0f || h == 0.0f) return;

    // Changing texture, blend state or palette state forces a flush; the batch
    // is otherwise one draw call for the whole HUD.
    //
    // The blend is not re-armed here: every caller of this method arms it first,
    // through `applyBlend`, which flushes when the state is not the one already
    // set -- so by the time a quad is pushed the batch is on the right blender
    // and this method only has the texture and the palette left to check.
    if (tex.id() != currentTexture_) {
        flush();
        currentTexture_ = tex.id();
    }
    applyPaletteState(lutId, fx);

    const float texW = static_cast<float>(tex.width());
    const float texH = static_cast<float>(tex.height());

    float u0 = src.x / texW;
    float v0 = src.y / texH;
    float u1 = (src.x + src.w) / texW;
    float v1 = (src.y + src.h) / texH;

    if (flipX) std::swap(u0, u1);
    if (flipY) std::swap(v0, v1);

    const float x1 = x + w;
    const float y1 = y + h;
    const float x0 = x;
    const float y0 = y;

    auto push = [&](float px, float py, float pu, float pv) {
        vertices_.push_back(Vertex{px, py, pu, pv, tint.r, tint.g, tint.b, tint.a});
    };

    // Two triangles, wound consistently.
    push(x0, y0, u0, v0);
    push(x1, y0, u1, v0);
    push(x1, y1, u1, v1);

    push(x0, y0, u0, v0);
    push(x1, y1, u1, v1);
    push(x0, y1, u0, v1);
}

void Renderer::fillRect(float x, float y, float w, float h, Color color, Blend blend) {
    applyBlend(blend, 1.0f);
    pushQuad(white_, x, y, w, h, Rect{0, 0, 1, 1}, color, false, false);
}

void Renderer::fillQuad(const Quad& quad, Color color, Blend blend) {
    if (!white_.valid()) return;

    applyBlend(blend, 1.0f);
    if (white_.id() != currentTexture_) {
        flush();
        currentTexture_ = white_.id();
    }
    // fillQuad bypasses pushQuad, so it re-arms the RGBA path itself: a solid
    // quad drawn after a fighter's palette run must not inherit its LUT.
    applyPaletteState(0, nullptr);

    // A 1x1 white texture, so every corner samples the same texel and only the
    // positions differ. Wound the same way round as pushQuad's two triangles:
    // 0-1-2 then 0-2-3.
    auto push = [&](int index) {
        vertices_.push_back(
            Vertex{quad.x[index], quad.y[index], 0.5f, 0.5f, color.r, color.g, color.b, color.a});
    };
    push(0);
    push(1);
    push(2);
    push(0);
    push(2);
    push(3);
}

void Renderer::fillShearedBar(float x, float y, float w, float h, float shear, Color color,
                              Blend blend) {
    Quad quad;
    quad.x[0] = x + shear;
    quad.y[0] = y;
    quad.x[1] = x + shear + w;
    quad.y[1] = y;
    quad.x[2] = x + w;
    quad.y[2] = y + h;
    quad.x[3] = x;
    quad.y[3] = y + h;
    fillQuad(quad, color, blend);
}

void Renderer::drawSprite(const Texture& tex, float x, float y, float w, float h, const Rect& src,
                          Color tint, bool flipX, bool flipY, TransParams trans,
                          const PaletteBind* palette) {
    applyBlend(trans.blend, trans.dstAlpha);
    // MUGEN's source alpha and the sprite's own alpha are one product, and the
    // shader has them in one place: `fragColor = texel * vColor`, so the vertex
    // alpha is what `GL_SRC_ALPHA` ends up reading.
    tint = transTint(tint, trans);
    unsigned lutId = 0;
    const PalFxParams* fx = nullptr;
    resolvePalette(tex, palette, &lutId, &fx);
    pushQuad(tex, x, y, w, h, src, tint, flipX, flipY, lutId, fx);
}

void Renderer::drawSpriteRotated(const Texture& tex, float pivotX, float pivotY, float x, float y,
                                 float w, float h, const Rect& src, float degrees, Color tint,
                                 bool flipX, bool flipY, TransParams trans,
                                 const PaletteBind* palette) {
    if (!tex.valid() || w == 0.0f || h == 0.0f) return;
    if (std::fmod(std::fabs(degrees), 360.0f) < 0.01f) {
        drawSprite(tex, x, y, w, h, src, tint, flipX, flipY, trans, palette);
        return;
    }

    const float radians = degrees * 0.0174532925f;
    const float sn = std::sin(radians);
    const float cs = std::cos(radians);
    auto rotate = [&](float px, float py, float* outX, float* outY) {
        const float dx = px - pivotX;
        const float dy = py - pivotY;
        *outX = pivotX + dx * cs - dy * sn;
        *outY = pivotY + dx * sn + dy * cs;
    };

    applyBlend(trans.blend, trans.dstAlpha);
    tint = transTint(tint, trans);
    if (tex.id() != currentTexture_) {
        flush();
        currentTexture_ = tex.id();
    }
    unsigned lutId = 0;
    const PalFxParams* fx = nullptr;
    resolvePalette(tex, palette, &lutId, &fx);
    applyPaletteState(lutId, fx);

    const float texW = static_cast<float>(tex.width());
    const float texH = static_cast<float>(tex.height());
    float u0 = src.x / texW;
    float v0 = src.y / texH;
    float u1 = (src.x + src.w) / texW;
    float v1 = (src.y + src.h) / texH;
    if (flipX) std::swap(u0, u1);
    if (flipY) std::swap(v0, v1);

    float corners[4][2];
    rotate(x, y, &corners[0][0], &corners[0][1]);
    rotate(x + w, y, &corners[1][0], &corners[1][1]);
    rotate(x + w, y + h, &corners[2][0], &corners[2][1]);
    rotate(x, y + h, &corners[3][0], &corners[3][1]);

    const float us[4] = {u0, u1, u1, u0};
    const float vs[4] = {v0, v0, v1, v1};
    auto push = [&](int index) {
        vertices_.push_back(Vertex{corners[index][0], corners[index][1], us[index], vs[index],
                                   tint.r, tint.g, tint.b, tint.a});
    };
    push(0);
    push(1);
    push(2);
    push(0);
    push(2);
    push(3);
}

void Renderer::drawSprite(const Texture& tex, float x, float y, const Rect& src, Color tint,
                          bool flipX, bool flipY, TransParams trans,
                          const PaletteBind* palette) {
    const float w = src.w != 0 ? src.w : static_cast<float>(tex.width());
    const float h = src.h != 0 ? src.h : static_cast<float>(tex.height());
    drawSprite(tex, x, y, w, h, src, tint, flipX, flipY, trans, palette);
}

}  // namespace af
