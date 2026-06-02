#include <catch2/catch_test_macros.hpp>
#include "ember/core/UUID.h"
#include <unordered_set>
#include <thread>
#include <vector>
#include <atomic>

using namespace ember;

TEST_CASE("UUID_NeverReturnsNull", "[uuid]") {
    for (int i = 0; i < 1'000'000; ++i)
        REQUIRE(generateUUID() != NULL_UUID);
}

TEST_CASE("UUID_NoDuplicatesInMillion", "[uuid]") {
    std::unordered_set<UUID> seen;
    seen.reserve(1'000'000);
    for (int i = 0; i < 1'000'000; ++i) {
        const UUID id = generateUUID();
        REQUIRE(seen.insert(id).second);
    }
    REQUIRE(seen.size() == 1'000'000u);
}

TEST_CASE("UUID_IsThreadSafe", "[uuid]") {
    constexpr int N = 250'000;
    constexpr int T = 4;
    std::vector<UUID> results(static_cast<std::size_t>(N * T));

    std::vector<std::thread> threads;
    threads.reserve(T);
    for (int t = 0; t < T; ++t) {
        threads.emplace_back([&results, t, N]() {
            for (int i = 0; i < N; ++i)
                results[static_cast<std::size_t>(t * N + i)] = generateUUID();
        });
    }
    for (auto& th : threads) th.join();

    std::unordered_set<UUID> seen(results.begin(), results.end());
    REQUIRE(seen.size() == results.size());
}
