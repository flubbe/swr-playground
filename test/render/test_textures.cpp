#include <gtest/gtest.h>

#include "renderer/render_device.h"
#include "texture_cache.h"

TEST(TextureCache, Construction)
{
    RenderDevice device{100, 100};
    ASSERT_NO_THROW(TextureCache cache(device));
}

TEST(TextureCache, LoadEmptyTexture)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    const auto image = assets::ImageRGBA8{
      .width = 0,
      .height = 0,
      .pixels = {}};

    ASSERT_THROW(cache.load(image), std::runtime_error);
}

TEST(TextureCache, LoadTexture)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    const auto image = assets::ImageRGBA8{
      .width = 1,
      .height = 1,
      .pixels = {0xAA, 0xBB, 0xCC, 0xDD}};

    std::optional<std::pair<TextureRef, std::string>> texture;
    ASSERT_NO_THROW(texture.emplace(cache.load(image)));
    ASSERT_TRUE(texture.has_value());

    ASSERT_EQ(texture.value().first.get(), 1);    // texture ids start at 1
    ASSERT_EQ(texture.value().second, "hash://bfd691c4f6750254");
}

TEST(TextureCache, LoadTwoTextures)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    const auto image0 = assets::ImageRGBA8{
      .width = 1,
      .height = 1,
      .pixels = {0xAA, 0xBB, 0xCC, 0xDD}};
    const auto image1 = assets::ImageRGBA8{
      .width = 1,
      .height = 1,
      .pixels = {0xAB, 0xBC, 0xCD, 0xDA}};

    std::optional<std::pair<TextureRef, std::string>> texture;
    ASSERT_NO_THROW(texture.emplace(cache.load(image0)));
    ASSERT_TRUE(texture.has_value());

    ASSERT_EQ(texture.value().first.get(), 1);    // texture ids start at 1
    ASSERT_EQ(texture.value().second, "hash://bfd691c4f6750254");

    ASSERT_NO_THROW(texture.emplace(cache.load(image1)));
    ASSERT_TRUE(texture.has_value());

    ASSERT_EQ(texture.value().first.get(), 2);
    ASSERT_NE(texture.value().second, "hash://bfd691c4f6750254");
}

TEST(TextureCache, Deduplicate)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    const auto image = assets::ImageRGBA8{
      .width = 1,
      .height = 1,
      .pixels = {0xAA, 0xBB, 0xCC, 0xDD}};

    std::optional<std::pair<TextureRef, std::string>> texture;
    ASSERT_NO_THROW(texture.emplace(cache.load(image)));
    ASSERT_TRUE(texture.has_value());

    ASSERT_EQ(texture.value().first.get(), 1);    // texture ids start at 1
    ASSERT_EQ(texture.value().second, "hash://bfd691c4f6750254");

    ASSERT_NO_THROW(texture.emplace(cache.load(image)));
    ASSERT_TRUE(texture.has_value());

    ASSERT_EQ(texture.value().first.get(), 1);    // texture ids start at 1
    ASSERT_EQ(texture.value().second, "hash://bfd691c4f6750254");
}

TEST(TextureCache, InvalidDimensions)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    {
        const auto image = assets::ImageRGBA8{
          .width = -1,
          .height = 1,
          .pixels = {0xAA, 0xBB, 0xCC, 0xDD}};

        std::optional<std::pair<TextureRef, std::string>> texture;
        EXPECT_THROW(texture.emplace(cache.load(image)), std::runtime_error);
    }
    {
        const auto image = assets::ImageRGBA8{
          .width = 1,
          .height = 0,
          .pixels = {0xAA, 0xBB, 0xCC, 0xDD}};

        std::optional<std::pair<TextureRef, std::string>> texture;
        EXPECT_THROW(texture.emplace(cache.load(image)), std::runtime_error);
    }
}

TEST(TextureCache, InvalidData)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    const auto image = assets::ImageRGBA8{
      .width = 1,
      .height = 1,
      .pixels = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}};

    std::optional<std::pair<TextureRef, std::string>> texture;
    EXPECT_THROW(texture.emplace(cache.load(image)), std::runtime_error);
}

TEST(TextureCache, ContainsGet)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    const auto image = assets::ImageRGBA8{
      .width = 1,
      .height = 1,
      .pixels = {0xAA, 0xBB, 0xCC, 0xDD}};

    std::optional<std::pair<TextureRef, swr::string>> retained_texture;    // keeps the cache entry valid.
    ASSERT_NO_THROW(retained_texture.emplace(cache.load(image)));

    std::optional<TextureRef> texture;
    ASSERT_NO_THROW(texture = cache.get("hash://bfd691c4f6750254"));
    ASSERT_TRUE(texture.has_value());
    EXPECT_EQ(texture.value().get().value, 1);
}

TEST(TextureCache, Delete)
{
    RenderDevice device{100, 100};
    TextureCache cache(device);

    const auto image = assets::ImageRGBA8{
      .width = 1,
      .height = 1,
      .pixels = {0xAA, 0xBB, 0xCC, 0xDD}};

    std::optional<std::__1::pair<TextureRef, swr::string>> retained_texture;    // keeps the cache entry valid.
    ASSERT_NO_THROW(retained_texture.emplace(cache.load(image)));

    ASSERT_NO_THROW(cache.delete_texture("hash://bfd691c4f6750254"));
    ASSERT_FALSE(cache.get("hash://bfd691c4f6750254").has_value());
}
