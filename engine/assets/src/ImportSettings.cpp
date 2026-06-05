#include "ember/assets/ImportSettings.h"

#include <sstream>

namespace ember {

// ---- Texture ----

TextureImportSettings TextureImportSettings::fromSettings(const AssetSettings& s) {
    TextureImportSettings t;
    t.filter       = (s.get("filter", "Linear") == "Nearest") ? TextureFilter::Nearest : TextureFilter::Linear;
    t.wrap         = (s.get("wrap", "Repeat") == "ClampToEdge") ? TextureWrap::ClampToEdge : TextureWrap::Repeat;
    const std::string fmt = s.get("format", "RGBA8");
    t.format       = (fmt == "R8")   ? TextureFormat::R8
                   : (fmt == "RGB8") ? TextureFormat::RGB8
                                     : TextureFormat::RGBA8;
    t.generateMips = s.getBool("generateMips", false);
    try { t.maxSize = std::stoi(s.get("maxSize", "0")); } catch (...) { t.maxSize = 0; }
    return t;
}

AssetSettings TextureImportSettings::toSettings() const {
    AssetSettings s;
    s.kv["filter"]       = (filter == TextureFilter::Nearest) ? "Nearest" : "Linear";
    s.kv["wrap"]         = (wrap == TextureWrap::ClampToEdge) ? "ClampToEdge" : "Repeat";
    s.kv["format"]       = (format == TextureFormat::R8)   ? "R8"
                         : (format == TextureFormat::RGB8) ? "RGB8" : "RGBA8";
    s.kv["generateMips"] = generateMips ? "true" : "false";
    s.kv["maxSize"]      = std::to_string(maxSize);
    return s;
}

// ---- Font ----

FontImportSettings FontImportSettings::fromSettings(const AssetSettings& s) {
    FontImportSettings f;
    const std::string list = s.get("sizes", "");
    if (!list.empty()) {
        f.sizes.clear();
        std::stringstream ss(list);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            try { f.sizes.push_back(std::stoi(tok)); } catch (...) {}
        }
    }
    return f;
}

AssetSettings FontImportSettings::toSettings() const {
    AssetSettings s;
    std::string list;
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        if (i) list += ',';
        list += std::to_string(sizes[i]);
    }
    s.kv["sizes"] = list;
    return s;
}

} // namespace ember
