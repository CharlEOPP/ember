#include "ember/assets/FileWatcher.h"
#include "ember/core/Log.h"

#include <chrono>
#include <system_error>
#include <unordered_map>

namespace ember {

FileWatcher::~FileWatcher() { stop(); }

void FileWatcher::stop() {
    m_running.store(false);
    if (m_thread.joinable()) m_thread.join();
}

std::vector<std::filesystem::path> FileWatcher::poll() {
    std::vector<std::filesystem::path> out;
    std::lock_guard<std::mutex> lk(m_mutex);
    out.swap(m_changed);
    return out;
}

#if defined(EMBER_HOT_RELOAD)

void FileWatcher::watch(const std::filesystem::path& root, u32 pollMs) {
    if (m_running.load()) return;
    m_running.store(true);
    m_thread = std::thread(&FileWatcher::run, this, root, pollMs);
    EMBER_LOG_INFO("FileWatcher: watching '{}' every {}ms", root.string(), pollMs);
}

void FileWatcher::run(std::filesystem::path root, u32 pollMs) {
    using clock = std::filesystem::file_time_type;
    std::unordered_map<std::string, clock> times;
    bool primed = false;

    while (m_running.load()) {
        std::error_code ec;
        std::vector<std::filesystem::path> hits;
        for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
             !ec && it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (!it->is_regular_file()) continue;
            if (it->path().extension() == ".meta") continue;
            const std::string key = it->path().string();
            const auto t = std::filesystem::last_write_time(it->path(), ec);
            if (ec) continue;
            auto found = times.find(key);
            if (found == times.end()) { times[key] = t; if (primed) hits.push_back(it->path()); }
            else if (found->second != t) { found->second = t; hits.push_back(it->path()); }
        }
        primed = true;   // ignore the first full scan (everything looks "new")
        if (!hits.empty()) {
            std::lock_guard<std::mutex> lk(m_mutex);
            for (auto& p : hits) m_changed.push_back(std::move(p));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
    }
}

#else // hot reload disabled (release / web): no-ops

void FileWatcher::watch(const std::filesystem::path&, u32) {}
void FileWatcher::run(std::filesystem::path, u32) {}

#endif

} // namespace ember
