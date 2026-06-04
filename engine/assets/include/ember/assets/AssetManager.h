#pragma once
#include "ember/core/Types.h"
#include "ember/core/Log.h"
#include "ember/assets/AssetHandle.h"
#include "ember/assets/IAssetLoader.h"
#include "ember/assets/ThreadPool.h"
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ember {

class AssetDatabase;

// Owns all live asset data. Assets are loaded by virtual path, cached, and
// reference counted; an asset is evicted when its refcount hits zero unless
// pinned. Loaders are registered per type.
//
// Threading: the cache is mutex-guarded. `loadAsync<T>` runs the loader's CPU
// phase on a worker thread and defers the GPU phase to `processPendingUploads()`
// (call once per frame on the main/GL thread). `get<T>` returns nullptr until
// the data is resident.
class AssetManager {
public:
    explicit AssetManager(AssetDatabase* db = nullptr) : m_db(db) {}

    template<typename T>
    void registerLoader(std::shared_ptr<IAssetLoader<T>> loader) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_loaders[std::type_index(typeid(T))] = std::move(loader);
    }

    // Synchronous load. Cached (type, path) ⇒ bump refcount and return; else run
    // the loader on the calling thread (assumed main/GL thread).
    template<typename T>
    AssetHandle<T> load(const std::string& virtualPath) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (auto it = m_byKey.find(makeKey<T>(virtualPath)); it != m_byKey.end()) {
                m_entries.at(it->second).refcount++;
                return AssetHandle<T>{ it->second };
            }
        }
        auto loader = findLoader<T>();
        if (!loader) {
            EMBER_LOG_ERROR("AssetManager: no loader registered for type '{}'", typeid(T).name());
            return AssetHandle<T>::null();
        }
        std::shared_ptr<T> data = loader->load(resolveReal(virtualPath), resolveSettings(virtualPath));
        if (!data) {
            EMBER_LOG_ERROR("AssetManager: loader failed for '{}'", virtualPath);
            return AssetHandle<T>::null();
        }
        return storeEntry<T>(virtualPath, std::move(data), loader);
    }

    // Asynchronous load. The returned future resolves once the asset is resident
    // — which, for GPU assets, happens inside processPendingUploads() on the main
    // thread. Loaders without async support fall back to a synchronous load.
    template<typename T>
    std::future<AssetHandle<T>> loadAsync(const std::string& virtualPath) {
        auto prom = std::make_shared<std::promise<AssetHandle<T>>>();
        std::future<AssetHandle<T>> fut = prom->get_future();

        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (auto it = m_byKey.find(makeKey<T>(virtualPath)); it != m_byKey.end()) {
                m_entries.at(it->second).refcount++;
                prom->set_value(AssetHandle<T>{ it->second });
                return fut;
            }
        }
        auto loader = findLoader<T>();
        if (!loader) {
            EMBER_LOG_ERROR("AssetManager: no loader registered for type '{}'", typeid(T).name());
            prom->set_value(AssetHandle<T>::null());
            return fut;
        }
        if (!loader->supportsAsync()) {
            prom->set_value(load<T>(virtualPath));   // synchronous fallback
            return fut;
        }

        const auto real = resolveReal(virtualPath);
        const AssetSettings settings = resolveSettings(virtualPath);
        m_pool.enqueue([this, virtualPath, real, settings, loader, prom] {
            std::shared_ptr<void> cpu = loader->loadCPU(real, settings);   // worker thread
            std::lock_guard<std::mutex> lk(m_uploadMutex);                 // queue GPU step
            m_pendingUploads.push_back([this, virtualPath, loader, cpu, prom] {
                std::shared_ptr<T> data = cpu ? loader->finalize(cpu) : nullptr;
                if (!data) { prom->set_value(AssetHandle<T>::null()); return; }
                prom->set_value(storeEntry<T>(virtualPath, std::move(data), loader));
            });
        });
        return fut;
    }

    // Run queued GPU-finalize steps. Call once per frame on the main/GL thread.
    void processPendingUploads();

    // Pointer to resident data, or nullptr if the handle is dead / wrong type /
    // not yet resident.
    template<typename T>
    T* get(AssetHandle<T> handle) {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto it = m_entries.find(handle.id);
        if (it == m_entries.end()) return nullptr;
        if (it->second.type != std::type_index(typeid(T))) return nullptr;
        return static_cast<T*>(it->second.data.get());
    }

    template<typename T> void release(AssetHandle<T> h) { release(h.id); }
    template<typename T> void pin(AssetHandle<T> h)     { pin(h.id); }
    template<typename T> void unpin(AssetHandle<T> h)   { unpin(h.id); }
    template<typename T> bool reload(AssetHandle<T> h)  { return reload(h.id); }

    // Generic (id-based) operations — also used by the file watcher (Phase 6).
    void release(u64 id);
    void pin(u64 id);
    void unpin(u64 id);
    bool reload(u64 id);

    // Reload every resident asset whose source file resolves to `realPath`
    // (used by hot reload). Returns the number reloaded.
    u32 reloadByRealPath(const std::filesystem::path& realPath);

    [[nodiscard]] usize liveCount() const;
    [[nodiscard]] u32   refCount(u64 id) const;

