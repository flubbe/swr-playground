/**
 * Software Rasterizer Playground.
 *
 * Reflection registry.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <unordered_map>

#include "class_registry.h"

namespace
{

/** Class registry type, mapping qualified names to class metadata. */
using ClassMap = std::unordered_multimap<
  std::string,
  const reflect::ClassInfo*>;

/** Head of pending class linked list. */
reflect::PendingClassNode*& pending_head() noexcept
{
    static reflect::PendingClassNode* head = nullptr;
    return head;
}

/** Class registry. */
ClassMap& classes() noexcept
{
    static ClassMap map;
    return map;
}

/** Automatic registration enable/disable. */
bool& auto_registration_enabled() noexcept
{
    static bool value = true;
    return value;
}

/**
 * Register a class.
 *
 * @param cls Class info/metadata to register.
 * @throws Throws a `std::runtime_error` if the class was already registered.
 */
void register_class(
  const reflect::ClassInfo& cls)
{
    const auto [begin, end] = classes().equal_range(cls.qualified_name);
    for(auto it = begin; it != end; ++it)
    {
        const reflect::ClassInfo* existing = it->second;
        if(existing != nullptr
           && existing->root_tag == cls.root_tag)
        {
            throw std::runtime_error{
              std::format(
                "Duplicate class registration for root '{}': {}",
                static_cast<const void*>(cls.root_tag),
                cls.qualified_name)};
        }
    }

    classes().emplace(
      cls.qualified_name,
      &cls);
}

/**
 * Reverse the property linked-list.
 *
 * When traversing the initial property linked list, the properties are encountered
 * in reverse declaration order. To not reverse them every time on object construction,
 * we do it once after registration.
 *
 * @param cls The class info.
 */
void reverse_properties(
  reflect::ClassInfo& cls) noexcept
{
    std::unique_ptr<
      reflect::PropertyDescriptor>
      prev = nullptr;
    std::unique_ptr<
      reflect::PropertyDescriptor>
      curr = std::move(cls.first_property);

    while(curr != nullptr)
    {
        auto next = std::move(curr->next);
        curr->next = std::move(prev);
        prev = std::move(curr);
        curr = std::move(next);
    }

    cls.first_property = std::move(prev);
}

}    // namespace

namespace reflect
{

void ReflectionSystem::add_pending(
  PendingClassNode* node) noexcept
{
    node->next = pending_head();
    pending_head() = node;
}

void ReflectionSystem::allow_auto_registration(
  const bool enabled) noexcept
{
    auto_registration_enabled() = enabled;
}

bool ReflectionSystem::is_auto_registration_allowed() noexcept
{
    return auto_registration_enabled();
}

void ReflectionSystem::process_pending_registrations()
{
    if(auto_registration_enabled())
    {
        throw std::runtime_error{
          "ReflectionSystem::process_pending_registrations called while auto registration is enabled"};
    }

    while(pending_head() != nullptr)
    {
        PendingClassNode* node = pending_head();
        pending_head() = node->next;
        node->next = nullptr;

        const PendingClassRegistration* reg = node->reg;
        if(reg == nullptr
           || reg->storage == nullptr)
        {
            throw std::runtime_error{
              "Invalid pending class registration entry"};
        }

        ClassInfo& cls = *reg->storage;
        if(!cls.name.empty())
        {
            throw std::runtime_error{
              std::format(
                "Class already registered: {}",
                cls.qualified_name)};
        }

        cls.name = reg->name;
        cls.module_name = reg->module_name;
        cls.qualified_name = std::format(
          "{}.{}",
          reg->module_name,
          reg->name);
        cls.super = reg->super;
        cls.root_tag = reg->root_tag;
        cls.size = reg->size;
        cls.factory = reg->factory;
        cls.destroy = reg->destroy;
        cls.register_properties = reg->register_properties;

        if(cls.register_properties != nullptr)
        {
            // register properties. properties are registered in reverse
            // declaration order, so we reverse them here.
            cls.register_properties(cls);
            reverse_properties(cls);
        }

        register_class(cls);
    }
}

const ClassInfo* ReflectionSystem::find_class(
  std::string_view qualified_name,
  const void* root_tag)
{
    const auto [begin, end] = classes().equal_range(std::string{qualified_name});
    for(auto it = begin; it != end; ++it)
    {
        const ClassInfo* cls = it->second;
        if(cls != nullptr
           && cls->root_tag == root_tag)
        {
            return cls;
        }
    }

    return nullptr;
}

bool ReflectionSystem::unregister_class(
  std::string_view qualified_name,
  const void* root_tag)
{
    bool removed = false;
    const std::string key{qualified_name};
    const auto [begin, end] = classes().equal_range(key);

    for(auto it = begin; it != end;)
    {
        const ClassInfo* cls = it->second;
        if(cls != nullptr
           && cls->root_tag == root_tag)
        {
            it = classes().erase(it);
            removed = true;
            continue;
        }

        ++it;
    }

    return removed;
}

std::size_t ReflectionSystem::unregister_module(
  std::string_view module_name)
{
    std::size_t removed_count = 0;

    for(auto it = classes().begin(); it != classes().end();)
    {
        if(it->second->module_name == module_name)
        {
            it = classes().erase(it);
            ++removed_count;
        }
        else
        {
            ++it;
        }
    }

    return removed_count;
}

std::vector<const ClassInfo*> ReflectionSystem::get_registered_classes()
{
    std::vector<const ClassInfo*> result;
    result.reserve(classes().size());

    for(const auto& [_, cls]: classes())
    {
        if(cls != nullptr)
        {
            result.push_back(cls);
        }
    }

    std::ranges::sort(
      result,
      [](const ClassInfo* a, const ClassInfo* b)
      {
          if(a->qualified_name != b->qualified_name)
          {
              return a->qualified_name < b->qualified_name;
          }
          return a->root_tag < b->root_tag;
      });

    return result;
}

void ReflectionSystem::clear()
{
    classes().clear();
}

}    // namespace reflect
