#include "FileBrowser.h"

#include <imgui.h>
#include <algorithm>
#include <system_error>
#include <vector>

namespace ember {

namespace fs = std::filesystem;

void FileBrowser::open(Mode mode, const fs::path& root, std::string extension) {
    m_mode = mode;
    m_ext  = std::move(extension);
    std::error_code ec;
    m_dir  = fs::exists(root, ec) ? fs::absolute(root, ec) : fs::current_path(ec);
    m_name[0] = '\0';
    m_result.clear();
    m_open = true;
}

bool FileBrowser::draw() {
    if (!m_open) return false;

    const char* title = (m_mode == Mode::Save) ? "Save Scene##fb" : "Open Scene##fb";
    ImGui::OpenPopup(title);

    bool confirmed = false;
    ImGui::SetNextWindowSize(ImVec2(520, 380), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal(title, &m_open)) {
        ImGui::TextUnformatted(m_dir.string().c_str());
        ImGui::Separator();

        // Gather entries (dirs first, then matching files).
        std::error_code ec;
        std::vector<fs::directory_entry> dirs, files;
        for (const auto& de : fs::directory_iterator(m_dir, ec)) {
            if (de.is_directory(ec)) dirs.push_back(de);
            else if (de.is_regular_file(ec)) {
                if (m_ext.empty() || de.path().extension() == m_ext) files.push_back(de);
            }
        }
        auto byName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
            return a.path().filename().string() < b.path().filename().string();
        };
        std::sort(dirs.begin(), dirs.end(), byName);
        std::sort(files.begin(), files.end(), byName);

        ImGui::BeginChild("##entries", ImVec2(0, m_mode == Mode::Save ? -64.0f : -36.0f), true);
        if (m_dir.has_parent_path() && ImGui::Selectable(".."))
            m_dir = m_dir.parent_path();
        for (const auto& d : dirs) {
            const std::string label = "[DIR] " + d.path().filename().string();
            if (ImGui::Selectable(label.c_str())) m_dir = d.path();
        }
        for (const auto& f : files) {
            const std::string fname = f.path().filename().string();
            if (ImGui::Selectable(fname.c_str())) {
                if (m_mode == Mode::Open) {
                    m_result   = f.path().string();
                    confirmed  = true;
                } else {
                    std::snprintf(m_name, sizeof(m_name), "%s", fname.c_str());
                }
            }
        }
        ImGui::EndChild();

        if (m_mode == Mode::Save) {
            ImGui::SetNextItemWidth(-90.0f);
            ImGui::InputText("##filename", m_name, sizeof(m_name));
            ImGui::SameLine();
            if (ImGui::Button("Save", ImVec2(80, 0)) && m_name[0] != '\0') {
                std::string fname = m_name;
                if (!m_ext.empty() && fs::path(fname).extension() != m_ext) fname += m_ext;
                m_result  = (m_dir / fname).string();
                confirmed = true;
            }
        }

        if (ImGui::Button("Cancel")) { m_open = false; ImGui::CloseCurrentPopup(); }

        if (confirmed) { m_open = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    return confirmed;
}

} // namespace ember
