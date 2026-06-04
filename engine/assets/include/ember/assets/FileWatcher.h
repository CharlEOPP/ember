#pragma once
#include "ember/core/Types.h"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

namespace ember {

// Polling-based directory watcher: a background thread snapshots file
// modification times under a root every `pollMs` and reports changed paths via
// a thread-safe queue drained by poll(). Portable across platforms (no inotify/
// ReadDirectoryChangesW/kqueue). Active only when compiled with EMBER_HOT_RELOAD;
// otherwise watch() is a no-op and poll() always returns empty.
class FileWatcher {
public:
    FileWatcher() = default;
    ~FileWatcher();

    FileWatcher(const FileWatcher&)            = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    void watch(const std::filesystem::path& root, u32 pollMs = 250);
    void stop();

    // Drain and return the paths changed since the last call (main thread).
    std::vector<std::filesystem::path> poll();

    [[nodiscard]] bool active() const { return m_running.load(); }

private:
    void run(std::filesystem::path root, u32 pollMs);

    std::thread             m_thread;
    std::atomic<bool>       m_running{false};
    std::mutex              m_mutex;
    std::vector<std::filesystem::path> m_changed;   // guarded by m_mutex
};

} // namespace ember
