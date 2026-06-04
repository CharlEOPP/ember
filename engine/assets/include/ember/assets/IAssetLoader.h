#pragma once
#include "ember/core/Types.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace ember {

// Import settings parsed from an asset's `.meta` sidecar (e.g. filter, wrap).
// Stored as strings; loaders interpret the keys they care about.
struct AssetSettings {
    std::unordered_map<std::string, std::string> kv;

    std::string get(const std::string& key, const std::string& fallback = {}) const {
        auto it = kv.find(key);
        return it == kv.end() ? fallback : it->second;
    }
    bool getBool(const std::string& key, bool fallback = false) const {
        auto it = kv.find(key);
        if (it == kv.end()) return fallback;
        return it->second == "true" || it->second == "1";
    }
};

// Typed loader contract. Loaders produce a shared_ptr<T> from a real filesystem
// path + settings, and may release any non-RAII resources in unload().
//
// Async (optional, two-phase): because GPU uploads must happen on the main
// thread, loaders that touch the GPU split work into loadCPU() (runs on a
// worker: file I/O + CPU decode, returns an opaque payload) and finalize()
// (runs on the main thread: e.g. GL upload). Loaders that don't override these
// report supportsAsync() == false and AssetManager falls back to sync load().
template<typename T>
struct IAssetLoader {
    virtual ~IAssetLoader() = default;
    virtual std::shared_ptr<T> load(const std::filesystem::path& path,
                                    const AssetSettings& settings) = 0;
    virtual void unload(T& /*asset*/) {}

    virtual bool supportsAsync() const { return false; }
    virtual std::shared_ptr<void> loadCPU(const std::filesystem::path& /*path*/,
                                          const AssetSettings& /*settings*/) { return nullptr; }
    virtual std::shared_ptr<T> finalize(const std::shared_ptr<void>& /*cpuPayload*/) { return nullptr; }
};

} // namespace ember
