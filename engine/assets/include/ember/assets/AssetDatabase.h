#pragma once
#include "ember/core/Types.h"
#include "ember/core/UUID.h"
#include "ember/assets/IAssetLoader.h"
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace ember {

// Per-asset metadata, mirrored to a `.meta` YAML sidecar next to the asset.
struct AssetMeta {
    UUID          uuid = NULL_UUID;
    std::string   type;       // e.g. "Texture2D"
    std::string   importer;   // e.g. "Texture2DLoader"
    AssetSettings settings;
};

// Maps virtual paths (e.g. "textures/player.png") to real filesystem paths and
// `.meta` data, and lets assets be referenced by UUID for stable serialization.
class AssetDatabase {
public:
    AssetDatabase() = default;

    // Walk `root` recursively; for each asset file (non-.meta) ensure a `.meta`
    // sidecar exists (creating one with a fresh UUID if missing) and register
    // the virtual-path / UUID mappings. `root` becomes the virtual-path base.
    void scan(const std::filesystem::path& root);

    // Real filesystem path for a virtual path. Falls back to resolving against
    // the scanned root (or the raw value) when unknown.
    [[nodiscard]] std::filesystem::path getRealPath(const std::string& virtualPath) const;

    [[nodiscard]] std::optional<std::filesystem::path> getPath(UUID uuid) const;
    [[nodiscard]] const AssetMeta* getMeta(const std::string& virtualPath) const;

    // Manual registration (used by scan and by tests).
    void registerAsset(const std::string& virtualPath,
                       const std::filesystem::path& realPath,
                       const AssetMeta& meta);

    [[nodiscard]] usize size() const { return m_meta.size(); }

    // Read/write a single `.meta` sidecar (YAML). Exposed for reuse/testing.
    static std::optional<AssetMeta> readMeta(const std::filesystem::path& metaPath);
    static bool writeMeta(const std::filesystem::path& metaPath, const AssetMeta& meta);

private:
    std::filesystem::path m_root;
    std::unordered_map<std::string, std::filesystem::path> m_vpathToReal;
    std::unordered_map<std::string, AssetMeta>             m_meta;
    std::unordered_map<UUID, std::filesystem::path>        m_uuidToPath;
};

} // namespace ember
