#include <ranges>

#include <swr/swr.h>
#include <swr/shaders.h>

#include <gtest/gtest.h>

#include "tasks/task_system.h"
#include "renderer/material_manager.h"
#include "renderer/render_device.h"
#include "shader_cache.h"
#include "shader_factory.h"
#include "texture_cache.h"

// Asset directory is defined via CMakeTests.txt.
// We also provide a fallback.
#ifndef ASSETS_SOURCE_DIR
#    define ASSETS_SOURCE_DIR "assets"
#endif

namespace
{

struct FirstShader final
: public swr::program<FirstShader>
{
    static constexpr std::string_view name = "FirstShader";

    FirstShader() = default;

    virtual void pre_link(
      boost::container::static_vector<
        swr::interpolation_qualifier,
        swr::limits::max::varyings>& iqs) const override
    {
        iqs = {};
    }

    void vertex_shader(
      [[maybe_unused]] int gl_VertexID,
      [[maybe_unused]] int gl_InstanceID,
      [[maybe_unused]] std::span<const ml::vec4> attribs,
      [[maybe_unused]] ml::vec4& gl_Position,
      [[maybe_unused]] float& gl_PointSize,
      [[maybe_unused]] std::span<float> gl_ClipDistance,
      [[maybe_unused]] std::span<ml::vec4> varyings) const override
    {
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      [[maybe_unused]] std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      [[maybe_unused]] ml::vec4& gl_FragColor) const override
    {
        return swr::fragment_shader_result::accept;
    }
};

struct SecondShader final
: public swr::program<SecondShader>
{
    static constexpr std::string_view name = "SecondShader";

    SecondShader() = default;

    virtual void pre_link(
      boost::container::static_vector<
        swr::interpolation_qualifier,
        swr::limits::max::varyings>& iqs) const override
    {
        iqs = {};
    }

    void vertex_shader(
      [[maybe_unused]] int gl_VertexID,
      [[maybe_unused]] int gl_InstanceID,
      [[maybe_unused]] std::span<const ml::vec4> attribs,
      [[maybe_unused]] ml::vec4& gl_Position,
      [[maybe_unused]] float& gl_PointSize,
      [[maybe_unused]] std::span<float> gl_ClipDistance,
      [[maybe_unused]] std::span<ml::vec4> varyings) const override
    {
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      [[maybe_unused]] std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      [[maybe_unused]] ml::vec4& gl_FragColor) const override
    {
        return swr::fragment_shader_result::accept;
    }
};

};    // namespace

TEST(MaterialManagerTests, Construction)
{
    task_system::TaskSystem task_system{1};

    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    ASSERT_NO_THROW(
      MaterialManager manager(
        task_system,
        device,
        shader_cache,
        shader_factory,
        texture_cache));
}

TEST(MaterialManagerTests, Load)
{
    task_system::TaskSystem task_system{1};

    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    shader_factory.register_shader<FirstShader>();

    MaterialManager manager{
      task_system,
      device,
      shader_cache,
      shader_factory,
      texture_cache};

    const std::string json =
      "{\n"
      "    \"shader\": \"FirstShader\"\n"
      "}";
    std::optional<ResolvableMaterial> result;
    ASSERT_NO_THROW(result.emplace(manager.load("FirstMaterial", json)));
    ASSERT_TRUE(result.has_value());

    ASSERT_TRUE(result.value()->valid());
    ASSERT_NO_THROW(result.value()->wait());
    EXPECT_EQ(result.value()->resolve(), 1);    // material ids start at 1

    const std::string unknown_shader =
      "{\n"
      "    \"shader\": \"Unknown\"\n"
      "}";
    ASSERT_NO_THROW(result.emplace(manager.load("UnknownMaterial", unknown_shader)));

    ASSERT_TRUE(result.value()->valid());
    ASSERT_NO_THROW(result.value()->wait());
    ASSERT_THROW(result.value()->resolve(), std::runtime_error);

    EXPECT_NO_THROW(manager.delete_material("FirstMaterial"));
}

