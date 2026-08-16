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
#include <string_view>
#include <utility>

#include <ml/all.h>

#include "containers/memory.h"
#include "containers/string.h"
#include "reflection/class_registry.h"
#include "reflection/property.h"

struct ObjectId
{
    using Type = unsigned int;

    Type value = 0;

    bool operator==(const ObjectId& other) const noexcept
    {
        return value == other.value;
    }
    bool operator!=(const ObjectId& other) const noexcept
    {
        return !(*this == other);
    }
};

// std::hash support.

namespace std
{

template<>
struct hash<ObjectId>
{
    std::size_t operator()(const ObjectId& id) const noexcept
    {
        return std::hash<ObjectId::Type>{}(id.value);
    }
};

}    // namespace std

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
public:
    static void register_properties(reflect::ClassInfo& class_info);

    /** object id. */
    ObjectId object_id{0};

    /** object name. */
    swr::string name;

    /** object transformation matrix. */
    ml::mat4x4 transform{ml::mat4x4::identity()};

    /** Whether the object should be rendered when supported by the renderer. */
    bool visible{true};

protected:
    /** per-instance baseline snapshot object. */
    swr::unique_ptr<Object> snapshot;

protected:
    explicit Object(
      const reflect::ClassInfo* class_info)
    : reflect::ReflectRoot<Object>{class_info}
    {
    }

    void set_class_info(const reflect::ClassInfo* class_info) noexcept
    {
        this->class_info = class_info;
    }

public:
    /** Default constructor. */
    Object()
    : reflect::ReflectRoot<Object>{Object::static_class()}
    {
    }

    /** Default destructor. */
    virtual ~Object() = default;

    /** Move constructor. */
    Object(Object&& other)
    : reflect::ReflectRoot<Object>{std::move(other)}
    , object_id{other.object_id}
    , name{std::move(other.name)}
    , transform{other.transform}
    , visible{other.visible}
    {
        other.class_info = nullptr;
    }

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    Object& operator=(Object&& other)
    {
        static_cast<ReflectRoot<Object>&>(*this) = std::move(other);

        object_id = other.object_id;
        name = std::move(other.name);
        transform = other.transform;
        visible = other.visible;

        return *this;
    }

    /** Called after object loading/construction. */
    virtual void post_load()
    {
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
    void set_object_id(
      ObjectId object_id) noexcept
    {
        this->object_id = object_id;
    }

    /** Get the object's name. */
    const swr::string& get_name() const noexcept
    {
        return name;
    }

    /**
     * Set the object's name.
     *
     * @param object_name The new object name.
     */
    void set_name(
      std::string_view object_name)
    {
        name = object_name;
    }

    /** Release all data. */
    virtual void release()
    {
    }

    /** Update the object. */
    virtual void tick(
      [[maybe_unused]] float delta_time)
    {
    }

    /** Called after one or more reflected properties have been modified. */
    virtual void on_properties_changed()
    {
    }

    /** Set the transformation matrix. */
    void set_transform(
      ml::mat4x4 m)
    {
        transform = m;
    }

    /** Return the transformation matrix. */
    ml::mat4x4 get_transform() const
    {
        return transform;
    }

    /** Set whether this object should be rendered when supported by the renderer. */
    void set_visible(
      bool in_visible) noexcept
    {
        visible = in_visible;
    }

    /** Return whether this object should be rendered when supported by the renderer. */
    [[nodiscard]]
    bool is_visible() const noexcept
    {
        return visible;
    }

    /** Set the object's world position while preserving the rest of the transform. */
    void set_position(
      const ml::vec3& position)
    {
        transform.rows[0].w = position.x;
        transform.rows[1].w = position.y;
        transform.rows[2].w = position.z;
    }

    /** Return the object's world position. */
    ml::vec3 get_position() const
    {
        return {
          transform.rows[0].w,
          transform.rows[1].w,
          transform.rows[2].w};
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
    bool has_property_snapshot(
      std::string_view property_name) const;

    /**
     * Reset one reflected property to its captured baseline value.
     *
     * @param property_name Name of the property.
     * @returns `true` if the property was reset to a snapshot.
     */
    bool reset_property_to_snapshot(
      std::string_view property_name);

    /**
     * Reset all properties to their snapshot.
     *
     * @returns `true` if all properties were reset.
     */
    bool reset_to_snapshot();
};

DECLARE_REFLECTION(Scene, Object);
