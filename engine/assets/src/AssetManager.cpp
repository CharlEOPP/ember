#include "ember/assets/AssetManager.h"
#include "ember/assets/AssetDatabase.h"
#include "ember/platform/FileSystem.h"

namespace ember {

void AssetManager::processPendingUploads() {
    std::vector<std::function<void()>> jobs;
    {
        std::lock_guard<std::mutex> lk(m_uploadMutex);
        jobs.swap(m_pendingUploads);
    }
    for (auto& job : jobs) job();   // each finalize locks m_mutex internally
}

void AssetManager::release(u64 id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(id);
    if (it == m_entries.end()) return;
    Entry& e = it->second;
    if (e.refcount > 0) --e.refcount;
    if (e.refcount == 0 && !e.pinned) evictLocked(id);
}

void AssetManager::pin(u64 id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(id);
    if (it != m_entries.end()) it->second.pinned = true;
}

void AssetManager::unpin(u64 id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(id);
    if (it == m_entries.end()) return;
    it->second.pinned = false;
    if (it->second.refcount == 0) evictLocked(id);
}

bool AssetManager::reload(u64 id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(id);
    if (it == m_entries.end()) return false;
    Entry& e = it->second;
    if (!e.reloadFn) return false;
    std::shared_ptr<void> fresh = e.reloadFn(resolveReal(e.path), resolveSettings(e.path));
    if (!fresh) {
        EMBER_LOG_WARN("AssetManager: reload failed for '{}', keeping previous data", e.path);
        return false;
    }
    e.data = std::move(fresh);
    EMBER_LOG_INFO("AssetManager: reloaded '{}'", e.path);
    return true;
}

u32 AssetManager::reloadByRealPath(const std::filesystem::path& realPath) {
    std::lock_guard<std::mutex> lk(m_mutex);
    u32 count = 0;
    const auto target = realPath.lexically_normal();
    for (auto& [id, e] : m_entries) {
        if (!e.reloadFn) continue;
        if (resolveReal(e.path).lexically_normal() != target) continue;
        std::shared_ptr<void> fresh = e.reloadFn(resolveReal(e.path), resolveSettings(e.path));
        if (fresh) { e.data = std::move(fresh); ++count; }
    }
    if (count) EMBER_LOG_INFO("AssetManager: hot-reloaded {} asset(s) for '{}'", count, realPath.string());
    return count;
}

u32 AssetManager::refCount(u64 id) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(id);
    return it == m_entries.end() ? 0u : it->second.refcount;
}

usize AssetManager::liveCount() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_entries.size();
}

std::string AssetManager::pathOf(u64 id) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(id);
    return it == m_entries.end() ? std::string{} : it->second.path;
}

u64 AssetManager::loadByType(std::type_index type, const std::string& path) {
    std::function<u64(AssetManager&, const std::string&)> fn;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto it = m_loadByType.find(type);
        if (it == m_loadByType.end()) {
            EMBER_LOG_WARN("AssetManager::loadByType: no loader for type '{}'", type.name());
            return 0;
        }
        fn = it->second;
    }
    return fn(*this, path);   // calls load<T> (re-locks internally)
}

void AssetManager::evictLocked(u64 id) {
    auto it = m_entries.find(id);
    if (it == m_entries.end()) return;
    Entry& e = it->second;
    if (e.unloadFn && e.data) e.unloadFn(e.data.get());
    m_byKey.erase(std::string(e.type.name()) + "|" + e.path);
    m_entries.erase(it);
}

std::filesystem::path AssetManager::resolveReal(const std::string& virtualPath) const {
    if (m_db) return m_db->getRealPath(virtualPath);
    return FileSystem::resolvePath(virtualPath);
}

AssetSettings AssetManager::resolveSettings(const std::string& virtualPath) const {
    if (m_db) {
        if (const AssetMeta* meta = m_db->getMeta(virtualPath)) return meta->settings;
    }
    return {};
}

} // namespace ember