TEST(MaterialManagerTests, LoadWithKey)
{
    task_system::TaskSystem task_system{1};

    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    shader_factory.register_shader<FirstShader>();

    MaterialManager manager{
      task_system,
      device,
      shader_cache,
      shader_factory,
      texture_cache};

    const std::string json =
      "{\n"
      "    \"shader\": \"FirstShader\"\n"
      "}";
    std::optional<ResolvableMaterial> handle;
    ASSERT_NO_THROW(handle.emplace(manager.load("FirstMaterial", json)));

    ASSERT_TRUE((*handle)->valid());
    ASSERT_NO_THROW((*handle)->wait());
    EXPECT_EQ((*handle)->resolve(), 1);    // material ids start at 1

    std::optional<ResolvableMaterial> result;
    ASSERT_NO_THROW(result = manager.get("FirstMaterial"));

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result.value()->valid());    // because it's already resolved
    EXPECT_TRUE(result.value()->is_resolved());
    EXPECT_EQ(result.value()->resolve(), 1);    // material ids start at 1

    ASSERT_NO_THROW(result = manager.get("UnknownShader"));
    EXPECT_FALSE(result.has_value());

    EXPECT_NO_THROW(manager.delete_material("FirstMaterial"));
    ASSERT_NO_THROW(result = manager.get("FirstMaterial"));
    EXPECT_FALSE(result.has_value());
}

TEST(MaterialManagerTests, Deduplicate)
{
    task_system::TaskSystem task_system{1};

    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    shader_factory.register_shader<FirstShader>();
    shader_factory.register_shader<SecondShader>();

    MaterialManager manager{
      task_system,
      device,
      shader_cache,
      shader_factory,
      texture_cache};

    const std::string json =
      "{\n"
      "    \"shader\": \"FirstShader\"\n"
      "}";
    std::optional<ResolvableMaterial> result;
    ASSERT_NO_THROW(result.emplace(manager.load("FirstMaterial", json)));

    ASSERT_TRUE(result.value()->valid());
    ASSERT_NO_THROW(result.value()->wait());
    EXPECT_EQ(result.value()->resolve(), 1);    // material ids start at 1

    ASSERT_NO_THROW(result.emplace(manager.load("FirstMaterial", json)));
    EXPECT_TRUE(result.value()->is_resolved());
    EXPECT_EQ(result.value()->resolve(), 1);

    const std::string json2 =
      "{\n"
      "    \"shader\": \"SecondShader\"\n"
      "}";
    ASSERT_NO_THROW(result.emplace(manager.load("SecondMaterial", json2)));

    ASSERT_TRUE(result.value()->valid());
    ASSERT_NO_THROW(result.value()->wait());
    EXPECT_FALSE(result.value()->is_resolved());

    EXPECT_EQ(result.value()->resolve(), 2);
    EXPECT_TRUE(result.value()->is_resolved());
}

TEST(MaterialManagerTests, LoadWithTextures)
{
    task_system::TaskSystem task_system{1};

    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    shader_factory.register_shader<FirstShader>();

    MaterialManager manager{
      task_system,
      device,
      shader_cache,
      shader_factory,
      texture_cache};

    const std::string json =
      "{\n"
      "    \"shader\": \"FirstShader\",\n"
      "    \"textures\": {\n"
      "        \"base_color\": {\n"
      "            \"path\": \"" ASSETS_SOURCE_DIR
      "/textures/tiles/tiles_0080_color_1k.png\",\n"
      "            \"color_space\": \"srgb\"\n"
      "        },\n"
      "        \"normal\": {\n"
      "            \"path\": \"" ASSETS_SOURCE_DIR
      "/textures/tiles/tiles_0080_normal_opengl_1k.png\",\n"
      "            \"convention\": \"opengl\"\n"
      "        }\n"
      "    }\n"
      "}";
    std::optional<ResolvableMaterial> result;
    ASSERT_NO_THROW(result.emplace(manager.load("FirstMaterial", json)));
    ASSERT_TRUE(result.value()->valid());
    ASSERT_NO_THROW(result.value()->wait());
    EXPECT_EQ(result.value()->resolve(), 1);    // material ids start at 1
}
