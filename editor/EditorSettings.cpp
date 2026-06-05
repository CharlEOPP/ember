#include "EditorSettings.h"

#include <fstream>
#include <sstream>

namespace ember {

namespace {
std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}
} // namespace

bool EditorSettings::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const std::string key = trim(line.substr(0, colon));
        const std::string val = trim(line.substr(colon + 1));
        try {
            if      (key == "theme")            theme            = std::stoi(val);
            else if (key == "gridSnap")         gridSnap         = std::stof(val);
            else if (key == "rotateSnap")       rotateSnap       = std::stof(val);
            else if (key == "cameraPanSpeed")   cameraPanSpeed   = std::stof(val);
            else if (key == "cameraZoomSpeed")  cameraZoomSpeed  = std::stof(val);
            else if (key == "autoSaveInterval") autoSaveInterval = std::stof(val);
            else if (key == "scriptTemplatePath") scriptTemplatePath = val;
        } catch (...) { /* ignore malformed value, keep default */ }
    }
    return true;
}

bool EditorSettings::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "theme: "            << theme            << '\n'
      << "gridSnap: "         << gridSnap         << '\n'
      << "rotateSnap: "       << rotateSnap       << '\n'
      << "cameraPanSpeed: "   << cameraPanSpeed   << '\n'
      << "cameraZoomSpeed: "  << cameraZoomSpeed  << '\n'
      << "autoSaveInterval: " << autoSaveInterval << '\n'
      << "scriptTemplatePath: " << scriptTemplatePath << '\n';
    return true;
}

} // namespace ember
