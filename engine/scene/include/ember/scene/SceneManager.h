#pragma once
#include "ember/scene/Scene.h"
#include "ember/core/Result.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ember {

class EventBus;

// Owns the set of loaded scenes and the active one. Scene file loading is injected
// via setLoader() so this module does not depend on the serialization layer
// (avoids a scene <-> serialization dependency cycle).
class SceneManager {
public:
    using Loader = std::function<Result<void, std::string>(const std::filesystem::path&, Scene&)>;

    explicit SceneManager(EventBus& bus) : m_bus(bus) {}

    void setLoader(Loader loader) { m_loader = std::move(loader); }

    Result<void, std::string> load(const std::filesystem::path& path);          // replaces active set
    Result<void, std::string> loadAdditive(const std::filesystem::path& path);  // adds on top
    void unload(std::string_view name);

    [[nodiscard]] Scene& getActive();
    [[nodiscard]] bool   hasActive() const { return m_active != nullptr; }

    void transition(const std::filesystem::path& path) { m_pending = path; } // deferred to endFrame
    void endFrame();

private:
    EventBus&                            m_bus;
    Loader                               m_loader;
    std::vector<std::unique_ptr<Scene>>  m_scenes;
    Scene*                               m_active = nullptr;
    std::optional<std::filesystem::path> m_pending;
};

} // namespace ember
