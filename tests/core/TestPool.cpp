#include <catch2/catch_test_macros.hpp>
#include "ember/core/Pool.h"
#include "ember/core/Assert.h"

using namespace ember;

struct Widget {
    int id;
    float value;
    Widget(int i, float v) : id(i), value(v) {}
};

TEST_CASE("Pool_AllocAndUse", "[pool]") {
    PoolAllocator<Widget> pool(8);
    auto* w = pool.alloc(42, 3.14f);
    REQUIRE(w != nullptr);
    REQUIRE(w->id    == 42);
    REQUIRE(w->value == 3.14f);
    REQUIRE(pool.size() == 1u);
}

TEST_CASE("Pool_FreeAndReuse", "[pool]") {
    PoolAllocator<Widget> pool(4);
    auto* a = pool.alloc(1, 1.0f);
    auto* b = pool.alloc(2, 2.0f);
    auto* c = pool.alloc(3, 3.0f);
    REQUIRE(pool.size() == 3u);

    pool.free(b); // free the middle one
    REQUIRE(pool.size() == 2u);

    auto* d = pool.alloc(99, 9.9f); // should reuse b's slot
    REQUIRE(d != nullptr);
    REQUIRE(d->id == 99);
    REQUIRE(pool.size() == 3u);

    // Clean up
    pool.free(a);
    pool.free(c);
    pool.free(d);
    REQUIRE(pool.size() == 0u);
}

TEST_CASE("Pool_SizeTracking", "[pool]") {
    PoolAllocator<int> pool(10);
    REQUIRE(pool.size()     == 0u);
    REQUIRE(pool.capacity() == 10u);

    std::vector<int*> ptrs;
    for (int i = 0; i < 5; ++i) ptrs.push_back(pool.alloc(i));
    REQUIRE(pool.size() == 5u);

    pool.free(ptrs[2]);
    REQUIRE(pool.size() == 4u);
}

TEST_CASE("Pool_FullPoolAsserts", "[pool]") {
    PoolAllocator<int> pool(2);
    pool.alloc(1);
    pool.alloc(2);
    REQUIRE(pool.size() == 2u);

    bool assertFired = false;
    Assert::setHandler([&](const char*, const char*, int, const char*) {
        assertFired = true;
    });
    int* ptr = pool.alloc(3);
    Assert::resetHandler();

    REQUIRE(assertFired);
    REQUIRE(ptr == nullptr);
}