private:
    struct Entry {
        std::type_index type;
        std::string     path;
        u32             refcount = 0;
        bool            pinned   = false;
        std::shared_ptr<void> data;
        std::function<void(void*)> unloadFn;
        std::function<std::shared_ptr<void>(const std::filesystem::path&, const AssetSettings&)> reloadFn;

        Entry(std::type_index t, std::string p) : type(t), path(std::move(p)) {}
    };

    template<typename T>
    static std::string makeKey(const std::string& path) {
        return std::string(typeid(T).name()) + "|" + path;
    }

    template<typename T>
    std::shared_ptr<IAssetLoader<T>> findLoader() {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto it = m_loaders.find(std::type_index(typeid(T)));
        if (it == m_loaders.end()) return nullptr;
        return std::static_pointer_cast<IAssetLoader<T>>(it->second);
    }

    // Create (or, on race, reuse) a cache entry for freshly-loaded data.
    template<typename T>
    AssetHandle<T> storeEntry(const std::string& vpath, std::shared_ptr<T> data,
                              std::shared_ptr<IAssetLoader<T>> loader) {
        std::lock_guard<std::mutex> lk(m_mutex);
        const std::string key = makeKey<T>(vpath);
        if (auto it = m_byKey.find(key); it != m_byKey.end()) {
            m_entries.at(it->second).refcount++;   // another load won the race
            return AssetHandle<T>{ it->second };
        }
        const u64 id = m_nextId++;
        Entry e(std::type_index(typeid(T)), vpath);
        e.refcount = 1;
        e.data     = std::move(data);
        e.unloadFn = [loader](void* p) { loader->unload(*static_cast<T*>(p)); };
        e.reloadFn = [loader](const std::filesystem::path& p, const AssetSettings& st)
                         -> std::shared_ptr<void> { return loader->load(p, st); };
        m_entries.emplace(id, std::move(e));
        m_byKey[key] = id;
        return AssetHandle<T>{ id };
    }

    std::filesystem::path resolveReal(const std::string& virtualPath) const;
    AssetSettings         resolveSettings(const std::string& virtualPath) const;
    void                  evictLocked(u64 id);   // caller holds m_mutex

    AssetDatabase* m_db = nullptr;
    mutable std::mutex m_mutex;                 // guards the maps below
    std::unordered_map<u64, Entry>          m_entries;
    std::unordered_map<std::string, u64>    m_byKey;
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_loaders;
    u64 m_nextId = 1;

    ThreadPool m_pool;
    std::mutex m_uploadMutex;                   // guards m_pendingUploads
    std::vector<std::function<void()>> m_pendingUploads;
};

} // namespace ember
