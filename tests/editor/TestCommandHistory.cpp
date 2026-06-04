#include <catch2/catch_test_macros.hpp>
#include "CommandHistory.h"

#include <memory>

using namespace ember;

namespace {
std::unique_ptr<Command> setCmd(int& target, int to) {
    const int from = target;
    return std::make_unique<FunctionalCommand>("set",
        [&target, to]   { target = to; },
        [&target, from] { target = from; });
}
} // namespace

TEST_CASE("CommandHistory executes on push and reverses on undo", "[editor][command]") {
    int v = 0;
    CommandHistory h;
    h.push(setCmd(v, 1));
    h.push(setCmd(v, 2));
    REQUIRE(v == 2);
    REQUIRE(h.size() == 2);

    h.undo(); REQUIRE(v == 1);
    h.undo(); REQUIRE(v == 0);
    REQUIRE_FALSE(h.canUndo());
    h.undo();                  // underflow is a no-op
    REQUIRE(v == 0);

    h.redo(); REQUIRE(v == 1);
    h.redo(); REQUIRE(v == 2);
    REQUIRE_FALSE(h.canRedo());
}

TEST_CASE("A new push truncates the redo tail", "[editor][command]") {
    int v = 0;
    CommandHistory h;
    h.push(setCmd(v, 1));
    h.push(setCmd(v, 2));
    h.undo();                  // v == 1, redo tail = {2}
    REQUIRE(h.canRedo());
    h.push(setCmd(v, 9));      // truncates the tail
    REQUIRE(v == 9);
    REQUIRE_FALSE(h.canRedo());
}

TEST_CASE("CommandHistory evicts the oldest past the cap", "[editor][command]") {
    int v = 0;
    CommandHistory h(3);
    h.push(setCmd(v, 1));
    h.push(setCmd(v, 2));
    h.push(setCmd(v, 3));
    h.push(setCmd(v, 4));      // first command evicted
    REQUIRE(h.size() == 3);
    // Can only undo back through the 3 retained commands (to v==1), not to 0.
    h.undo(); h.undo(); h.undo();
    REQUIRE(v == 1);
    REQUIRE_FALSE(h.canUndo());
}

TEST_CASE("pushExecuted records without re-executing", "[editor][command]") {
    int v = 42;
    CommandHistory h;
    h.pushExecuted(setCmd(v, 100));   // captured from=42; do() NOT run
    REQUIRE(v == 42);
    h.undo(); REQUIRE(v == 42);        // undo restores the captured 'from'
    h.redo(); REQUIRE(v == 100);       // redo runs do()
}
