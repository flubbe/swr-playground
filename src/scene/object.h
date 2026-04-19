/**
 * Software Rasterizer Playground.
 *
 * An object in the scene.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <memory>

#include "ml/all.h"

struct MeshHandle
{
    std::uint32_t mesh_handle{0};
    std::uint32_t material_handle{0};
};

class Object
{
    /** object transformation matrix. */
    ml::mat4x4 transform{ml::mat4x4::identity()};

    /** meshes. */
    std::vector<MeshHandle> mesh_handles;

public:
    /** default constructor. */
    Object() = default;

    /** default destructor. */
    virtual ~Object() = default;

    /** initialize the object with a mesh. */
    Object(std::vector<MeshHandle> mesh_handles)
    : mesh_handles{std::move(mesh_handles)}
    {
    }

    /** move data. */
    Object(Object&& other)
    : mesh_handles{std::move(other.mesh_handles)}
    {
    }

    Object(const Object&) = default;
    Object& operator=(const Object&) = default;

    /** release all data. */
    virtual void release()
    {
    }

    /** set the mesh. */
    void set_meshes(std::vector<MeshHandle> handles)
    {
        mesh_handles = std::move(handles);
    }

    /** clear the mesh. */
    void clear_mesh()
    {
        mesh_handles.clear();
    }

    /** get the mesh handle. */
    const std::vector<MeshHandle>& get_meshes() const
    {
        return mesh_handles;
    }

    /** whether the object is drawable. */
    virtual bool is_drawable() const
    {
        return !mesh_handles.empty();
    }

    /** update the object. */
    virtual void tick(
      [[maybe_unused]] float delta_time)
    {
    }

    /** Set the transformation matrix. */
    void set_transform(ml::mat4x4 m)
    {
        transform = m;
    }

    /** Return the transformation matrix. */
    ml::mat4x4 get_transform() const
    {
        return transform;
    }
};
