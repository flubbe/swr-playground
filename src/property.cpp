/**
 * Software Rasterizer Playground.
 *
 * Property reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <utility>

#include "property.h"

namespace
{

const std::string& empty_string()
{
    static const std::string k_empty{};
    return k_empty;
}

}    // namespace

Property::Property(
  std::string name,
  std::string label,
  bool read_only)
: name{std::move(name)}
, label{std::move(label)}
, read_only{read_only}
{
}

IntProperty::IntProperty(
  std::string name,
  std::string label,
  int* value,
  bool read_only,
  float speed)
: Property{std::move(name), std::move(label), read_only}
, value{value}
, speed{speed}
{
}

IntProperty::IntProperty(
  std::string name,
  std::string label,
  int* value,
  int min_value,
  int max_value,
  bool read_only,
  float speed)
: Property{std::move(name), std::move(label), read_only}
, value{value}
, min_value{min_value}
, max_value{max_value}
, has_limits{true}
, speed{speed}
{
}

bool IntProperty::has_value() const noexcept
{
    return value != nullptr;
}

int IntProperty::get_value() const noexcept
{
    return value != nullptr ? *value : 0;
}

bool IntProperty::set_value(int in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    if(has_limits)
    {
        in_value = std::clamp(in_value, min_value, max_value);
    }

    *value = in_value;
    return true;
}

bool IntProperty::has_limits_enabled() const noexcept
{
    return has_limits;
}

int IntProperty::get_min_value() const noexcept
{
    return min_value;
}

int IntProperty::get_max_value() const noexcept
{
    return max_value;
}

float IntProperty::get_speed() const noexcept
{
    return speed;
}

void IntProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

UIntProperty::UIntProperty(
  std::string name,
  std::string label,
  std::uint32_t* value,
  bool read_only,
  float speed)
: Property{std::move(name), std::move(label), read_only}
, value{value}
, speed{speed}
{
}

UIntProperty::UIntProperty(
  std::string name,
  std::string label,
  std::uint32_t* value,
  std::uint32_t min_value,
  std::uint32_t max_value,
  bool read_only,
  float speed)
: Property{std::move(name), std::move(label), read_only}
, value{value}
, min_value{min_value}
, max_value{max_value}
, has_limits{true}
, speed{speed}
{
}

bool UIntProperty::has_value() const noexcept
{
    return value != nullptr;
}

std::uint32_t UIntProperty::get_value() const noexcept
{
    return value != nullptr ? *value : 0;
}

bool UIntProperty::set_value(std::uint32_t in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    if(has_limits)
    {
        in_value = std::clamp(in_value, min_value, max_value);
    }

    *value = in_value;
    return true;
}

bool UIntProperty::has_limits_enabled() const noexcept
{
    return has_limits;
}

std::uint32_t UIntProperty::get_min_value() const noexcept
{
    return min_value;
}

std::uint32_t UIntProperty::get_max_value() const noexcept
{
    return max_value;
}

float UIntProperty::get_speed() const noexcept
{
    return speed;
}

void UIntProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

FloatProperty::FloatProperty(
  std::string name,
  std::string label,
  float* value,
  bool read_only,
  float speed,
  const char* format)
: Property{std::move(name), std::move(label), read_only}
, value{value}
, speed{speed}
, format{format}
{
}

FloatProperty::FloatProperty(
  std::string name,
  std::string label,
  float* value,
  float min_value,
  float max_value,
  bool read_only,
  float speed,
  const char* format)
: Property{std::move(name), std::move(label), read_only}
, value{value}
, min_value{min_value}
, max_value{max_value}
, has_limits{true}
, speed{speed}
, format{format}
{
}

bool FloatProperty::has_value() const noexcept
{
    return value != nullptr;
}

float FloatProperty::get_value() const noexcept
{
    return value != nullptr ? *value : 0.0f;
}

bool FloatProperty::set_value(float in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    if(has_limits)
    {
        in_value = std::clamp(in_value, min_value, max_value);
    }

    *value = in_value;
    return true;
}

bool FloatProperty::has_limits_enabled() const noexcept
{
    return has_limits;
}

float FloatProperty::get_min_value() const noexcept
{
    return min_value;
}

float FloatProperty::get_max_value() const noexcept
{
    return max_value;
}

float FloatProperty::get_speed() const noexcept
{
    return speed;
}

const char* FloatProperty::get_format() const noexcept
{
    return format;
}

void FloatProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

BoolProperty::BoolProperty(
  std::string name,
  std::string label,
  bool* value,
  bool read_only)
: Property{std::move(name), std::move(label), read_only}
, value{value}
{
}

bool BoolProperty::has_value() const noexcept
{
    return value != nullptr;
}

bool BoolProperty::get_value() const noexcept
{
    return value != nullptr ? *value : false;
}

bool BoolProperty::set_value(bool in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    *value = in_value;
    return true;
}

void BoolProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

StringProperty::StringProperty(
  std::string name,
  std::string label,
  std::string* value,
  bool read_only,
  std::size_t max_length)
: Property{std::move(name), std::move(label), read_only}
, value{value}
, max_length{max_length}
{
}

bool StringProperty::has_value() const noexcept
{
    return value != nullptr;
}

const std::string& StringProperty::get_value() const noexcept
{
    return value != nullptr ? *value : empty_string();
}

bool StringProperty::set_value(std::string_view in_value)
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    const std::size_t count = std::min(in_value.size(), max_length);
    value->assign(in_value.data(), count);
    return true;
}

std::size_t StringProperty::get_max_length() const noexcept
{
    return max_length;
}

void StringProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}
