#include "ember/assets/AssetDatabase.h"
#include "ember/core/Log.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <system_error>

namespace ember {

namespace {
// Normalize a path to forward-slash virtual form relative to root.
std::string toVirtual(const std::filesystem::path& root, const std::filesystem::path& file) {
    std::error_code ec;
    auto rel = std::filesystem::relative(file, root, ec);
    std::string s = ec ? file.generic_string() : rel.generic_string();
    return s;
}
} // namespace

std::optional<AssetMeta> AssetDatabase::readMeta(const std::filesystem::path& metaPath) {
    YAML::Node node;
    try {
        node = YAML::LoadFile(metaPath.string());
    } catch (const std::exception&) {
        return std::nullopt;
    }
    AssetMeta meta;
    meta.uuid     = node["uuid"]     ? node["uuid"].as<std::uint64_t>(0ull) : 0ull;
    meta.type     = node["type"]     ? node["type"].as<std::string>("")     : "";
    meta.importer = node["importer"] ? node["importer"].as<std::string>("") : "";
    if (node["settings"] && node["settings"].IsMap()) {
        for (auto it = node["settings"].begin(); it != node["settings"].end(); ++it)
            meta.settings.kv[it->first.as<std::string>()] = it->second.as<std::string>();
    }
    return meta;
}

bool AssetDatabase::writeMeta(const std::filesystem::path& metaPath, const AssetMeta& meta) {
    YAML::Node node;
    node["uuid"]     = static_cast<std::uint64_t>(meta.uuid);
    node["type"]     = meta.type;
    node["importer"] = meta.importer;
    YAML::Node settings;
    for (const auto& [k, v] : meta.settings.kv) settings[k] = v;
    node["settings"] = settings;
    try {
        std::ofstream out(metaPath);
        if (!out) return false;
        out << node;
    } catch (const std::exception& e) {
        EMBER_LOG_WARN("AssetDatabase: failed to write meta '{}': {}", metaPath.string(), e.what());
        return false;
    }
    return true;
}

void AssetDatabase::scan(const std::filesystem::path& root) {
    m_root = root;
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        EMBER_LOG_WARN("AssetDatabase: scan root '{}' does not exist", root.string());
        return;
    }
    for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); ++it) {
        if (!it->is_regular_file()) continue;
        const std::filesystem::path& file = it->path();
        if (file.extension() == ".meta") continue;

        const std::filesystem::path metaPath = file.string() + ".meta";
        AssetMeta meta;
        if (auto existing = readMeta(metaPath)) {
            meta = *existing;
            if (meta.uuid == NULL_UUID) meta.uuid = generateUUID();
        } else {
            meta.uuid = generateUUID();
            meta.type = file.extension().string();
            writeMeta(metaPath, meta);
        }
        registerAsset(toVirtual(root, file), file, meta);
    }
    EMBER_LOG_INFO("AssetDatabase: scanned '{}', {} assets", root.string(), m_meta.size());
}

void AssetDatabase::registerAsset(const std::string& virtualPath,
                                  const std::filesystem::path& realPath,
                                  const AssetMeta& meta) {
    m_vpathToReal[virtualPath] = realPath;
    m_meta[virtualPath]        = meta;
    if (meta.uuid != NULL_UUID) m_uuidToPath[meta.uuid] = realPath;
}

std::filesystem::path AssetDatabase::getRealPath(const std::string& virtualPath) const {
    auto it = m_vpathToReal.find(virtualPath);
    if (it != m_vpathToReal.end()) return it->second;
    return m_root.empty() ? std::filesystem::path(virtualPath) : (m_root / virtualPath);
}

std::optional<std::filesystem::path> AssetDatabase::getPath(UUID uuid) const {
    auto it = m_uuidToPath.find(uuid);
    if (it == m_uuidToPath.end()) return std::nullopt;
    return it->second;
}

const AssetMeta* AssetDatabase::getMeta(const std::string& virtualPath) const {
    auto it = m_meta.find(virtualPath);
    return it == m_meta.end() ? nullptr : &it->second;
}

} // namespace ember
