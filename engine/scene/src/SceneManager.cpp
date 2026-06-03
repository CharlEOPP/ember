#include "ember/scene/SceneManager.h"
#include "ember/events/EventBus.h"
#include "ember/events/Events.h"
#include "ember/core/Assert.h"

#include <algorithm>
#include <utility>

namespace ember {

static std::string sceneNameFromPath(const std::filesystem::path& p) {
    return p.stem().string();
}

Result<void, std::string> SceneManager::load(const std::filesystem::path& path) {
    if (!m_loader) return Err(std::string("SceneManager: no loader installed"));

    auto scene = std::make_unique<Scene>(sceneNameFromPath(path));
    auto result = m_loader(path, *scene);
    if (!result) return result;   // active set untouched on failure (SCM-06)

    m_scenes.clear();
    m_active = scene.get();
    m_scenes.push_back(std::move(scene));
    m_bus.post<SceneLoadedEvent>(m_active->name());
    return {};
}

Result<void, std::string> SceneManager::loadAdditive(const std::filesystem::path& path) {
    if (!m_loader) return Err(std::string("SceneManager: no loader installed"));

    auto scene = std::make_unique<Scene>(sceneNameFromPath(path));
    auto result = m_loader(path, *scene);
    if (!result) return result;

    const std::string nm = scene->name();
    if (!m_active) m_active = scene.get();
    m_scenes.push_back(std::move(scene));
    m_bus.post<SceneLoadedEvent>(nm);
    return {};
}

void SceneManager::unload(std::string_view name) {
    auto it = std::find_if(m_scenes.begin(), m_scenes.end(),
                           [&](const std::unique_ptr<Scene>& s) { return s->name() == name; });
    if (it == m_scenes.end()) return;

    const bool wasActive = (it->get() == m_active);
    const std::string nm = (*it)->name();
    m_scenes.erase(it);
    if (wasActive) m_active = m_scenes.empty() ? nullptr : m_scenes.back().get();
    m_bus.post<SceneUnloadedEvent>(nm);
}

Scene& SceneManager::getActive() {
    EMBER_ASSERT(m_active != nullptr, "SceneManager::getActive(): no active scene");
    return *m_active;
}

void SceneManager::endFrame() {
    if (!m_pending) return;
    const auto path = *m_pending;
    m_pending.reset();
    (void)load(path);
}

} // namespace ember
