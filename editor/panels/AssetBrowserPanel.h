#pragma once
#include "../EditorContext.h"

#include <filesystem>
#include <functional>
#include <string>

namespace ember {

// Browses the project's assets/ folder: a directory tree on the left, a file list
// on the right. Files are drag sources (payload "EMBER_ASSET_PATH") so they can be
// dropped onto Inspector asset fields; double-clicking a .escene opens it.
class AssetBrowserPanel {
public:
    void setOpenSceneCallback(std::function<void(const std::string&)> cb) { m_openScene = std::move(cb); }

    void onImGuiRender(EditorContext& ctx);

private:
    void drawDirTree(const std::filesystem::path& dir);

    std::filesystem::path m_root    = "assets";
    std::filesystem::path m_current = "assets";
    std::filesystem::path m_pendingDelete;
    std::function<void(const std::string&)> m_openScene;
};

} // namespace ember
