#pragma once
#include "ember/core/Types.h"
#include "ember/ecs/World.h"

#include <array>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace ember {

// Execution phases, run in this order each frame.
enum class Phase { PreUpdate, Update, PostUpdate, Render, Debug, Count };

// A system is anything deriving from ISystem.
struct ISystem {
    virtual ~ISystem() = default;
    virtual void update(World& world, f32 dt) = 0;
};

class SystemScheduler {
public:
    template<typename T, typename... Args>
    T& addSystem(Phase phase, Args&&... args) {
        static_assert(std::is_base_of_v<ISystem, T>, "System must derive from ISystem");
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *sys;
        bucket(phase).push_back(std::move(sys));
        return ref;
    }

    void runPhase(Phase phase, World& world, f32 dt) {
        for (auto& s : bucket(phase)) s->update(world, dt);
    }

private:
    using Bucket = std::vector<std::unique_ptr<ISystem>>;

    Bucket& bucket(Phase phase) {
        return m_phases[static_cast<usize>(phase)];
    }

    std::array<Bucket, static_cast<usize>(Phase::Count)> m_phases;
};

} // namespace ember
