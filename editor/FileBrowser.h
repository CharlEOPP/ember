#pragma once
#include <filesystem>
#include <string>

namespace ember {

// Minimal in-ImGui file browser modal (no native dependency). Drive it by calling
// open(); then call draw() every frame — it returns true for one frame when the
// user confirms a path (available via result()).
class FileBrowser {
public:
    enum class Mode { Open, Save };

    void open(Mode mode, const std::filesystem::path& root, std::string extension);
    [[nodiscard]] bool isOpen() const { return m_open; }
    [[nodiscard]] Mode mode() const { return m_mode; }

    bool draw();   // true once on confirm
    [[nodiscard]] const std::string& result() const { return m_result; }

private:
    bool                   m_open = false;
    Mode                   m_mode = Mode::Open;
    std::filesystem::path  m_dir;
    std::string            m_ext;          // e.g. ".escene" ("" = any)
    char                   m_name[128] = {0};
    std::string            m_result;
};

} // namespace ember
