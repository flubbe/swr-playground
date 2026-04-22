/**
 * Software Rasterizer Playground.
 *
 * Property reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ml/all.h"

class IntProperty;
class UIntProperty;
class FloatProperty;
class BoolProperty;
class StringProperty;
class Mat4Property;
class Object;
struct ClassInfo;

class PropertyVisitor
{
public:
    virtual ~PropertyVisitor() = default;

    virtual void visit(IntProperty& property) = 0;
    virtual void visit(UIntProperty& property) = 0;
    virtual void visit(FloatProperty& property) = 0;
    virtual void visit(BoolProperty& property) = 0;
    virtual void visit(StringProperty& property) = 0;
    virtual void visit(Mat4Property& property) = 0;
};

class Property
{
    std::string name;
    std::string label;
    bool read_only{false};

public:
    Property(
      std::string name,
      std::string label,
      bool read_only = false);

    virtual ~Property() = default;

    const std::string& get_name() const noexcept
    {
        return name;
    }

    const std::string& get_label() const noexcept
    {
        return label;
    }

    bool is_read_only() const noexcept
    {
        return read_only;
    }

    virtual void accept(PropertyVisitor& visitor) = 0;
};

using PropertyList = std::vector<std::unique_ptr<Property>>;

using ConstructFn = std::unique_ptr<Property> (*)(
  Object&,
  std::string_view,    // name
  std::string_view,    // label
  bool                 // read_only
);

struct PropertyDescriptor
{
    std::string_view name;
    std::string_view label;
    bool read_only{false};
    ConstructFn construct{nullptr};
    PropertyDescriptor* next{nullptr};
};

struct PropertyRegistration
{
    PropertyRegistration(
      const ClassInfo* (*get_class)() noexcept,
      PropertyDescriptor* descriptor) noexcept;
};

class IntProperty : public Property
{
    int* value{nullptr};
    int min_value{0};
    int max_value{0};
    bool has_limits{false};
    float speed{1.0f};

public:
    IntProperty(
      std::string name,
      std::string label,
      int* value,
      bool read_only = false,
      float speed = 1.0f);

    IntProperty(
      std::string name,
      std::string label,
      int* value,
      int min_value,
      int max_value,
      bool read_only = false,
      float speed = 1.0f);

    bool has_value() const noexcept;
    int get_value() const noexcept;
    bool set_value(int in_value) noexcept;

    bool has_limits_enabled() const noexcept;
    int get_min_value() const noexcept;
    int get_max_value() const noexcept;
    float get_speed() const noexcept;

    void accept(PropertyVisitor& visitor) override;
};

class UIntProperty : public Property
{
    unsigned int* value{nullptr};
    unsigned int min_value{0};
    unsigned int max_value{0};
    bool has_limits{false};
    float speed{1.0f};

public:
    UIntProperty(
      std::string name,
      std::string label,
      unsigned int* value,
      bool read_only = false,
      float speed = 1.0f);

    UIntProperty(
      std::string name,
      std::string label,
      unsigned int* value,
      unsigned int min_value,
      unsigned int max_value,
      bool read_only = false,
      float speed = 1.0f);

    bool has_value() const noexcept;
    unsigned int get_value() const noexcept;
    bool set_value(unsigned int in_value) noexcept;

    bool has_limits_enabled() const noexcept;
    unsigned int get_min_value() const noexcept;
    unsigned int get_max_value() const noexcept;
    float get_speed() const noexcept;

    void accept(PropertyVisitor& visitor) override;
};

class FloatProperty : public Property
{
    float* value{nullptr};
    float min_value{0.0f};
    float max_value{0.0f};
    bool has_limits{false};
    float speed{0.01f};
    const char* format{"%.3f"};

public:
    FloatProperty(
      std::string name,
      std::string label,
      float* value,
      bool read_only = false,
      float speed = 0.01f,
      const char* format = "%.3f");

    FloatProperty(
      std::string name,
      std::string label,
      float* value,
      float min_value,
      float max_value,
      bool read_only = false,
      float speed = 0.01f,
      const char* format = "%.3f");

    bool has_value() const noexcept;
    float get_value() const noexcept;
    bool set_value(float in_value) noexcept;

    bool has_limits_enabled() const noexcept;
    float get_min_value() const noexcept;
    float get_max_value() const noexcept;
    float get_speed() const noexcept;
    const char* get_format() const noexcept;

    void accept(PropertyVisitor& visitor) override;
};

class BoolProperty : public Property
{
    bool* value{nullptr};

public:
    BoolProperty(
      std::string name,
      std::string label,
      bool* value,
      bool read_only = false);

    bool has_value() const noexcept;
    bool get_value() const noexcept;
    bool set_value(bool in_value) noexcept;

    void accept(PropertyVisitor& visitor) override;
};

class StringProperty : public Property
{
    std::string* value{nullptr};
    std::size_t max_length{256};

public:
    StringProperty(
      std::string name,
      std::string label,
      std::string* value,
      bool read_only = false,
      std::size_t max_length = 256);

    bool has_value() const noexcept;
    const std::string& get_value() const noexcept;
    bool set_value(std::string_view in_value);
    std::size_t get_max_length() const noexcept;

    void accept(PropertyVisitor& visitor) override;
};

class Mat4Property : public Property
{
    ml::mat4x4* value{nullptr};

public:
    Mat4Property(
      std::string name,
      std::string label,
      ml::mat4x4* value,
      bool read_only = false);

    bool has_value() const noexcept;
    const ml::mat4x4& get_value() const noexcept;
    bool set_value(const ml::mat4x4& in_value) noexcept;

    void accept(PropertyVisitor& visitor) override;
};

template<typename T>
struct PropertyTypeMap;

template<>
struct PropertyTypeMap<int>
{
    using type = IntProperty;
};

template<>
struct PropertyTypeMap<std::uint32_t>
{
    using type = UIntProperty;
};

template<>
struct PropertyTypeMap<float>
{
    using type = FloatProperty;
};

template<>
struct PropertyTypeMap<bool>
{
    using type = BoolProperty;
};

template<>
struct PropertyTypeMap<std::string>
{
    using type = StringProperty;
};

template<>
struct PropertyTypeMap<ml::mat4x4>
{
    using type = Mat4Property;
};

template<typename T>
struct PropertyFactory;

template<>
struct PropertyFactory<int>
{
    static std::unique_ptr<Property> construct(
      std::string name,
      std::string_view label,
      int& value,
      bool read_only)
    {
        return std::make_unique<IntProperty>(
          std::string{name},
          std::string{label},
          &value,
          read_only);
    }
};

template<>
struct PropertyFactory<std::uint32_t>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::uint32_t& value,
      bool read_only)
    {
        return std::make_unique<UIntProperty>(
          std::string{name},
          std::string{label},
          &value,
          read_only);
    }
};

template<>
struct PropertyFactory<float>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      float& value,
      bool read_only)
    {
        return std::make_unique<FloatProperty>(
          std::string{name},
          std::string{label},
          &value,
          read_only);
    }
};

template<>
struct PropertyFactory<bool>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      bool& value,
      bool read_only)
    {
        return std::make_unique<BoolProperty>(
          std::string{name},
          std::string{label},
          &value,
          read_only);
    }
};

template<>
struct PropertyFactory<std::string>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::string& value,
      bool read_only)
    {
        return std::make_unique<StringProperty>(
          std::string{name},
          std::string{label},
          &value,
          read_only);
    }
};

template<>
struct PropertyFactory<ml::mat4x4>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      ml::mat4x4& value,
      bool read_only)
    {
        return std::make_unique<Mat4Property>(
          std::string{name},
          std::string{label},
          &value,
          read_only);
    }
};

template<typename T>
struct UnwrapType
{
    using Type = T;

    static Type& get(T& value) noexcept
    {
        return value;
    }
};

template<typename Class, typename Member, Member Class::* MemberPtr>
std::unique_ptr<Property> construct_member(
  Object& obj,
  std::string_view name,
  std::string_view label,
  bool read_only)
{
    static_assert(std::is_base_of_v<Object, Class>,
                  "Class must derive from Object");

    auto& typed = static_cast<Class&>(obj);
    Member& value = typed.*MemberPtr;

    using Traits = UnwrapType<Member>;
    using UnwrappedType = typename Traits::Type;

    return PropertyFactory<UnwrappedType>::construct(
      name,
      label,
      Traits::get(value),
      read_only);
}
