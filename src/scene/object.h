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
#include <vector>

#include "class_registry.h"
#include "ml/all.h"
#include "property.h"

struct RenderData
{
    std::uint32_t mesh_handle{0};
    std::uint32_t material_handle{0};
};

struct ObjectId
{
    std::uint32_t value = 0;

    bool operator==(const ObjectId& other) const noexcept
    {
        return value == other.value;
    }
    bool operator!=(const ObjectId& other) const noexcept
    {
        return !(*this == other);
    }
};

inline ObjectId make_object_id(std::uint32_t value)
{
    return {value};
}

class Object
{
    DECLARE_ROOT_CLASS(Object);

    /** RTTI-style type info. */
    const ClassInfo* class_info{Object::static_class()};

    /** object id. */
    ObjectId object_id{0};

    /** object name. */
    std::string name;

    /** object transformation matrix. */
    ml::mat4x4 transform{ml::mat4x4::identity()};

    /** meshes. */
    std::vector<RenderData> mesh_handles;

protected:
    Object(
      const ClassInfo* class_info,
      std::vector<RenderData> mesh_handles = {})
    : class_info{class_info}
    , mesh_handles{std::move(mesh_handles)}
    {
    }

    void set_class_info(const ClassInfo* class_info) noexcept
    {
        this->class_info = class_info;
    }

public:
    using PropertyList = std::vector<std::unique_ptr<Property>>;

    static std::uint32_t* property_access_object_id_value(Object& object) noexcept
    {
        return &object.object_id.value;
    }

    static std::string* property_access_name(Object& object) noexcept
    {
        return &object.name;
    }

    /** default constructor. */
    Object() = default;

    /** default destructor. */
    virtual ~Object() = default;

    /** initialize the object with a mesh. */
    Object(
      std::vector<RenderData> mesh_handles)
    : Object{
        Object::static_class(),
        std::move(mesh_handles)}
    {
    }

    /** move data. */
    Object(Object&& other)
    : class_info{other.class_info}
    , object_id{other.object_id}
    , name{std::move(other.name)}
    , mesh_handles{std::move(other.mesh_handles)}
    {
    }

    Object(const Object&) = default;
    Object& operator=(const Object&) = default;

    Object& operator=(Object&& other)
    {
        class_info = other.class_info;
        object_id = other.object_id;
        name = std::move(other.name);
        mesh_handles = std::move(other.mesh_handles);

        return *this;
    }

    ObjectId get_object_id() const noexcept
    {
        return object_id;
    }

    void set_object_id(ObjectId object_id) noexcept
    {
        this->object_id = object_id;
    }

    const std::string& get_name() const noexcept
    {
        return name;
    }

    void set_name(std::string object_name)
    {
        name = std::move(object_name);
    }

    virtual const ClassInfo* get_class() const
    {
        return class_info != nullptr
                 ? class_info
                 : static_class();
    }

    template<typename T>
    bool is_a() const
    {
        return get_class()->is_a(T::static_class());
    }

    bool is_a(const ClassInfo* cls) const
    {
        return get_class()->is_a(cls);
    }

    virtual PropertyList get_properties();

    /** release all data. */
    virtual void release()
    {
    }

    /** set the mesh. */
    void set_meshes(std::vector<RenderData> handles)
    {
        mesh_handles = std::move(handles);
    }

    /** clear the mesh. */
    void clear_mesh()
    {
        mesh_handles.clear();
    }

    /** get the mesh handle. */
    const std::vector<RenderData>& get_meshes() const
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
