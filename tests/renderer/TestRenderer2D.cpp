#include <catch2/catch_test_macros.hpp>
#include "ember/renderer/RHI.h"
#include "ember/renderer/ShaderLibrary.h"
#include "ember/renderer/TextureAtlas.h"

#include <memory>
#include <string>

using namespace ember;

// These tests are CPU-only (no GL context): they exercise the layout math, the
// shader #type parser, and atlas UV computation. On-GPU batching is verified by
// the sandbox.

TEST_CASE("VertexLayout_StrideAndOffsets", "[renderer]") {
    BufferLayout layout({
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float4, "a_Color"    },
        { ShaderDataType::Float2, "a_UV"       },
        { ShaderDataType::Float,  "a_TexIndex" },
        { ShaderDataType::Float,  "a_EntityID" },
    });
    REQUIRE(layout.stride() == (12u + 16u + 8u + 4u + 4u));   // 44 bytes
    const auto& e = layout.elements();
    REQUIRE(e.size() == 5u);
    REQUIRE(e[0].offset == 0u);
    REQUIRE(e[1].offset == 12u);
    REQUIRE(e[2].offset == 28u);
    REQUIRE(e[3].offset == 36u);
    REQUIRE(e[4].offset == 40u);
}

TEST_CASE("ShaderParse_TypeDirectives", "[renderer]") {
    const std::string src =
        "#type vertex\n"
        "void vs() {}\n"
        "#type fragment\n"
        "void fs() {}\n";
    std::string vs, fs;
    ShaderLibrary::splitSource(src, vs, fs);
    REQUIRE(vs.find("void vs() {}") != std::string::npos);
    REQUIRE(vs.find("void fs() {}") == std::string::npos);
    REQUIRE(fs.find("void fs() {}") != std::string::npos);
    REQUIRE(fs.find("void vs() {}") == std::string::npos);
}

namespace {
// Minimal ITexture2D so atlas UVs can be tested without a GL context.
struct MockTexture : ITexture2D {
    u32 w, h;
    MockTexture(u32 width, u32 height) : w(width), h(height) {}
    void bind(u32) const override {}
    void unbind()  const override {}
    u32  getWidth()  const override { return w; }
    u32  getHeight() const override { return h; }
    u32  rendererID() const override { return 0; }
};
} // namespace

TEST_CASE("TextureAtlas_RegionUVs", "[renderer]") {
    auto tex = std::make_shared<MockTexture>(128u, 128u);
    TextureAtlas atlas(tex);
    atlas.addRegion("tile", 32, 32, 64, 64);

    REQUIRE(atlas.has("tile"));
    UVRect r = atlas.region("tile");
    REQUIRE(r.u0 == 0.25f);   // 32/128
    REQUIRE(r.v0 == 0.25f);
    REQUIRE(r.u1 == 0.75f);   // 96/128
    REQUIRE(r.v1 == 0.75f);

    REQUIRE_FALSE(atlas.has("missing"));
}
