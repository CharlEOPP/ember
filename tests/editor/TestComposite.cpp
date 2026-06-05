#include <catch2/catch_test_macros.hpp>
#include "CommandHistory.h"

#include <memory>
#include <vector>

using namespace ember;

namespace {
std::unique_ptr<Command> setv(int& t, int to) {
    const int from = t;
    return std::make_unique<FunctionalCommand>("s", [&t, to] { t = to; }, [&t, from] { t = from; });
}
// Local replica of Commands::composite (which needs Scene to link); same shape.
std::unique_ptr<Command> composite(std::vector<std::unique_ptr<Command>> cmds) {
    auto box = std::make_shared<std::vector<std::unique_ptr<Command>>>(std::move(cmds));
    return std::make_unique<FunctionalCommand>("c",
        [box] { for (auto& c : *box) c->execute(); },
        [box] { for (auto it = box->rbegin(); it != box->rend(); ++it) (*it)->undo(); });
}
} // namespace

TEST_CASE("composite applies + undoes as one step", "[editor][command]") {
    int a = 0, b = 0, c = 0;
    std::vector<std::unique_ptr<Command>> cmds;
    cmds.push_back(setv(a, 1));
    cmds.push_back(setv(b, 2));
    cmds.push_back(setv(c, 3));

    CommandHistory h;
    h.push(composite(std::move(cmds)));
    REQUIRE((a == 1 && b == 2 && c == 3));
    REQUIRE(h.size() == 1);

    h.undo();
    REQUIRE((a == 0 && b == 0 && c == 0));
    h.redo();
    REQUIRE((a == 1 && b == 2 && c == 3));
}
