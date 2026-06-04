#include "ember/renderer/ShaderLibrary.h"
#include "ember/platform/FileSystem.h"
#include "ember/core/Log.h"

#include <sstream>

namespace ember {

void ShaderLibrary::splitSource(const std::string& src, std::string& vertOut, std::string& fragOut) {
    std::istringstream stream(src);
    std::ostringstream vs, fs;
    std::string line;
    int current = -1;   // 0 = vertex, 1 = fragment

    while (std::getline(stream, line)) {
        if (line.find("#type") != std::string::npos) {
            if (line.find("vertex") != std::string::npos)        current = 0;
            else if (line.find("fragment") != std::string::npos ||
                     line.find("pixel")    != std::string::npos) current = 1;
            else                                                 current = -1;
            continue;
        }
        if      (current == 0) vs << line << '\n';
        else if (current == 1) fs << line << '\n';
    }
    vertOut = vs.str();
    fragOut = fs.str();
}

std::shared_ptr<IShader> ShaderLibrary::loadSource(const std::string& name, const std::string& combinedSrc) {
    std::string vs, fs;
    splitSource(combinedSrc, vs, fs);
    auto shader = RHI::createShader(vs, fs);
    if (shader) m_shaders[name] = shader;
    else        EMBER_LOG_ERROR("ShaderLibrary: failed to compile shader '{}'", name);
    return shader;
}

std::shared_ptr<IShader> ShaderLibrary::loadFile(const std::string& name, const std::string& path) {
    auto contents = FileSystem::readTextFile(path);
    if (!contents) {
        EMBER_LOG_ERROR("ShaderLibrary: cannot read '{}': {}", path, contents.error());
        return nullptr;
    }
    m_paths[name] = path;
    return loadSource(name, *contents);
}

std::shared_ptr<IShader> ShaderLibrary::get(const std::string& name) const {
    auto it = m_shaders.find(name);
    return it == m_shaders.end() ? nullptr : it->second;
}

bool ShaderLibrary::exists(const std::string& name) const {
    return m_shaders.find(name) != m_shaders.end();
}

#if !defined(NDEBUG)
std::shared_ptr<IShader> ShaderLibrary::reload(const std::string& name) {
    auto it = m_paths.find(name);
    if (it == m_paths.end()) return get(name);
    return loadFile(name, it->second);
}
#endif

} // namespace ember
