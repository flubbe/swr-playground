/**
 * Software Rasterizer Playground.
 *
 * A mesh.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <vector>

struct Mesh
{
    /** index buffer. */
    std::vector<std::uint32_t> index_buffer;

    /** vertex buffer id. */
    std::uint32_t vertex_buffer_id{0};

    /** normal buffer id. */
    std::uint32_t normal_buffer_id{0};

    /** remember if we still store data. */
    bool has_data{false};
};
