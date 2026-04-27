/**
 * Software Rasterizer Playground.
 *
 * Class metadata for object reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include "property.h"

namespace reflect
{

/**
 * Class info for RTTI-style object queries and editor metadata.
 *
 * Thread safety:
 * - `get_super()` is internally synchronized for concurrent lazy resolution.
 * - Other mutable fields are not internally synchronized.
 */
struct ClassInfo
{
    using FactoryFn = void* (*)();
    using DestroyFn = void (*)(void*);
    using PropertyRegisterFn = void (*)(ClassInfo&);
    using SuperResolverFn = const ClassInfo* (*)();

    enum class SuperState
    {
        Unresolved,
        Resolving,
        Resolved
    };

    /** Module name of the class. */
    std::string module_name;

    /** The class name. */
    std::string name;

    /** Qualified name as `module_name.name`. */
    std::string qualified_name;

    /** Byte size of the class. */
    std::size_t size{0};

    /** Super-class info. */
    mutable const ClassInfo* super{nullptr};

    /** Lazy resolver for `super`. */
    SuperResolverFn resolve_super{nullptr};

    /** Root hierarchy marker for this class. */
    const void* root_tag{nullptr};

    /** Type-erased instance creation function. */
    FactoryFn factory{nullptr};

    /** Type-erased instance destruction function. */
    DestroyFn destroy{nullptr};

    /** Property registration. */
    PropertyRegisterFn register_properties{nullptr};

    /** Linked list of registered properties for this class. */
    std::unique_ptr<PropertyDescriptor> first_property;

    /**
     * Get this class' super class.
     *
     * The result is resolved lazily via `resolve_super` and cached in `super`.
     * This function is safe for concurrent calls.
     *
     * @throws Throws a `std::runtime_error` on circular super-class resolution.
     */
    const ClassInfo* get_super() const
    {
        std::unique_lock lock{super_mutex};

        for(;;)
        {
            if(super_state == SuperState::Resolved)
            {
                return super;
            }

            if(super_state == SuperState::Unresolved)
            {
                if(resolve_super == nullptr)
                {
                    super_state = SuperState::Resolved;
                    return super;
                }

                super_state = SuperState::Resolving;
                super_resolving_thread = std::this_thread::get_id();
                break;
            }

            if(super_resolving_thread == std::this_thread::get_id())
            {
                throw std::runtime_error{
                  std::format(
                    "Circular super-class resolution for class '{}'",
                    qualified_name)};
            }

            super_cv.wait(
              lock,
              [this]
              {
                  return super_state != SuperState::Resolving;
              });
        }

        const SuperResolverFn resolver = resolve_super;
        lock.unlock();

        const ClassInfo* resolved_super = nullptr;
        try
        {
            resolved_super = resolver();
        }
        catch(...)
        {
            lock.lock();
            super_state = SuperState::Unresolved;
            super_resolving_thread = std::thread::id{};
            lock.unlock();
            super_cv.notify_all();
            throw;
        }

        lock.lock();
        super = resolved_super;
        super_state = SuperState::Resolved;
        super_resolving_thread = std::thread::id{};
        lock.unlock();
        super_cv.notify_all();

        return resolved_super;
    }

    /**
     * Find a registered property descriptor by internal property name.
     *
     * @param property_name Internal property name (`DescriptorBase::name`).
     * @returns Returns the first matching descriptor, or `nullptr` if not found.
     */
    PropertyDescriptor* find_property(std::string_view property_name) noexcept
    {
        for(auto* descriptor = first_property.get();
            descriptor != nullptr;
            descriptor = descriptor->next.get())
        {
            if(descriptor->name == property_name)
            {
                return descriptor;
            }
        }

        return nullptr;
    }

    /**
     * Find a registered property descriptor by internal property name.
     *
     * Searches this class first, then walks through superclasses.
     *
     * @param property_name Internal property name (`PropertyDescriptor::name`).
     * @returns The first matching descriptor, or `nullptr` if not found.
     */
    const PropertyDescriptor* find_property(std::string_view property_name) const
    {
        for(const auto* cls = this; cls != nullptr; cls = cls->get_super())
        {
            for(const auto* descriptor = cls->first_property.get();
                descriptor != nullptr;
                descriptor = descriptor->next.get())
            {
                if(descriptor->name == property_name)
                {
                    return descriptor;
                }
            }
        }

        return nullptr;
    }

    /**
     * Check if this class is a child of another class.
     *
     * @param other The potential super class.
     * @returns Returns `true` if `other` is a super class.
     */
    bool is_a(const ClassInfo* other) const
    {
        for(auto p = this; p != nullptr; p = p->get_super())
        {
            if(p == other)
            {
                return true;
            }
        }
        return false;
    }

private:
    /** Synchronizes lazy super-class resolution. */
    mutable std::mutex super_mutex;

    /** Notifies waiters when super-class resolution completes. */
    mutable std::condition_variable super_cv;

    /** Current super-class resolution state. */
    mutable SuperState super_state{SuperState::Unresolved};

    /** Thread currently resolving the super class (for re-entrancy/cycle detection). */
    mutable std::thread::id super_resolving_thread{};
};

}    // namespace reflect
