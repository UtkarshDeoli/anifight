#include "engine/screens/storyboard_screen.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

#include "core/file_source.h"
#include "core/log.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "gfx/font.h"

namespace af {

namespace {

std::string directoryOf(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

// First candidate that exists. An empty result means every one missed.
std::string resolveExisting(FileSource& files, const std::vector<std::string>& candidates) {
    for (const std::string& candidate : candidates) {
        if (!candidate.empty() && files.exists(candidate)) return candidate;
    }
    return {};
}

std::string resolveStoryboard(FileSource& files, const Game& game, const std::string& ref) {
    const std::string clean = normaliseFileRef(ref);
    if (clean.empty()) return {};
    if (clean.front() == '/') return files.exists(clean) ? clean : std::string{};
    // The reference's own search puts a storyboard in the screenpack directory
    // first; the KFM motif's `gameover.def` sits beside `system.def` and names
    // `spr = gameover.sff` the same way. The other two cover a path written
    // relative to MUGEN's data root instead.
    return resolveExisting(files, {game.root() + clean, "data/" + clean, clean});
}

// Storyboard assets resolve against the storyboard's own directory first, then
// the screenpack directory, then the data root -- the same order the reference
// uses for a storyboard's `spr` and `snd`.
std::string resolveAsset(FileSource& files, const Game& game, const std::string& dir,
                         const std::string& ref) {
    const std::string clean = normaliseFileRef(ref);
    if (clean.empty()) return {};
    if (clean.front() == '/') return files.exists(clean) ? clean : std::string{};
    return resolveExisting(files, {dir + clean, game.root() + clean, "data/" + clean, clean});
}

// Fonts have one extra documented fallback: the reference searches the
// storyboard's directory and then a `font/` folder. The data root is
// configurable, so the same folder is tried under both the screenpack root and
// the literal data root before giving up.
std::string resolveFont(FileSource& files, const Game& game, const std::string& dir,
                        const std::string& ref) {
    const std::string clean = normaliseFileRef(ref);
    if (clean.empty()) return {};
    if (clean.front() == '/') return files.exists(clean) ? clean : std::string{};
    return resolveExisting(files,
                           {dir + clean, game.root() + "font/" + clean, "data/font/" + clean,
                            "font/" + clean, clean});
}

// MUGEN's storyboard font alignment is 1 left, 0 centre, -1 right. That is the
// opposite sense from `textAlignFromInt`, which exists for screenpack elements;
// storyboard text is mapped here so the two conventions do not contaminate each
// other.
TextAlign storyboardAlign(int align) {
    if (align > 0) return TextAlign::Left;
    if (align < 0) return TextAlign::Right;
    return TextAlign::Center;
}

// `layerN.text` is documented to break lines with a literal `\n`; the parsed
// string already has real newlines. Font::draw handles one line, so this walks
// them and advances by the font's line height, scaled on the vertical axis.
void drawStoryboardText(Renderer& renderer, const Font& font, const std::string& text, float x,
                        float y, int bank, int align, Color color, Vec2 scale) {
    if (text.empty() || scale.x <= 0.0f) return;
    const TextAlign anchor = storyboardAlign(align);
    const float lineStep = static_cast<float>(font.lineHeight()) * std::max(0.0f, scale.y);

    size_t begin = 0;
    float lineTop = y;
    while (true) {
        const size_t end = text.find('\n', begin);
        const size_t length = (end == std::string::npos ? text.size() : end) - begin;
        if (length > 0) {
            font.draw(renderer, x, lineTop, std::string_view(text).substr(begin, length), bank,
                      anchor, color, scale.x);
        }
        if (end == std::string::npos) break;
        begin = end + 1;
        lineTop += lineStep;
    }
}

// Storyboard sound banks live for the process, not for the screen.
//
// `Mixer::playFrom` registers the bank by raw pointer and never removes it, and
// the audio thread reads that registry for as long as the engine runs. A bank
// owned by StoryboardScreen would therefore dangle the moment the scene popped
// and a later character sound searched the list. Screenpack and character banks
// do not have this problem because Game owns them for the app's lifetime.
//
// Caching by resolved path gives storyboard banks the same lifetime and the
// same bound: a screenpack can name at most a few storyboard files, and each is
// decoded once however many times the storyboard plays.
std::shared_ptr<SndBank> sharedStoryboardSounds(FileSource& files, const std::string& path,
                                               std::string* errorOut) {
    static std::map<std::string, std::shared_ptr<SndBank>> cache;

    const auto found = cache.find(path);
    if (found != cache.end()) return found->second;

    std::unique_ptr<FileData> data = files.read(path);
    if (!data) {
        if (errorOut != nullptr) *errorOut = "file not found";
        return nullptr;
    }

    auto bank = std::make_shared<SndBank>();
    if (!bank->load(*data, errorOut)) return nullptr;
    cache.emplace(path, bank);
    return bank;
}

}  // namespace

StoryboardScreen::StoryboardScreen(FileSource& files, Game& game, std::string storyboardPath)
    : game_(game) {
    load(files, game, storyboardPath);
}

void StoryboardScreen::load(FileSource& files, Game& game, const std::string& storyboardPath) {
    path_ = resolveStoryboard(files, game, storyboardPath);
    if (path_.empty()) {
        error_ = "storyboard not found: " + normaliseFileRef(storyboardPath);
        AF_LOGW("storyboard: %s", error_.c_str());
        return;
    }

    std::unique_ptr<FileData> data = files.read(path_);
    if (!data || data->empty()) {
        error_ = "storyboard unreadable: " + path_;
        AF_LOGW("storyboard: %s", error_.c_str());
        return;
    }

    const std::string_view text(reinterpret_cast<const char*>(data->data()), data->size());
    def_ = parseStoryboard(IniFile::parse(text));
    if (def_.empty()) {
        error_ = "storyboard has no [Scene] sections: " + path_;
        AF_LOGW("storyboard: %s", error_.c_str());
        return;
    }

    timing_ = StoryboardTiming::fromStoryboard(def_);
    // `startscene` is documented as a debugging aid, not a normal playback
    // mode, but honouring it is exactly what the reference does: start the
    // clock at that scene instead of at the beginning.
    timing_.startAtScene(static_cast<size_t>(std::max(0, def_.startScene)));

    const std::string dir = directoryOf(path_);

    if (!def_.spriteFile.empty()) {
        const std::string spritePath = resolveAsset(files, game, dir, def_.spriteFile);
        std::string error;
        if (spritePath.empty() || !sprites_.load(files, spritePath, &error)) {
            // A storyboard made only of text is legal; missing art is a
            // presentation failure, not a reason to refuse the whole screen.
            AF_LOGW("storyboard: sprites \"%s\" not loaded%s%s", def_.spriteFile.c_str(),
                    error.empty() ? "" : ": ", error.c_str());
        }
    }

    if (!def_.soundFile.empty()) {
        const std::string soundPath = resolveAsset(files, game, dir, def_.soundFile);
        std::string error;
        if (soundPath.empty()) {
            error = "file not found";
        } else {
            sounds_ = sharedStoryboardSounds(files, soundPath, &error);
        }
        if (!sounds_) {
            AF_LOGW("storyboard: sounds \"%s\" not loaded%s%s", def_.soundFile.c_str(),
                    error.empty() ? "" : ": ", error.c_str());
        }
    }

    for (int i = 0; i < 10; ++i) {
        const std::string& ref = def_.fontFiles[static_cast<size_t>(i)];
        if (ref.empty()) continue;
        const std::string fontPath = resolveFont(files, game, dir, ref);
        if (fontPath.empty()) {
            AF_LOGW("storyboard: font%d \"%s\" not found", i, ref.c_str());
            continue;
        }
        auto font = std::make_unique<Font>();
        std::string error;
        if (!font->load(files, fontPath, &error)) {
            AF_LOGW("storyboard: font%d \"%s\" not loaded: %s", i, fontPath.c_str(),
                    error.c_str());
            continue;
        }
        fonts_[static_cast<size_t>(i)] = std::move(font);
    }

    ready_ = true;
}

void StoryboardScreen::enter() {
    if (!ready_) return;

    if (engine() != nullptr) {
        const Vec2 local = def_.localCoord;
        if (local.x > 0.0f && local.y > 0.0f) {
            engine()->renderer().setVirtualSize(local.x, local.y);
            engine()->renderer().setAspectMode(game_.config().aspect);
        }
    }

    AF_LOGI("storyboard: \"%s\" -- %zu scene(s), %lld ticks, localcoord %.0fx%.0f", path_.c_str(),
            def_.scenes.size(), timing_.totalTicks(), def_.localCoord.x, def_.localCoord.y);

    // Report the documented features this player deliberately does not render,
    // once per storyboard. The scan is over every scene, because a `bgm` that
    // starts in scene 3 is just as unsupported as one in scene 0.
    bool hasBgm = false;
    bool hasBackground = false;
    // Volume and pan now reach the mixer. The one request still not honoured is
    // a `volumescale` above 100: the reference says storyboard volume cannot be
    // raised past the authored level, so that is clamped rather than amplified.
    bool hasTooLoudVolume = false;
    for (const StoryboardScene& scene : def_.scenes) {
        if (!scene.bgm.empty()) hasBgm = true;
        if (!scene.bgName.empty()) hasBackground = true;
        for (const StoryboardSound& sound : scene.sounds) {
            if (sound.present && sound.volumeScale > 100.0f + 1e-4f) hasTooLoudVolume = true;
        }
    }

    if (hasBgm && !bgmReported_) {
        bgmReported_ = true;
        AF_LOGW("storyboard: bgm is not playable -- this engine has no music decoder; "
                "continuing silently");
    }
    if (hasBackground && !backgroundReported_) {
        backgroundReported_ = true;
        AF_LOGW("storyboard: bg.name background objects are not supported; drawing layers only");
    }
    if (hasTooLoudVolume && !audioLimitsReported_) {
        audioLimitsReported_ = true;
        AF_LOGW("storyboard: sound volumescale above 100 is not supported by MUGEN storyboards; "
                "playing at authored level");
    }
}

void StoryboardScreen::update(double dt, const InputState& input) {
    (void)dt;
    (void)input;
    if (!ready_ || popping_) return;

    timing_.advance();
    if (timing_.finished()) {
        finish();
        return;
    }

    const size_t sceneIndex = timing_.sceneAt(timing_.tick());
    if (sceneIndex != currentScene_) {
        currentScene_ = sceneIndex;
        soundStarted_.fill(false);
    }
    playDueSounds(sceneIndex, timing_.sceneLocalTick(timing_.tick()));
}

void StoryboardScreen::playDueSounds(size_t sceneIndex, long long localTick) {
    const StoryboardScene& scene = def_.scenes[sceneIndex];
    for (size_t i = 0; i < scene.sounds.size(); ++i) {
        const StoryboardSound& sound = scene.sounds[i];
        if (!sound.present || soundStarted_[i]) continue;
        if (localTick < sound.startTime) continue;

        // Marked even when it cannot be played: a broken sample must not be
        // retried every tick for the rest of the scene.
        soundStarted_[i] = true;

        if (!sounds_) continue;
        const SoundSample* sample = sounds_->find(sound.group, sound.sample);
        if (sample == nullptr) {
            if (!missingSoundReported_) {
                missingSoundReported_ = true;
                AF_LOGW("storyboard: sound %d,%d is not in \"%s\"", sound.group, sound.sample,
                        def_.soundFile.c_str());
            }
            continue;
        }

        // The bank is process-lived (see sharedStoryboardSounds), so both the
        // bank pointer the mixer registers and the PCM a voice points into stay
        // valid after this screen pops.
        //
        // `playFromFull` is the bank-scoped full command: the sound must come
        // from this storyboard's own .snd (group 0 sample 0 would otherwise mean
        // a menu blip in the screenpack), while volume and pan ride along with
        // it. Storyboard volume is a percentage of the authored level and pan is
        // documented as -127..127, so both are converted to the mixer's units.
        if (engine() != nullptr) {
            SndCommand command;
            command.group = sound.group;
            command.sample = sound.sample;
            command.volume = volumeScaleToGain(sound.volumeScale);
            command.pan = storyboardPanToMixer(sound.pan);
            engine()->mixer().playFromFull(*sounds_, command);
        }
    }
}

void StoryboardScreen::finish() {
    if (popping_) return;
    popping_ = true;
    AF_LOGI("storyboard: finished after %lld ticks", timing_.tick());
    if (engine() != nullptr) engine()->popScreen();
}

Font* StoryboardScreen::fontFor(int index) const {
    if (index < 0 || index >= static_cast<int>(fonts_.size())) return nullptr;
    Font* font = fonts_[static_cast<size_t>(index)].get();
    return font != nullptr && font->loaded() ? font : nullptr;
}

void StoryboardScreen::draw(Renderer& renderer) {
    if (!ready_ || def_.scenes.empty()) return;

    const size_t sceneIndex = timing_.sceneAt(timing_.tick());
    const StoryboardScene& scene = def_.scenes[sceneIndex];
    const long long localTick = timing_.sceneLocalTick(timing_.tick());
    const float width = renderer.virtualWidth();
    const float height = renderer.virtualHeight();

    // `clearcolor = -1` is MUGEN's "do not clear". Engine clears black before
    // every screen, so the closest honest reading here is a black backdrop:
    // preserving the previous frame would require a framebuffer this stack does
    // not expose. Every other clear colour is drawn as written.
    if (scene.clearScreen) {
        renderer.fillRect(0.0f, 0.0f, width, height, scene.clearColor);
    }

    // Layers are drawn low number to high, which the reference defines as
    // bottom to top.
    for (size_t i = 0; i < scene.layers.size(); ++i) {
        const StoryboardLayer& layer = scene.layers[i];
        if (!layer.present) continue;
        if (localTick < layer.startTime) continue;
        if (layer.endTime >= 0 && localTick >= layer.endTime) continue;

        const float x = scene.layerAllPos.x + layer.offset.x;
        const float y = scene.layerAllPos.y + layer.offset.y;

        if (layer.anim >= 0) {
            const Animation* animation = def_.animations.find(layer.anim);
            if (animation != nullptr && !animation->empty()) {
                const long long animTick = localTick - layer.startTime;
                const AnimFrame frame = animation->frameDrawn(animTick);
                if (!frame.blank()) {
                    drawSpriteAt(renderer, sprites_, frame.group, frame.image, x + frame.offsetX,
                                 y + frame.offsetY, 1,
                                 layer.scale.x * frame.scaleX,
                                 layer.scale.y * frame.scaleY, Color{}, frame.trans(), nullptr,
                                 frame.flipV);
                }
            }
        }

        if (layer.hasText) {
            Font* font = fontFor(layer.font);
            if (font == nullptr) continue;

            std::string text = layer.text;
            if (layer.textDelay > 0) {
                // One character at starttime, then one every `textdelay`
                // ticks. Spaces and newlines count, as they do in the
                // reference's reveal.
                const long long elapsed = localTick - layer.startTime;
                const size_t visible =
                    1 + static_cast<size_t>(elapsed / layer.textDelay);
                if (visible < text.size()) text.resize(visible);
            }
            drawStoryboardText(renderer, *font, text, x, y, layer.bank, layer.align, layer.color,
                               layer.scale);
        }
    }

    // The fade is one whole-screen rectangle over the finished scene. It uses
    // the fade-out colour only once that ramp is the stronger of the two, which
    // is what makes a white-in/black-out scene read the way its file says.
    const StoryboardTiming::Fade fade = timing_.fadeAt(timing_.tick());
    if (fade.alpha > 0.0f) {
        Color colour = fade.useFadeOutColor ? scene.fadeOutColor : scene.fadeInColor;
        colour.a = fade.alpha;
        renderer.fillRect(0.0f, 0.0f, width, height, colour);
    }
}

}  // namespace af
