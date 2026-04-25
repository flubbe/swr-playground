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

template<>
struct PropertyTypeMap<ObjectId>
{
    using type = UIntProperty;
};

template<>
struct UnwrapType<ObjectId>
{
    using Type = unsigned int;

    static Type& get(ObjectId& value) noexcept
    {
        return value.value;
    }
};

inline ObjectId make_object_id(std::uint32_t value)
{
    return {value};
}

class Object
{
    DECLARE_ROOT_CLASS(Scene, Object);

    /** RTTI-style type info. */
    const ClassInfo* class_info{Object::static_class()};

    /** Reflected properties, filled in by `initialize_properties`. */
    std::vector<std::unique_ptr<Property>> properties;

    /** meshes. */
    std::vector<RenderData> mesh_handles;

public:
    /** object transformation matrix. */
    ml::mat4x4 transform{ml::mat4x4::identity()};

    /** object id. */
    ObjectId object_id{0};

    /** object name. */
    std::string name;

protected:
    Object(
      const ClassInfo* class_info,
      std::vector<RenderData> mesh_handles = {})
    : class_info{class_info}
    , mesh_handles{std::move(mesh_handles)}
    {
        initialize_properties();
    }

    void set_class_info(const ClassInfo* class_info) noexcept
    {
        this->class_info = class_info;
    }

    void initialize_properties();

public:
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
        initialize_properties();
    }

    /** move data. */
    Object(Object&& other)
    : class_info{other.class_info}
    , properties{std::move(other.properties)}
    , mesh_handles{std::move(other.mesh_handles)}
    , object_id{other.object_id}
    , name{std::move(other.name)}
    {
        other.class_info = nullptr;
    }

    Object(const Object&) = default;
    Object& operator=(const Object&) = default;

    Object& operator=(Object&& other)
    {
        properties = std::move(other.properties);
        class_info = other.class_info;
        mesh_handles = std::move(other.mesh_handles);
        object_id = other.object_id;
        name = std::move(other.name);

        other.class_info = nullptr;

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

    std::vector<
      std::unique_ptr<Property>>&
      get_properties()
    {
        return properties;
    }

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
