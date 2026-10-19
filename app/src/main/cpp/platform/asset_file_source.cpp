#include "platform/asset_file_source.h"

#include <android/asset_manager.h>

#include <sys/types.h>

#include <cstdio>
#include <utility>
#include <vector>

#include "core/log.h"

namespace af {

namespace {
// A sprite bank can be large -- some community character SFFs run past 64 MiB --
// and it is read once and released now that pixels are decoded on demand. The
// cap is only here so a corrupt or hostile file cannot exhaust memory.
constexpr long kMaxAssetBytes = 256L * 1024 * 1024;
}  // namespace

AssetFileSource::AssetFileSource(AAssetManager* manager, std::string root)
    : manager_(manager), root_(std::move(root)) {
    // Normalise "mugen/" -> "mugen".
    while (!root_.empty() && root_.back() == '/') root_.pop_back();
}

std::string AssetFileSource::resolve(const std::string& path) const {
    std::string clean = path;
    // Accept both "data/system.def" and "\\data\\system.def" style separators.
    for (char& c : clean) {
        if (c == '\\') c = '/';
    }
    while (!clean.empty() && clean.front() == '/') clean.erase(clean.begin());
    if (root_.empty()) return clean;
    return root_ + "/" + clean;
}

std::unique_ptr<FileData> AssetFileSource::read(const std::string& path) {
    if (manager_ == nullptr) return nullptr;

    const std::string full = resolve(path);
    AAsset* asset = AAssetManager_open(manager_, full.c_str(), AASSET_MODE_UNKNOWN);
    if (asset == nullptr) return nullptr;

    const long length = AAsset_getLength(asset);
    if (length < 0 || length > kMaxAssetBytes) {
        AF_LOGW("asset '%s' has implausible length %ld", full.c_str(), length);
        AAsset_close(asset);
        return nullptr;
    }

    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    size_t total = 0;
    while (total < bytes.size()) {
        const int got = AAsset_read(asset, bytes.data() + total, bytes.size() - total);
        if (got <= 0) break;
        total += static_cast<size_t>(got);
    }
    AAsset_close(asset);

    if (total != bytes.size()) {
        AF_LOGW("short read on asset '%s' (%zu of %zu bytes)", full.c_str(), total, bytes.size());
        bytes.resize(total);
    }

    return std::make_unique<FileData>(full, std::move(bytes));
}

bool AssetFileSource::readRange(const std::string& path, size_t offset, size_t length,
                                std::vector<uint8_t>& out) {
    out.clear();
    if (manager_ == nullptr) return false;

    const std::string full = resolve(path);
    AAsset* asset = AAssetManager_open(manager_, full.c_str(), AASSET_MODE_UNKNOWN);
    if (asset == nullptr) return false;

    const off_t fileLength = AAsset_getLength(asset);
    if (fileLength < 0 || offset > static_cast<size_t>(fileLength) ||
        length > static_cast<size_t>(fileLength) - offset) {
        AAsset_close(asset);
        return false;
    }

    // Uncompressed assets (all of ours -- see noCompress in build.gradle.kts)
    // seek directly. A compressed one cannot, so fall back to the whole-file
    // path rather than failing: the engine still works, it just pays for the
    // inflation.
    if (AAsset_seek(asset, static_cast<off_t>(offset), SEEK_SET) < 0) {
        AAsset_close(asset);
        std::unique_ptr<FileData> whole = read(path);
        if (!whole || offset > whole->size() || length > whole->size() - offset) return false;
        out.assign(whole->data() + offset, whole->data() + offset + length);
        return true;
    }

    out.resize(length);
    size_t total = 0;
    while (total < out.size()) {
        const int got = AAsset_read(asset, out.data() + total, out.size() - total);
        if (got <= 0) break;
        total += static_cast<size_t>(got);
    }
    AAsset_close(asset);

    if (total != out.size()) {
        AF_LOGW("short range read on asset '%s' (%zu of %zu bytes at %zu)", full.c_str(), total,
                out.size(), offset);
        out.clear();
        return false;
    }
    return true;
}

bool AssetFileSource::exists(const std::string& path) {
    if (manager_ == nullptr) return false;
    const std::string full = resolve(path);
    AAsset* asset = AAssetManager_open(manager_, full.c_str(), AASSET_MODE_UNKNOWN);
    if (asset == nullptr) return false;
    AAsset_close(asset);
    return true;
}

}  // namespace af
