#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ember {

// A reversible editor action.
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    [[nodiscard]] virtual const std::string& name() const = 0;
};

// Command built from do/undo lambdas (used for all built-in editor commands).
class FunctionalCommand : public Command {
public:
    FunctionalCommand(std::string name, std::function<void()> doFn, std::function<void()> undoFn)
        : m_name(std::move(name)), m_do(std::move(doFn)), m_undo(std::move(undoFn)) {}
    void execute() override { m_do(); }
    void undo() override { m_undo(); }
    [[nodiscard]] const std::string& name() const override { return m_name; }

private:
    std::string           m_name;
    std::function<void()> m_do;
    std::function<void()> m_undo;
};

// Linear undo/redo stack. push() executes immediately and clears the redo tail;
// the stack is capped (oldest evicted). ImGui-free → unit-testable.
class CommandHistory {
public:
    explicit CommandHistory(std::size_t cap = 100) : m_cap(cap) {}

    void push(std::unique_ptr<Command> cmd);          // execute + record
    void pushExecuted(std::unique_ptr<Command> cmd);  // record an already-applied command

    [[nodiscard]] bool canUndo() const { return m_index > 0; }
    [[nodiscard]] bool canRedo() const { return m_index < m_stack.size(); }
    void undo();
    void redo();
    void clear();

    [[nodiscard]] std::size_t size()  const { return m_stack.size(); }
    [[nodiscard]] std::size_t index() const { return m_index; }

private:
    std::vector<std::unique_ptr<Command>> m_stack;
    std::size_t m_index = 0;   // # of applied commands; next undo targets m_index-1
    std::size_t m_cap;
};

} // namespace ember
