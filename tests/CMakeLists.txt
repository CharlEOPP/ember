# Only build tests if Catch2 was fetched (EMBER_FETCH_DEPS=ON)
if(NOT TARGET Catch2::Catch2WithMain)
    message(STATUS "Ember tests: Catch2 not available -- skipping test target")
    return()
endif()

include(CTest)
# Catch2 (via FetchContent) adds its extras/ dir to CMAKE_MODULE_PATH,
# so the Catch module is included by name, not by relative path.
include(Catch)

add_executable(ember_tests
    core/TestUUID.cpp
    core/TestStringHash.cpp
    core/TestArena.cpp
    core/TestPool.cpp
    core/TestLog.cpp
    platform/TestTimer.cpp
    platform/TestFileSystem.cpp
    # Epic 03 — ECS & Scene
    events/TestEventBus.cpp
    events/TestSignal.cpp
    ecs/TestWorld.cpp
    ecs/TestScheduler.cpp
    ecs/TestReflection.cpp
    ecs/TestPerf.cpp
    scene/TestHierarchy.cpp
    serialization/TestSceneSerialization.cpp
    # Epic 04 — 2D Renderer (CPU-only: layout, shader parse, atlas, camera)
    renderer/TestRenderer2D.cpp
    renderer/TestCamera.cpp
    # Epic 05 — Input & Assets (CPU-only: edge states, action map, context)
    input/TestInputState.cpp
    assets/TestAssetManager.cpp
    # Epic 06 — Scripting (CPU-only: lifecycle, order, serialization)
    scripting/TestScriptLifecycle.cpp
    scripting/TestScriptSerialization.cpp
    scripting/TestScriptAssets.cpp
    # Epic 12 — 2D gameplay & audio
    audio/TestAudioEngine.cpp
    physics2d/TestRigidBody2D.cpp
    physics2d/TestCollision2D.cpp
    physics2d/TestCollisionEvents.cpp
    renderer/TestSpriteAnimation.cpp
    renderer/TestTilemap.cpp
    renderer/TestParticles.cpp
    # Epic 07 — Editor (ImGui-free logic via ember_editor_core)
    editor/TestCommandHistory.cpp
    editor/TestSceneCommands.cpp
    editor/TestPicking.cpp
    editor/TestInspectorModel.cpp
    # Epic 08 — Editor v2
    editor/TestSelection.cpp
    editor/TestGizmo.cpp
    editor/TestComposite.cpp
    editor/TestProfiler.cpp
    editor/TestTilemapEdit.cpp
    editor/TestImportSettings.cpp
    editor/TestEditorSettings.cpp
)

target_link_libraries(ember_tests
    PRIVATE
        ember
        ember_editor_core   # Epic 07 editor logic (ImGui-free) + its include dir
        Catch2::Catch2WithMain
        # scripting tests include RegisterScript.h (registerComponentType → yaml);
        # yaml-cpp is PRIVATE in serialization so it isn't propagated transitively.
        yaml-cpp::yaml-cpp
)

ember_set_compiler_options(ember_tests)
# Catch2 returns exit code 4 when all run test cases were skipped (SKIP()).
# Map that to a CTest "skipped" result so Release-only skips don't fail the run.
catch_discover_tests(ember_tests PROPERTIES SKIP_RETURN_CODE 4)
