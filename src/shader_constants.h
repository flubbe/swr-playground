/**
 * Software Rasterizer Playground.
 *
 * Shared shader uniform layout constants.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>

namespace shader
{

constexpr std::size_t camera_projection_uniform_index = 0;
constexpr std::size_t camera_view_uniform_index = 1;
constexpr std::size_t directional_light_count_uniform_index = 2;
constexpr std::size_t directional_light_uniform_base_index = 3;

constexpr std::size_t max_lights = 2;
constexpr std::size_t max_spot_lights = 1;
constexpr std::size_t spot_light_uniform_stride = 4;

constexpr std::size_t spot_light_count_uniform_index =
  directional_light_uniform_base_index + max_lights;
constexpr std::size_t spot_light_uniform_base_index =
  spot_light_count_uniform_index + 1;
constexpr std::size_t material_color_uniform_index =
  spot_light_uniform_base_index + max_spot_lights * spot_light_uniform_stride;
constexpr std::size_t shadow_map_enabled_uniform_index =
  material_color_uniform_index + 1;
constexpr std::size_t shadow_map_matrix_uniform_index =
  shadow_map_enabled_uniform_index + 1;
constexpr std::size_t shadow_map_params_uniform_index =
  shadow_map_matrix_uniform_index + 1;

constexpr std::size_t spot_light_position_uniform_offset = 0;
constexpr std::size_t spot_light_direction_uniform_offset = 1;
constexpr std::size_t spot_light_params_uniform_offset = 2;
constexpr std::size_t spot_light_color_uniform_offset = 3;
constexpr std::size_t shadow_map_sampler_unit = 2;

constexpr std::size_t directional_light_uniform_index(
  std::size_t light_index) noexcept
{
    return directional_light_uniform_base_index + light_index;
}

constexpr std::size_t spot_light_uniform_index(
  std::size_t light_index,
  std::size_t field_offset) noexcept
{
    return spot_light_uniform_base_index
           + light_index * spot_light_uniform_stride
           + field_offset;
}

}    // namespace shader
