#pragma once
#include "ember/renderer/RHI.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace ember {

// Caches compiled shaders by name and parses combined "#type" GLSL files.
class ShaderLibrary {
public:
    // Compile a shader from a single source containing #type vertex / #type fragment.
    std::shared_ptr<IShader> loadSource(const std::string& name, const std::string& combinedSrc);
    // Read a .glsl file (combined source) and compile it.
    std::shared_ptr<IShader> loadFile(const std::string& name, const std::string& path);

    std::shared_ptr<IShader> get(const std::string& name) const;
    bool exists(const std::string& name) const;

#if !defined(NDEBUG)
    std::shared_ptr<IShader> reload(const std::string& name);   // re-read from disk (debug only)
#endif

    // Split a combined source into vertex/fragment stage strings. Static + GL-free for testing.
    static void splitSource(const std::string& src, std::string& vertOut, std::string& fragOut);

private:
    std::unordered_map<std::string, std::shared_ptr<IShader>> m_shaders;
    std::unordered_map<std::string, std::string>             m_paths;
};

} // namespace ember
