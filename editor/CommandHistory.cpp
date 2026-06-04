#include "CommandHistory.h"

namespace ember {

void CommandHistory::push(std::unique_ptr<Command> cmd) {
    cmd->execute();
    pushExecuted(std::move(cmd));
}

void CommandHistory::pushExecuted(std::unique_ptr<Command> cmd) {
    // Drop any redo tail, then append.
    if (m_index < m_stack.size())
        m_stack.erase(m_stack.begin() + static_cast<std::ptrdiff_t>(m_index), m_stack.end());

    m_stack.push_back(std::move(cmd));
    m_index = m_stack.size();

    // Enforce the cap by evicting the oldest entries.
    while (m_stack.size() > m_cap) {
        m_stack.erase(m_stack.begin());
        --m_index;
    }
}

void CommandHistory::undo() {
    if (!canUndo()) return;
    --m_index;
    m_stack[m_index]->undo();
}

void CommandHistory::redo() {
    if (!canRedo()) return;
    m_stack[m_index]->execute();
    ++m_index;
}

void CommandHistory::clear() {
    m_stack.clear();
    m_index = 0;
}

} // namespace ember
