#include "ember/platform/FileSystem.h"
#include <fstream>
#include <sstream>

namespace ember {

std::filesystem::path FileSystem::s_assetsRoot = ".";

void FileSystem::setAssetsRoot(const std::filesystem::path& root) {
    s_assetsRoot = root;
}

std::filesystem::path FileSystem::resolvePath(std::string_view virtualPath) {
    return s_assetsRoot / virtualPath;
}

Result<std::string, std::string> FileSystem::readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return Err(std::string("Cannot open file: ") + path.string());
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

Result<std::vector<std::byte>, std::string> FileSystem::readBinaryFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return Err(std::string("Cannot open file: ") + path.string());
    const auto size = file.tellg();
    file.seekg(0);
    std::vector<std::byte> buf(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

Result<void, std::string> FileSystem::writeTextFile(const std::filesystem::path& path,
                                                      std::string_view content) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return Err(std::string("Cannot create directories: ") + ec.message());
    std::ofstream file(path);
    if (!file.is_open())
        return Err(std::string("Cannot write file: ") + path.string());
    file << content;
    return {};
}

bool FileSystem::exists(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

} // namespace ember
