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
 * @param dst Destination property wrapper.
 * @param src Source property wrapper.
 * @returns `true` if both wrappers have the same supported type and the value was written.
 */
bool copy_property_value(
  reflect::Property& dst,
  const reflect::Property& src)
{
    if(auto* p = dst.try_as<reflect::IntProperty>())
    {
        const auto* s = src.try_as<reflect::IntProperty>();
        return s != nullptr && p->set_value(s->get_value());
    }
    if(auto* p = dst.try_as<reflect::UIntProperty>())
    {
        const auto* s = src.try_as<reflect::UIntProperty>();
        return s != nullptr && p->set_value(s->get_value());
    }
    if(auto* p = dst.try_as<reflect::FloatProperty>())
    {
        const auto* s = src.try_as<reflect::FloatProperty>();
        return s != nullptr && p->set_value(s->get_value());
    }
    if(auto* p = dst.try_as<reflect::BoolProperty>())
    {
        const auto* s = src.try_as<reflect::BoolProperty>();
        return s != nullptr && p->set_value(s->get_value());
    }
    if(auto* p = dst.try_as<reflect::StringProperty>())
    {
        const auto* s = src.try_as<reflect::StringProperty>();
        return s != nullptr && p->set_value(s->get_value());
    }
    if(auto* p = dst.try_as<reflect::SwrStringProperty>())
    {
        const auto* s = src.try_as<reflect::SwrStringProperty>();
        return s != nullptr && p->set_value(s->get_value());
    }
    if(auto* p = dst.try_as<reflect::Vec4Property>())
    {
        const auto* s = src.try_as<reflect::Vec4Property>();
        return s != nullptr && p->set_value(s->get_value());
    }
    if(auto* p = dst.try_as<reflect::Mat4Property>())
    {
        const auto* s = src.try_as<reflect::Mat4Property>();
        return s != nullptr && p->set_value(s->get_value());
    }

    return false;
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

    const auto dst_it = std::ranges::find_if(
      dst_props,
      [property_name](const std::unique_ptr<reflect::Property>& property)
      {
          return property != nullptr && property->get_name() == property_name;
      });
    const auto src_it = std::ranges::find_if(
      src_props,
      [property_name](const std::unique_ptr<reflect::Property>& property)
      {
          return property != nullptr && property->get_name() == property_name;
      });

    if(dst_it == dst_props.end() || *dst_it == nullptr
       || src_it == src_props.end() || *src_it == nullptr)
    {
        return false;
    }

    return copy_property_value(*(*dst_it), *(*src_it));
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
    if(cls == nullptr || cls->factory == nullptr)
    {
        snapshot.reset();
        return;
    }

    snapshot.reset(static_cast<Object*>(cls->factory()));
    if(snapshot == nullptr)
    {
        return;
    }

    auto& snap_props = snapshot->get_properties();
    const auto& cur_props = get_properties();
    for(const auto& src_property: cur_props)
    {
        if(src_property == nullptr)
        {
            continue;
        }
        const auto snap_it = std::ranges::find_if(
          snap_props,
          [&src_property](const std::unique_ptr<reflect::Property>& property)
          {
              return property != nullptr
                     && property->get_name() == src_property->get_name();
          });
        if(snap_it == snap_props.end() || *snap_it == nullptr)
        {
            continue;
        }
        copy_property_value(*(*snap_it), *src_property);
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
      [property_name](const std::unique_ptr<reflect::Property>& property)
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
        if(prop->is_read_only())
        {
            continue;
        }
        success &= reset_property_to_snapshot(prop->get_name());
    }
    return success;
}
