#include <ranges>

#include <swr/swr.h>
#include <swr/shaders.h>

#include <gtest/gtest.h>

#include "renderer/materialmanager.h"
#include "renderer/renderdevice.h"
#include "shader_cache.h"
#include "shader_factory.h"
#include "texture_cache.h"

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
    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    EXPECT_NO_THROW(
      MaterialManager manager(
        device,
        shader_cache,
        shader_factory,
        texture_cache));
}

TEST(MaterialManagerTests, Load)
{
    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    shader_factory.register_shader<FirstShader>();

    MaterialManager manager{
      device,
      shader_cache,
      shader_factory,
      texture_cache};

    const std::string json =
      "{\n"
      "    \"shader\": \"FirstShader\"\n"
      "}";
    std::pair<MaterialHandle, swr::string> result;
    EXPECT_NO_THROW(result = manager.load(json));
    EXPECT_EQ(result.first.value, 1);                       // material ids start at 1
    EXPECT_EQ(result.second, "hash://e91290016cb6df0b");    // hash of the JSON

    const std::string unknown_shader =
      "{\n"
      "    \"shader\": \"Unknown\"\n"
      "}";
    EXPECT_THROW(
      manager.load(unknown_shader),
      std::runtime_error);
}

TEST(MaterialManagerTests, LoadWithKey)
{
    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    shader_factory.register_shader<FirstShader>();

    MaterialManager manager{
      device,
      shader_cache,
      shader_factory,
      texture_cache};

    const std::string json =
      "{\n"
      "    \"shader\": \"FirstShader\"\n"
      "}";
    MaterialHandle handle;
    EXPECT_NO_THROW(handle = manager.load("FirstShader", json));
    EXPECT_EQ(handle.value, 1);    // material ids start at 1

    std::optional<MaterialHandle> result;
    EXPECT_NO_THROW(result = manager.get("FirstShader"));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().value, 1);    // material ids start at 1

    EXPECT_NO_THROW(result = manager.get("UnknownShader"));
    ASSERT_FALSE(result.has_value());
}

TEST(MaterialManagerTests, Deduplicate)
{
    RenderDevice device{100, 100};
    ShaderFactory shader_factory;

    ShaderCache shader_cache{device};
    TextureCache texture_cache{device};

    shader_factory.register_shader<FirstShader>();
    shader_factory.register_shader<SecondShader>();

    MaterialManager manager{
      device,
      shader_cache,
      shader_factory,
      texture_cache};

    const std::string json =
      "{\n"
      "    \"shader\": \"FirstShader\"\n"
      "}";
    std::pair<MaterialHandle, swr::string> result;
    EXPECT_NO_THROW(result = manager.load(json));
    EXPECT_EQ(result.first.value, 1);                       // material ids start at 1
    EXPECT_EQ(result.second, "hash://e91290016cb6df0b");    // hash of the JSON

    EXPECT_NO_THROW(result = manager.load(json));
    EXPECT_EQ(result.first.value, 1);
    EXPECT_EQ(result.second, "hash://e91290016cb6df0b");

    const std::string json2 =
      "{\n"
      "    \"shader\": \"SecondShader\"\n"
      "}";
    EXPECT_NO_THROW(result = manager.load(json2));
    EXPECT_EQ(result.first.value, 2);
    EXPECT_NE(result.second, "hash://e91290016cb6df0b");
}
