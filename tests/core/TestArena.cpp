#include <catch2/catch_test_macros.hpp>
#include "ember/core/Arena.h"
#include "ember/core/Assert.h"
#include <cstdint>

using namespace ember;

struct Vec3 { float x, y, z; };

TEST_CASE("Arena_AllocAndUse", "[arena]") {
    ArenaAllocator arena(1024);
    auto* v = arena.alloc<Vec3>(1.0f, 2.0f, 3.0f);
    REQUIRE(v != nullptr);
    REQUIRE(v->x == 1.0f);
    REQUIRE(v->y == 2.0f);
    REQUIRE(v->z == 3.0f);
}

TEST_CASE("Arena_UsedAndCapacity", "[arena]") {
    ArenaAllocator arena(256);
    REQUIRE(arena.used()     == 0u);
    REQUIRE(arena.capacity() == 256u);

    arena.alloc<int>(42);
    REQUIRE(arena.used() >= sizeof(int));
    REQUIRE(arena.capacity() == 256u);
}

TEST_CASE("Arena_ResetAndReuse", "[arena]") {
    ArenaAllocator arena(64);
    for (int i = 0; i < 16; ++i)
        arena.alloc<int>(i);

    arena.reset();
    REQUIRE(arena.used() == 0u);

    // Can allocate again from the start
    auto* ptr = arena.alloc<int>(99);
    REQUIRE(ptr != nullptr);
    REQUIRE(*ptr == 99);
}

TEST_CASE("Arena_AlignmentCorrect", "[arena]") {
    ArenaAllocator arena(512);
    // Mis-align intentionally with a 1-byte alloc first
    arena.allocRaw(1, 1);
    // Then request 16-byte alignment
    void* aligned = arena.allocRaw(1, 16);
    REQUIRE(aligned != nullptr);
    const uintptr_t addr = reinterpret_cast<uintptr_t>(aligned);
    REQUIRE(addr % 16 == 0);
}

TEST_CASE("Arena_FullArenaAsserts", "[arena]") {
#if defined(NDEBUG)
    SKIP("EMBER_ASSERT is compiled out in Release builds (NDEBUG)");
#else
    ArenaAllocator arena(8);
    // Fill the arena completely
    arena.allocRaw(8, 1);
    REQUIRE(arena.used() == 8u);

    // The next allocation should trigger the assertion handler
    bool assertFired = false;
    Assert::setHandler([&](const char*, const char*, int, const char*) {
        assertFired = true;
    });
    void* ptr = arena.allocRaw(1, 1);
    Assert::resetHandler();

    REQUIRE(assertFired);
    REQUIRE(ptr == nullptr);
#endif
}
