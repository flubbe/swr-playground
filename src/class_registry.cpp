/**
 * Software Rasterizer Playground.
 *
 * Object reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>
#include <stdexcept>
#include <unordered_map>

#include "class_registry.h"

namespace
{
using ClassMap = std::unordered_map<std::string, const ClassInfo*>;

PendingClassNode*& pending_head()
{
    static PendingClassNode* head = nullptr;
    return head;
}

ClassMap& classes()
{
    static ClassMap map;
    return map;
}

bool& auto_registration_enabled()
{
    static bool value = true;
    return value;
}

void register_class(const ClassInfo& cls)
{
    const auto [_, inserted] = classes().emplace(cls.qualified_name, &cls);
    if(!inserted)
    {
        throw std::runtime_error{
          std::format("Duplicate class registration: {}", cls.qualified_name)};
    }
}
}    // namespace

void ReflectionSystem::allow_auto_registration(const bool enabled) noexcept
{
    auto_registration_enabled() = enabled;
}

bool ReflectionSystem::is_auto_registration_allowed() noexcept
{
    return auto_registration_enabled();
}

void ReflectionSystem::add_pending(PendingClassNode* node) noexcept
{
    node->next = pending_head();
    pending_head() = node;
}

void ReflectionSystem::process_pending_registrations()
{
    if(!auto_registration_enabled())
    {
        throw std::runtime_error{
          "ReflectionSystem::process_pending_registrations called while auto registration is disabled"};
    }

    while(pending_head() != nullptr)
    {
        PendingClassNode* node = pending_head();
        pending_head() = node->next;
        node->next = nullptr;

        const PendingClassRegistration* reg = node->reg;
        if(reg == nullptr || reg->storage == nullptr)
        {
            throw std::runtime_error{
              "Invalid pending class registration entry"};
        }

        ClassInfo& cls = *reg->storage;
        if(!cls.name.empty())
        {
            throw std::runtime_error{
              std::format("Class already initialized: {}", cls.qualified_name)};
        }

        cls.name = reg->name;
        cls.module_name = reg->module_name;
        cls.qualified_name = std::format("{}.{}", reg->module_name, reg->name);
        cls.parent = (reg->get_base_class != nullptr) ? reg->get_base_class() : nullptr;
        cls.size = reg->size;
        cls.factory = reg->factory;

        register_class(cls);
    }
}

const ClassInfo* ReflectionSystem::find_class(const std::string_view qualified_name) noexcept
{
    const auto it = classes().find(qualified_name.data());
    return (it != classes().end()) ? it->second : nullptr;
}

void ReflectionSystem::unregister_class(const std::string_view qualified_name)
{
    classes().erase(qualified_name.data());
}

void ReflectionSystem::unregister_module(const std::string_view module_name)
{
    for(auto it = classes().begin(); it != classes().end();)
    {
        if(it->second->module_name == module_name)
        {
            it = classes().erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ReflectionSystem::clear()
{
    classes().clear();
}
