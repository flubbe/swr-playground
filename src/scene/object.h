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

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ml/all.h"

#include "reflection/class_registry.h"
#include "reflection/property.h"

struct RenderData
{
    std::uint32_t mesh_handle{0};
    std::uint32_t material_handle{0};
};

struct ObjectId
{
    unsigned int value = 0;

    bool operator==(const ObjectId& other) const noexcept
    {
        return value == other.value;
    }
    bool operator!=(const ObjectId& other) const noexcept
    {
        return !(*this == other);
    }
};

// Property support.

namespace reflect
{

template<>
struct UnwrapType<ObjectId>
{
    using ValueType = unsigned int;

    static ValueType& get(ObjectId& value) noexcept
    {
        return value.value;
    }
};

}    // namespace reflect

inline ObjectId make_object_id(std::uint32_t value)
{
    return {value};
}

class Object
: public reflect::ReflectRoot<Object>
{
private:
    /** meshes. */
    std::vector<RenderData> mesh_handles;

protected:
    /** per-instance baseline snapshot object. */
    std::unique_ptr<Object> snapshot;

public:
    static void register_properties(reflect::ClassInfo& class_info);

    /** object transformation matrix. */
    ml::mat4x4 transform{ml::mat4x4::identity()};

    /** object id. */
    ObjectId object_id{0};

    /** object name. */
    std::string name;

protected:
    Object(
      const reflect::ClassInfo* class_info,
      std::vector<RenderData> mesh_handles = {})
    : reflect::ReflectRoot<Object>{class_info}
    , mesh_handles{std::move(mesh_handles)}
    {
    }

    void set_class_info(const reflect::ClassInfo* class_info) noexcept
    {
        this->class_info = class_info;
    }

public:
    /** Default constructor. */
    Object() = default;

    /** Default destructor. */
    virtual ~Object() = default;

    /** initialize the object with a mesh. */
    Object(
      std::vector<RenderData> mesh_handles)
    : Object{
        Object::static_class(),
        std::move(mesh_handles)}
    {
    }

    /** Move constructor. */
    Object(Object&& other)
    : reflect::ReflectRoot<Object>{std::move(other)}
    , mesh_handles{std::move(other.mesh_handles)}
    , object_id{other.object_id}
    , name{std::move(other.name)}
    {
        other.class_info = nullptr;
    }

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    Object& operator=(Object&& other)
    {
        static_cast<ReflectRoot<Object>&>(*this) = std::move(other);

        mesh_handles = std::move(other.mesh_handles);
        object_id = other.object_id;
        name = std::move(other.name);

        return *this;
    }

    /** Return the object id. */
    ObjectId get_object_id() const noexcept
    {
        return object_id;
    }

    /**
     * Set the object id.
     *
     * @param object_id The new object id.
     */
    void set_object_id(ObjectId object_id) noexcept
    {
        this->object_id = object_id;
    }

    /** Get the object's name. */
    const std::string& get_name() const noexcept
    {
        return name;
    }

    /**
     * Set the object's name.
     *
     * @param object_name The new object name.
     */
    void set_name(std::string object_name)
    {
        name = std::move(object_name);
    }

    /** Release all data. */
    virtual void release()
    {
    }

    /**
     * Set the mesh.
     *
     * @param handles The new mesh handles.
     */
    void set_meshes(std::vector<RenderData> handles)
    {
        mesh_handles = std::move(handles);
    }

    /** Clear the mesh. */
    void clear_mesh()
    {
        mesh_handles.clear();
    }

    /** Get the mesh handle. */
    const std::vector<RenderData>& get_meshes() const
    {
        return mesh_handles;
    }

    /** Whether the object is drawable. */
    virtual bool is_drawable() const
    {
        return !mesh_handles.empty();
    }

    /** Update the object. */
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

    /*
     * Snapshots.
     */

    /** Capture current reflected values as this instance's reset baseline. */
    void capture_snapshot();

    /**
     * Return whether a reflected property has a captured reset baseline.
     *
     * @param property_name Name of the property.
     * @returns `true` if the property has a snapshot.
     */
    bool has_property_snapshot(std::string_view property_name) const;

    /**
     * Reset one reflected property to its captured baseline value.
     *
     * @param property_name Name of the property.
     * @returns `true` if the property was reset to a snapshot.
     */
    bool reset_property_to_snapshot(std::string_view property_name);

    /**
     * Reset all properties to their snapshot.
     *
     * @returns `true` if all properties were reset.
     */
    bool reset_to_snapshot();
};

DECLARE_REFLECTION(Scene, Object);
