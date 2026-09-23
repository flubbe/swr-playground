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
#include "reflection/construct.h"
#include "reflection/property.h"

/*
 * Forward declarations.
 */

struct AssetResolver;
class Object;
class Scene;

/** An object identifier. */
struct ObjectId
{
    std::uint32_t value = 0;

    bool operator==(const ObjectId& other) const noexcept = default;
};

// std::hash support.

namespace std
{

template<>
struct hash<ObjectId>
{
    std::size_t operator()(const ObjectId& id) const noexcept
    {
        return std::hash<decltype(ObjectId::value)>{}(id.value);
    }
};

}    // namespace std

namespace reflect
{

/*
 * Allocator customization.
 */

#if SWR_USE_CUSTOM_STD_ALLOCATORS

template<>
struct Allocation<Object>
{
    static void* allocate(
      std::size_t size,
      std::size_t alignment)
    {
        return memory::heap().allocate(
          size,
          alignment,
          memory::MemoryTag::Object);
    }

    static void deallocate(
      void* p,
      std::size_t size,
      std::size_t alignment) noexcept
    {
        return memory::heap().deallocate(
          p,
          size,
          alignment,
          memory::MemoryTag::Object);
    }
};

#endif /* SWR_USE_CUSTOM_STD_ALLOCATORS */

/*
 * Property support.
 */

template<>
struct UnwrapType<ObjectId>
{
    using ValueType = decltype(ObjectId::value);

    static ValueType& get(ObjectId& value) noexcept
    {
        return value.value;
    }
};

}    // namespace reflect

/** Create an object id from a value. */
inline ObjectId make_object_id(
  std::uint32_t value)
{
    return {value};
}

/** Scene object base class. */
class Object
: public reflect::ReflectRoot<Object>
{
public:
    /** Property registration hook. */
    static void register_properties(
      reflect::ClassInfo& class_info);

    /** Object id. */
    ObjectId object_id{0};

    /** Object name. */
    swr::string name;

    /** Object transformation matrix. */
    ml::mat4x4 transform{ml::mat4x4::identity()};

    /** Whether the object should be rendered when supported by the renderer. */
    bool visible{true};

protected:
    /** Containing scene. */
    Scene* scene{nullptr};

    /** Per-instance baseline snapshot object. */
    reflect::unique_ptr<Object> snapshot;

public:
    /** Default constructor. */
    Object()
    : reflect::ReflectRoot<Object>{
        Object::static_class()}
    {
    }

    /** Default destructor. */
    virtual ~Object() = default;

    /**
     * Move constructor.
     *
     * FIXME Should be `noexcept`, but `ReflectRoot` can throw
     *     (which it likely shouldn't do).
     */
    Object(Object&& other)
    : reflect::ReflectRoot<Object>{std::move(other)}
    , object_id{other.object_id}
    , name{std::move(other.name)}
    , transform{other.transform}
    , visible{other.visible}
    , scene{other.scene}
    , snapshot{std::move(other.snapshot)}
    {
        other.class_info = nullptr;
        other.scene = nullptr;
    }

    /** Disable copy construction. */
    Object(const Object&) = delete;

    /** Disable copy assignment. */
    Object& operator=(const Object&) = delete;

    /** Move assignment. */
    Object& operator=(Object&& other)
    {
        static_cast<ReflectRoot<Object>&>(*this) = std::move(other);

        object_id = other.object_id;
        name = std::move(other.name);
        transform = other.transform;
        visible = other.visible;
        scene = other.scene;
        snapshot = std::move(other.snapshot);

        other.class_info = nullptr;
        other.scene = nullptr;

        return *this;
    }

    /**
     * Called after object data loading is complete.
     * Resolves object dependencies (e.g. resources/assets).
     */
    virtual void resolve(
      [[maybe_unused]] AssetResolver& resolver)
    {
    }

    /** Called after object dependency resolution. */
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

    /**
     * Set the containing scene.
     *
     * @param scene The containing scene.
     */
    void set_scene(
      Scene* scene) noexcept
    {
        this->scene = scene;
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
