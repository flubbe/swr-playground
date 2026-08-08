#include <ranges>

#include <gtest/gtest.h>

#include "shader_cache.h"

namespace
{

struct TestShader final
: public swr::program<TestShader>
{
    static constexpr std::string_view name = "TestShader";

    TestShader() = default;

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

struct SecondTestShader final
: public swr::program<SecondTestShader>
{
    static constexpr std::string_view name = "SecondTestShader";

    int some_value{0};    // changes the size of the shader.

    SecondTestShader() = default;

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
        return swr::fragment_shader_result::discard;
    }
};

};    // namespace

TEST(ShaderCacheTests, Construction)
{
    EXPECT_NO_THROW(
      ShaderCache cache);
}

TEST(ShaderCacheTests, Registration)
{
    ShaderCache cache;

    EXPECT_TRUE(
      cache.register_shader<TestShader>());
    EXPECT_FALSE(
      cache.register_shader<TestShader>());
}

TEST(ShaderCacheTests, Names)
{
    std::vector<std::string> names;
    ShaderCache cache;

    EXPECT_TRUE(
      cache.register_shader<TestShader>());
    names = cache.get_names();
    ASSERT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], TestShader::name);

    EXPECT_TRUE(
      cache.register_shader<SecondTestShader>());
    names = cache.get_names();
    std::ranges::sort(names);
    ASSERT_EQ(names.size(), 2);
    EXPECT_EQ(names[0], SecondTestShader::name);
    EXPECT_EQ(names[1], TestShader::name);

    EXPECT_FALSE(
      cache.register_shader<TestShader>());
    names = cache.get_names();
    std::ranges::sort(names);
    ASSERT_EQ(names.size(), 2);
    EXPECT_EQ(names[0], SecondTestShader::name);
    EXPECT_EQ(names[1], TestShader::name);

    EXPECT_FALSE(
      cache.register_shader<SecondTestShader>());
    names = cache.get_names();
    std::ranges::sort(names);
    ASSERT_EQ(names.size(), 2);
    EXPECT_EQ(names[0], SecondTestShader::name);
    EXPECT_EQ(names[1], TestShader::name);
}

TEST(ShaderCacheTests, GetInstance)
{
    static_assert(sizeof(TestShader) != sizeof(SecondTestShader));

    std::vector<std::string> names;
    ShaderCache cache;

    float fragDepth{0.f};
    ml::vec4 fragColor;

    EXPECT_TRUE(
      cache.register_shader<TestShader>());
    EXPECT_TRUE(
      cache.register_shader<SecondTestShader>());

    auto* test_instance = cache.get_or_create<TestShader>();
    EXPECT_EQ(test_instance->size(), sizeof(TestShader));
    EXPECT_EQ(test_instance->fragment_shader(
                {},
                true,
                {},
                {},
                fragDepth,
                fragColor),
              swr::fragment_shader_result::accept);

    auto* second_instance = cache.get_or_create<SecondTestShader>();
    EXPECT_EQ(second_instance->size(), sizeof(SecondTestShader));
    EXPECT_EQ(second_instance->fragment_shader(
                {},
                true,
                {},
                {},
                fragDepth,
                fragColor),
              swr::fragment_shader_result::discard);
}

TEST(ShaderCacheTests, GetInstanceByName)
{
    static_assert(sizeof(TestShader) != sizeof(SecondTestShader));

    std::vector<std::string> names;
    ShaderCache cache;

    float fragDepth{0.f};
    ml::vec4 fragColor;

    EXPECT_TRUE(
      cache.register_shader<TestShader>());
    EXPECT_TRUE(
      cache.register_shader<SecondTestShader>());

    auto* test_instance = cache.get(TestShader::name);
    EXPECT_EQ(test_instance->size(), sizeof(TestShader));
    EXPECT_EQ(test_instance->fragment_shader(
                {},
                true,
                {},
                {},
                fragDepth,
                fragColor),
              swr::fragment_shader_result::accept);

    auto* second_instance = cache.get(SecondTestShader::name);
    EXPECT_EQ(second_instance->size(), sizeof(SecondTestShader));
    EXPECT_EQ(second_instance->fragment_shader(
                {},
                true,
                {},
                {},
                fragDepth,
                fragColor),
              swr::fragment_shader_result::discard);
}