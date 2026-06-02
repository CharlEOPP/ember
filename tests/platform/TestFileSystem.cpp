#include <catch2/catch_test_macros.hpp>
#include "ember/platform/FileSystem.h"
#include <filesystem>
#include <string>

using namespace ember;

static const std::filesystem::path kTmpDir = std::filesystem::temp_directory_path() / "ember_fs_tests";

TEST_CASE("FS_ReadWriteRoundtrip", "[filesystem]") {
    std::filesystem::create_directories(kTmpDir);
    const auto path = kTmpDir / "roundtrip.txt";
    const std::string content = "Hello, Ember!\nLine 2.";

    auto writeResult = FileSystem::writeTextFile(path, content);
    REQUIRE(writeResult.has_value());

    auto readResult = FileSystem::readTextFile(path);
    REQUIRE(readResult.has_value());
    REQUIRE(*readResult == content);

    std::filesystem::remove(path);
}

TEST_CASE("FS_ExistsReturnsFalse", "[filesystem]") {
    REQUIRE_FALSE(FileSystem::exists(kTmpDir / "does_not_exist_xyz.txt"));
}

TEST_CASE("FS_ExistsReturnsTrueForReal", "[filesystem]") {
    std::filesystem::create_directories(kTmpDir);
    const auto path = kTmpDir / "exists.txt";
    std::filesystem::remove(path);
    FileSystem::writeTextFile(path, "x");
    REQUIRE(FileSystem::exists(path));
    std::filesystem::remove(path);
}

TEST_CASE("FS_ReadNonexistentReturnsError", "[filesystem]") {
    auto result = FileSystem::readTextFile(kTmpDir / "ghost_file.txt");
    REQUIRE_FALSE(result.has_value());
    REQUIRE_FALSE(result.error().empty());
}

TEST_CASE("FS_VirtualPathResolution", "[filesystem]") {
    FileSystem::setAssetsRoot("/tmp/test_assets");
    const auto resolved = FileSystem::resolvePath("textures/player.png");
    REQUIRE(resolved == std::filesystem::path("/tmp/test_assets/textures/player.png"));
}
