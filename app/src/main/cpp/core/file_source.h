// Reading engine data from disk or from the APK's asset store.
//
// MUGEN's own files (.sff, .fnt, .def ...) are small enough that we can just
// slurp them whole. The screenpack assets total a few hundred KB, so there is
// no reason to build a streaming reader.
#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace af {

// A blob of bytes plus a name, used for every file the engine opens.
class FileData {
public:
    FileData() = default;
    FileData(std::string name, std::vector<uint8_t> bytes)
        : name_(std::move(name)), bytes_(std::move(bytes)) {}

    [[nodiscard]] const uint8_t* data() const { return bytes_.data(); }
    [[nodiscard]] size_t size() const { return bytes_.size(); }
    [[nodiscard]] bool empty() const { return bytes_.empty(); }
    [[nodiscard]] const std::string& name() const { return name_; }

private:
    std::string name_;
    std::vector<uint8_t> bytes_;
};

// A file name out of a .def, in the one spelling the rest of the engine
// understands.
//
// Character and screenpack folders are packaged by hand on every platform MUGEN
// ever ran on, so the values are messy in ways that are all *legal* in MUGEN and
// all fatal if taken literally: `sprite = "kfm.sff"` is quoted, `st =
// ASB\\killa.cns` uses Windows separators, and both are frequently padded with
// spaces. MUGEN accepts all three, which is why no file bothers not to.
[[nodiscard]] inline std::string normaliseFileRef(std::string_view value) {
    size_t begin = value.find_first_not_of(" \t\r\n");
    size_t end = value.find_last_not_of(" \t\r\n");
    if (begin == std::string_view::npos) return {};
    std::string out(value.substr(begin, end - begin + 1));
    if (out.size() >= 2 && out.front() == '"' && out.back() == '"') {
        out = out.substr(1, out.size() - 2);
    }
    for (char& c : out) {
        if (c == '\\') c = '/';
    }
    return out;
}

// Abstracts "where the game's data folder lives". On Android this is backed by
// AAssetManager; on desktop it would be a plain directory walk.
class FileSource {
public:
    virtual ~FileSource() = default;

    // `path` is relative to the game's data root, using '/' separators.
    // Returns nullptr when the file does not exist.
    virtual std::unique_ptr<FileData> read(const std::string& path) = 0;

    // Reads a window of a file without keeping the whole thing. This is how a
    // sprite's compressed block is fetched when it is first drawn: a roster of
    // character SFFs is far too large to hold resident, but one block at a
    // time is small.
    //
    // `out` is replaced with exactly the requested bytes. Returns false when
    // the file does not exist or the window is out of range. Sources that can
    // seek override this; the default reads the whole file and copies.
    virtual bool readRange(const std::string& path, size_t offset, size_t length,
                           std::vector<uint8_t>& out) {
        out.clear();
        std::unique_ptr<FileData> whole = read(path);
        if (!whole || offset >= whole->size()) return false;
        const size_t available = whole->size() - offset;
        if (length > available) return false;
        out.assign(whole->data() + offset, whole->data() + offset + length);
        return true;
    }

    [[nodiscard]] virtual bool exists(const std::string& path) = 0;
};

}  // namespace af
