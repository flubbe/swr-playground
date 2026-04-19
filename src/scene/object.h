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

struct RenderData
{
    std::uint32_t mesh_handle{0};
    std::uint32_t material_handle{0};
};

/** Class info for RTTI-style object queries. */
struct ClassInfo
{
    std::string_view name;
    const ClassInfo* parent = nullptr;

    bool is_a(const ClassInfo* other) const
    {
        for(auto p = this; p != nullptr; p = p->parent)
        {
            if(p == other)
            {
                return true;
            }
        }
        return false;
    }
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
    /** RTTI-style type info. */
    ClassInfo class_info;

    /** object id. */
    ObjectId object_id{0};

    /** object name. */
    std::string name;

    /** object transformation matrix. */
    ml::mat4x4 transform{ml::mat4x4::identity()};

    /** meshes. */
    std::vector<RenderData> mesh_handles;

public:
    /** default constructor. */
    Object() = default;

    /** default destructor. */
    virtual ~Object() = default;

    /** initialize the object with a mesh. */
    Object(
      std::vector<RenderData> mesh_handles)
    : mesh_handles{std::move(mesh_handles)}
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
        return static_class();
    }

    static const ClassInfo* static_class()
    {
        static const ClassInfo cls{
          .name = "Object",
          .parent = nullptr};
        return &cls;
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

#define DECLARE_CLASS(Type, Base)               \
public:                                         \
    using Super = Base;                         \
    static const ClassInfo* static_class();     \
    const ClassInfo* get_class() const override \
    {                                           \
        return Type::static_class();            \
    }

#define DEFINE_CLASS(Type)                                              \
    const ClassInfo* Type::static_class()                               \
    {                                                                   \
        static const ClassInfo cls{#Type, Type::Super::static_class()}; \
        return &cls;                                                    \
    }
