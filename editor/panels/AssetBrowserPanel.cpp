#include "AssetBrowserPanel.h"

#include <imgui.h>
#include <algorithm>
#include <system_error>
#include <vector>

namespace ember {

namespace fs = std::filesystem;

void AssetBrowserPanel::drawDirTree(const fs::path& dir) {
    std::error_code ec;
    for (const auto& de : fs::directory_iterator(dir, ec)) {
        if (!de.is_directory(ec)) continue;
        const std::string name = de.path().filename().string();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (de.path() == m_current) flags |= ImGuiTreeNodeFlags_Selected;

        const bool open = ImGui::TreeNodeEx(name.c_str(), flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) m_current = de.path();
        if (open) { drawDirTree(de.path()); ImGui::TreePop(); }
    }
}

void AssetBrowserPanel::onImGuiRender(EditorContext& ctx) {
    (void)ctx;
    ImGui::Begin("Assets");

    std::error_code ec;
    if (!fs::exists(m_root, ec)) {
        ImGui::TextUnformatted("No 'assets' folder next to the editor.");
        ImGui::End();
        return;
    }

    // ---- Left: directory tree ----
    ImGui::BeginChild("##dirtree", ImVec2(200.0f, 0.0f), true);
    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
    if (m_current == m_root) rootFlags |= ImGuiTreeNodeFlags_Selected;
    if (ImGui::TreeNodeEx("assets", rootFlags)) {
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) m_current = m_root;
        drawDirTree(m_root);
        ImGui::TreePop();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // ---- Right: files in the current directory ----
    ImGui::BeginChild("##files", ImVec2(0.0f, 0.0f), true);
    ImGui::TextDisabled("%s", m_current.string().c_str());
    ImGui::Separator();

    std::vector<fs::directory_entry> files;
    for (const auto& de : fs::directory_iterator(m_current, ec))
        if (de.is_regular_file(ec)) files.push_back(de);
    std::sort(files.begin(), files.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
        return a.path().filename().string() < b.path().filename().string();
    });

    for (const auto& f : files) {
        const std::string fname = f.path().filename().string();
        const std::string full  = f.path().string();
        const bool isScene = f.path().extension() == ".escene";

        ImGui::Selectable(fname.c_str());

        // Drag source: full path, consumed by Inspector asset fields.
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("EMBER_ASSET_PATH", full.c_str(),
                                      full.size() + 1);   // include the null terminator
            ImGui::TextUnformatted(fname.c_str());
            ImGui::EndDragDropSource();
        }

        // Double-click a scene to open it.
        if (isScene && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)
            && m_openScene)
            m_openScene(full);

        // Right-click: delete (confirmed below).
        if (ImGui::BeginPopupContextItem(full.c_str())) {
            if (ImGui::MenuItem("Delete")) m_pendingDelete = f.path();
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();

    // Deferred delete with confirm.
    if (!m_pendingDelete.empty()) {
        ImGui::OpenPopup("Delete file?");
        if (ImGui::BeginPopupModal("Delete file?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Delete '%s'?", m_pendingDelete.filename().string().c_str());
            if (ImGui::Button("Delete")) {
                std::error_code dec;
                fs::remove(m_pendingDelete, dec);
                m_pendingDelete.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) { m_pendingDelete.clear(); ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

} // namespace ember
