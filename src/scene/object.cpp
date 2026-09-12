/**
 * Software Rasterizer Playground.
 *
 * Object implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <ranges>
#include <string_view>

#include "reflection/builtin_properties.h"
#include "scene/properties.h"
#include "object.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Object);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

namespace
{

/**
 * Copy a reflected property value between same-typed property wrappers.
 *
 * @param dst Destination object.
 * @param src Source object.
 * @param prop Property to copy.
 */
void copy_property_value(
  Object& dst,
  const Object& src,
  const reflect::Property& prop)
{
    auto* dst_address =
      reinterpret_cast<std::byte*>(&dst) + prop.get_offset();
    auto* src_address =
      reinterpret_cast<const std::byte*>(&src) + prop.get_offset();

    prop.copy_value(dst_address, src_address);
}

/**
 * Copy one reflected property value by property name between objects.
 *
 * @param dst_obj Destination object whose property is written.
 * @param src_obj Source object whose property value is read.
 * @param property_name Internal reflected property name.
 * @returns `true` if a matching property exists on both objects and was copied.
 */
bool copy_property_by_name(
  Object& dst_obj,
  const Object& src_obj,
  std::string_view property_name)
{
    auto& dst_props = dst_obj.get_properties();
    const auto& src_props = src_obj.get_properties();

    const auto prop_it = std::ranges::find_if(
      dst_props,
      [property_name](const swr::unique_ptr<reflect::Property>& property)
      {
          return property != nullptr && property->get_name() == property_name;
      });
    const auto src_it = std::ranges::find_if(
      src_props,
      [property_name](const swr::unique_ptr<reflect::Property>& property)
      {
          return property != nullptr && property->get_name() == property_name;
      });

    if(prop_it == dst_props.end() || *prop_it == nullptr
       || src_it == src_props.end() || *src_it == nullptr)
    {
        return false;
    }

    if((*prop_it)->get_offset() != (*src_it)->get_offset())
    {
        throw std::runtime_error{
          std::format(
            "Trying to copy property '{}' between incompatible objects '{} {}', '{} {}'.",
            property_name,
            dst_obj.get_class()
              ? dst_obj.get_class()->name
              : swr::string{"<none>"},
            dst_obj.get_name(),
            src_obj.get_class()
              ? src_obj.get_class()->name
              : swr::string{"<none>"},
            src_obj.get_name())};
    }

    copy_property_value(
      dst_obj,
      src_obj,
      *(*prop_it));

    return true;
}

}    // namespace

void Object::register_properties(reflect::ClassInfo& class_info)
{
    register_property<&Object::object_id>(
      class_info,
      "object_id",
      "Object ID",
      reflect::PropertyFlags::ReadOnly);
    register_property<&Object::name>(
      class_info,
      "name",
      "Name");
    register_property<&Object::transform>(
      class_info,
      "transform",
      "Transform");
    register_property<&Object::visible>(
      class_info,
      "visible",
      "Visible");
}

void Object::capture_snapshot()
{
    const reflect::ClassInfo* cls = get_class();
    if(cls == nullptr
       || cls->factory == nullptr)
    {
        snapshot.reset();
        return;
    }

    snapshot.reset(static_cast<Object*>(cls->factory()));
    if(snapshot == nullptr)
    {
        return;
    }

    for(const auto& prop: get_properties())
    {
        if(prop == nullptr)
        {
            throw std::runtime_error{
              std::format(
                "Null property in object '{} {}'.",
                get_class()->name,
                get_name())};
        }

        copy_property_value(
          *snapshot,
          *this,
          *prop);
    }
}

bool Object::has_property_snapshot(std::string_view property_name) const
{
    if(snapshot == nullptr)
    {
        return false;
    }

    const auto& snap_props = snapshot->get_properties();
    return std::ranges::any_of(
      snap_props,
      [property_name](const swr::unique_ptr<reflect::Property>& property)
      {
          return property != nullptr
                 && property->get_name() == property_name;
      });
}

bool Object::reset_property_to_snapshot(std::string_view property_name)
{
    if(snapshot == nullptr)
    {
        return false;
    }
    const bool changed = copy_property_by_name(*this, *snapshot, property_name);
    if(changed)
    {
        on_properties_changed();
    }
    return changed;
}

bool Object::reset_to_snapshot()
{
    if(snapshot == nullptr)
    {
        return false;
    }

    bool success = true;
    for(const auto& prop: snapshot->get_properties())
    {
        success &= reset_property_to_snapshot(prop->get_name());
    }
    return success;
}
