// Phase-1 probe: what did the loader actually attach to a state?
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "engine/character.h"
#include "engine/cns.h"

namespace {
class DirectorySource : public af::FileSource {
public:
    explicit DirectorySource(std::string root) : root_(std::move(root)) {}
    std::unique_ptr<af::FileData> read(const std::string& path) override {
        std::ifstream file(root_ + "/" + path, std::ios::binary);
        if (!file) return nullptr;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        const std::string bytes = buffer.str();
        std::vector<uint8_t> data(bytes.begin(), bytes.end());
        return std::make_unique<af::FileData>(path, std::move(data));
    }
    bool exists(const std::string& path) override {
        std::ifstream file(root_ + "/" + path, std::ios::binary);
        return file.good();
    }

private:
    std::string root_;
};
}  // namespace

int main(int argc, char** argv) {
    const std::string defPath = argv[1];
    const size_t slash = defPath.find_last_of('/');
    const std::string folder = defPath.substr(0, slash);
    const std::string leaf = defPath.substr(slash + 1);

    af::Character character;
    DirectorySource files(folder);
    std::string error;
    if (!character.load(files, leaf, &error)) {
        std::printf("load failed: %s\n", error.c_str());
        return 1;
    }

    for (int n = 0; n <= 12; ++n) {
        const af::StateDef* state = character.states().find(n);
        if (state == nullptr) {
            std::printf("state %3d: absent\n", n);
            continue;
        }
        std::printf("state %3d: defined=%d hasCtrl=%d anim=%s controllers=%zu\n", n,
                    state->defined ? 1 : 0, state->hasCtrl ? 1 : 0,
                    state->hasAnim ? state->animRaw.c_str() : "(none)", state->controllers.size());
        for (size_t i = 0; i < state->controllers.size() && i < 14; ++i) {
            const af::Controller& c = state->controllers[i];
            std::printf("      [%zu] type=%-14s triggers=%zu\n", i, c.type().c_str(),
                        c.triggerCount());
        }
    }
    return 0;
}
