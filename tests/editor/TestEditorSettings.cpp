#include <catch2/catch_test_macros.hpp>
#include "EditorSettings.h"

#include <cstdio>
#include <filesystem>

using namespace ember;

TEST_CASE("EditorSettings save/load round-trips", "[editor][settings]") {
    const std::string path =
        (std::filesystem::temp_directory_path() / "ember_prefs_test.yaml").string();

    EditorSettings a;
    a.theme = 2; a.gridSnap = 0.5f; a.rotateSnap = 45.0f;
    a.cameraPanSpeed = 2.0f; a.cameraZoomSpeed = 0.5f;
    a.autoSaveInterval = 30.0f; a.scriptTemplatePath = "tpl/x.cpp";
    REQUIRE(a.save(path));

    EditorSettings b;
    REQUIRE(b.load(path));
    REQUIRE(b.theme == 2);
    REQUIRE(b.gridSnap == 0.5f);
    REQUIRE(b.rotateSnap == 45.0f);
    REQUIRE(b.cameraPanSpeed == 2.0f);
    REQUIRE(b.cameraZoomSpeed == 0.5f);
    REQUIRE(b.autoSaveInterval == 30.0f);
    REQUIRE(b.scriptTemplatePath == "tpl/x.cpp");

    std::remove(path.c_str());
}

TEST_CASE("EditorSettings keeps defaults when file is missing", "[editor][settings]") {
    EditorSettings c;
    REQUIRE_FALSE(c.load("definitely/not/a/real/path.yaml"));
    REQUIRE(c.theme == 0);
    REQUIRE(c.gridSnap == 1.0f);
    REQUIRE(c.rotateSnap == 15.0f);
}
