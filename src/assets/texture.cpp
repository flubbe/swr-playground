/**
 * Software Rasterizer Playground.
 *
 * Texture asset loading helpers.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#include "assets/texture.h"

namespace assets
{

ImageRGBA8 load_texture_rgba8(
  const std::filesystem::path& path)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(
      path.string().c_str(),
      &width,
      &height,
      &channels,
      STBI_rgb_alpha);
    if(pixels == nullptr)
    {
        const char* failure_reason = stbi_failure_reason();
        throw std::runtime_error{
          std::format(
            "Unable to load texture '{}': {}",
            path.string(),
            (failure_reason != nullptr
               ? std::string{failure_reason}
               : "unknown stb_image error"))};
    }

    ImageRGBA8 image{
      .width = width,
      .height = height,
      .pixels = std::vector<std::uint8_t>{
        pixels,
        pixels + static_cast<std::size_t>(width * height * 4)},
    };
    stbi_image_free(pixels);

    return image;
}

ImageRGBA8 load_normal_map_rgba8(
  const std::filesystem::path& path,
  NormalMapConvention convention)
{
    ImageRGBA8 image = load_texture_rgba8(path);
    if(convention == NormalMapConvention::DirectX)
    {
        for(std::size_t pixel_index = 0;
            pixel_index + 3 < image.pixels.size();
            pixel_index += 4)
        {
            image.pixels[pixel_index + 1] =
              static_cast<std::uint8_t>(255 - image.pixels[pixel_index + 1]);
        }
    }

    return image;
}

}    // namespace assets
