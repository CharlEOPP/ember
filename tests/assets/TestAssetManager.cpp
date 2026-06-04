#include <catch2/catch_test_macros.hpp>
#include "ember/assets/AssetManager.h"
#include "ember/assets/AssetDatabase.h"
#include "ember/platform/FileSystem.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

using namespace ember;

namespace {
struct Dummy { int value = 0; };

struct DummyLoader : IAssetLoader<Dummy> {
    int loadCalls   = 0;
    int unloadCalls = 0;
    std::shared_ptr<Dummy> load(const std::filesystem::path&, const AssetSettings&) override {
        ++loadCalls;
        return std::make_shared<Dummy>(Dummy{42});
    }
    void unload(Dummy&) override { ++unloadCalls; }
};

// Two-phase async loader: loadCPU returns an opaque int payload on a worker;
// finalize turns it into the asset on the main thread (no GPU here).
struct AsyncDummyLoader : IAssetLoader<Dummy> {
    int cpuCalls = 0, finCalls = 0;
    std::shared_ptr<Dummy> load(const std::filesystem::path&, const AssetSettings&) override {
        return std::make_shared<Dummy>(Dummy{1});
    }
    bool supportsAsync() const override { return true; }
    std::shared_ptr<void> loadCPU(const std::filesystem::path&, const AssetSettings&) override {
        ++cpuCalls; return std::make_shared<int>(99);
    }
    std::shared_ptr<Dummy> finalize(const std::shared_ptr<void>& cpu) override {
        ++finCalls; return std::make_shared<Dummy>(Dummy{*std::static_pointer_cast<int>(cpu)});
    }
};
} // namespace

TEST_CASE("AssetHandle null and equality", "[assets]") {
    auto n = AssetHandle<Dummy>::null();
    REQUIRE_FALSE(n.valid());
    REQUIRE(AssetHandle<Dummy>{5} == AssetHandle<Dummy>{5});
    REQUIRE(AssetHandle<Dummy>{5} != AssetHandle<Dummy>{6});
}

TEST_CASE("AssetManager caches by path (single load)", "[assets]") {
    AssetManager mgr;
    auto loader = std::make_shared<DummyLoader>();
    mgr.registerLoader<Dummy>(loader);

    auto h1 = mgr.load<Dummy>("things/a.dummy");
    auto h2 = mgr.load<Dummy>("things/a.dummy");
    REQUIRE(h1.valid());
    REQUIRE(h1 == h2);                 // same cache entry
    REQUIRE(loader->loadCalls == 1);   // loader ran exactly once
    REQUIRE(mgr.liveCount() == 1);
    REQUIRE(mgr.refCount(h1.id) == 2); // two outstanding references

    Dummy* d = mgr.get(h1);
    REQUIRE(d != nullptr);
    REQUIRE(d->value == 42);
}

TEST_CASE("AssetManager refcount eviction", "[assets]") {
    AssetManager mgr;
    auto loader = std::make_shared<DummyLoader>();
    mgr.registerLoader<Dummy>(loader);

    auto h = mgr.load<Dummy>("x.dummy");
    (void)mgr.load<Dummy>("x.dummy");  // refcount = 2
    REQUIRE(mgr.liveCount() == 1);

    mgr.release(h);                    // -> 1, still resident
    REQUIRE(mgr.liveCount() == 1);
    REQUIRE(loader->unloadCalls == 0);

    mgr.release(h);                    // -> 0, evicted
    REQUIRE(mgr.liveCount() == 0);
    REQUIRE(loader->unloadCalls == 1);
    REQUIRE(mgr.get(h) == nullptr);    // dead handle
}

TEST_CASE("AssetManager pin prevents eviction", "[assets]") {
    AssetManager mgr;
    auto loader = std::make_shared<DummyLoader>();
    mgr.registerLoader<Dummy>(loader);

    auto h = mgr.load<Dummy>("p.dummy");
    mgr.pin(h);
    mgr.release(h);                    // refcount 0 but pinned -> stays
    REQUIRE(mgr.liveCount() == 1);
    REQUIRE(loader->unloadCalls == 0);

    mgr.unpin(h);                      // refcount already 0 -> evict now
    REQUIRE(mgr.liveCount() == 0);
    REQUIRE(loader->unloadCalls == 1);
}

TEST_CASE("AssetManager reload re-runs the loader", "[assets]") {
    AssetManager mgr;
    auto loader = std::make_shared<DummyLoader>();
    mgr.registerLoader<Dummy>(loader);

    auto h = mgr.load<Dummy>("r.dummy");
    REQUIRE(loader->loadCalls == 1);
    REQUIRE(mgr.reload(h.id));
    REQUIRE(loader->loadCalls == 2);
    REQUIRE(mgr.get(h) != nullptr);
}

TEST_CASE("AssetManager missing loader yields null handle", "[assets]") {
    AssetManager mgr;
    auto h = mgr.load<Dummy>("nope.dummy");
    REQUIRE_FALSE(h.valid());
}

TEST_CASE("AssetDatabase .meta round-trips through YAML", "[assets]") {
    auto tmp = std::filesystem::temp_directory_path() / "ember_meta_test.png.meta";
    AssetMeta meta;
    meta.uuid     = 0x1234ABCDull;
    meta.type     = "Texture2D";
    meta.importer = "Texture2DLoader";
    meta.settings.kv["filter"] = "Linear";
    meta.settings.kv["wrap"]   = "Repeat";

    REQUIRE(AssetDatabase::writeMeta(tmp, meta));
    auto read = AssetDatabase::readMeta(tmp);
    REQUIRE(read.has_value());
    REQUIRE(read->uuid == meta.uuid);
    REQUIRE(read->type == "Texture2D");
    REQUIRE(read->importer == "Texture2DLoader");
    REQUIRE(read->settings.get("filter") == "Linear");
    REQUIRE(read->settings.getBool("wrap", false) == false); // "Repeat" != truthy

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

TEST_CASE("AssetManager async load resolves via processPendingUploads", "[assets]") {
    AssetManager mgr;
    auto loader = std::make_shared<AsyncDummyLoader>();
    mgr.registerLoader<Dummy>(loader);

    auto fut = mgr.loadAsync<Dummy>("async.dummy");

    // Resolve by pumping the upload queue from the main thread (the worker needs
    // a moment to enqueue the finalize step).
    bool resolved = false;
    for (int i = 0; i < 200 && !resolved; ++i) {
        mgr.processPendingUploads();
        if (fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) resolved = true;
        else std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    REQUIRE(resolved);
    auto h = fut.get();
    REQUIRE(h.valid());
    REQUIRE(loader->cpuCalls == 1);
    REQUIRE(loader->finCalls == 1);
    REQUIRE(mgr.get(h) != nullptr);
    REQUIRE(mgr.get(h)->value == 99);
}

TEST_CASE("AssetManager reloadByRealPath reloads matching assets", "[assets]") {
    AssetManager mgr;
    auto loader = std::make_shared<DummyLoader>();
    mgr.registerLoader<Dummy>(loader);

    auto h = mgr.load<Dummy>("r.dummy");
    REQUIRE(loader->loadCalls == 1);
    // With no DB, the manager resolves a virtual path via FileSystem::resolvePath;
    // pass the same resolved path the watcher would report.
    REQUIRE(mgr.reloadByRealPath(FileSystem::resolvePath("r.dummy")) == 1);
    REQUIRE(loader->loadCalls == 2);          // reloadFn re-ran the loader
    REQUIRE(mgr.reloadByRealPath(FileSystem::resolvePath("other")) == 0);
    (void)h;
}
