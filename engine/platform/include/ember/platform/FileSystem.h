#pragma once
#include "ember/core/Result.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>

namespace ember {

class FileSystem {
public:
    /** Set the root directory for virtual path resolution. */
    static void setAssetsRoot(const std::filesystem::path& root);

    /** Prepend the assets root to a virtual path. */
    [[nodiscard]] static std::filesystem::path resolvePath(std::string_view virtualPath);

    /** Read entire file as a UTF-8 string. */
    [[nodiscard]] static Result<std::string, std::string>
        readTextFile(const std::filesystem::path& path);

    /** Read entire file as raw bytes. */
    [[nodiscard]] static Result<std::vector<std::byte>, std::string>
        readBinaryFile(const std::filesystem::path& path);

    /** Write string to file, creating parent directories as needed. */
    [[nodiscard]] static Result<void, std::string>
        writeTextFile(const std::filesystem::path& path, std::string_view content);

    [[nodiscard]] static bool exists(const std::filesystem::path& path);

private:
    static std::filesystem::path s_assetsRoot;
};

} // namespace ember
