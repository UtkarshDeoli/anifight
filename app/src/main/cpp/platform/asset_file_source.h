// Reads the game's data folder out of the APK.
//
// Everything under `assets/mugen/` is stored uncompressed (see the
// `androidResources.noCompress` list in build.gradle.kts) so that the engine can
// map it rather than inflate it, but we still read through a normal loop so the
// engine keeps working if a file ever does end up compressed.
#pragma once

#include <android/asset_manager.h>

#include <string>

#include "core/file_source.h"

namespace af {

class AssetFileSource final : public FileSource {
public:
    // `root` is the asset subdirectory holding the game, normally "mugen".
    AssetFileSource(AAssetManager* manager, std::string root);

    std::unique_ptr<FileData> read(const std::string& path) override;
    bool readRange(const std::string& path, size_t offset, size_t length,
                   std::vector<uint8_t>& out) override;
    [[nodiscard]] bool exists(const std::string& path) override;

private:
    [[nodiscard]] std::string resolve(const std::string& path) const;

    AAssetManager* manager_ = nullptr;
    std::string root_;
};

}  // namespace af
