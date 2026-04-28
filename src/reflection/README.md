# Reflection

This module provides runtime class metadata and property reflection for object hierarchies rooted in `ReflectRoot<T>`.

## Components

- `class_info.h`: `ClassInfo` metadata (name, module, size, inheritance, factory/destroy hooks, property descriptors).
- `class_registry.h/.cpp`: global registry, static registration queue, lookup/unregister APIs, root-scoped type separation.
- `property.h`: `Property` base type, descriptor model, typed access helpers, member-binding construction.
- `builtin_properties.h/.cpp`: built-in property implementations (`int`, `unsigned int`, `float`, `bool`, `std::string`).
- `flags.h`: `PropertyFlags` bit flags (for example read-only).
- `except.h`: reflection-specific exception types.
- `traits.h`: member-pointer traits used by descriptor construction.

## Registration Model

Registration is split into declaration and definition:

- `DECLARE_REFLECTION(Module, Type)` in the type header.
- `DEFINE_REFLECTION(Type)` in exactly one translation unit.

Each registration contributes a `PendingClassRegistration`. `ReflectionSystem::process_pending_registrations()` validates and publishes queued classes into the registry. Lookup is done by qualified name (`module.class`) and root tag.

## Type Hierarchy

- `ReflectRoot<Base>` is the root base class for a reflected hierarchy.
- `Reflected<Derived, Base>` is the CRTP helper for non-root types.
- `static_class()` returns static metadata for the type.
- `get_class()` returns metadata for an instance.
- `is_a(...)` supports hierarchy checks.

## Property Model

Properties are registered by member pointer:

- `register_property<&Type::member>(class_info, "name", "Label", flags)`.
- `register_property<&Type::member>(class_info, "name", "Label", flags, constraint)`.

This appends a `PropertyDescriptor` containing a construction function. At object initialization, descriptors are materialized into concrete `Property` objects.
Descriptors are registered in declaration order (the registration list is reversed once during finalization).

Each `Property` stores:

- internal name and display label
- flags
- optional typed constraint metadata (`std::shared_ptr<const PropertyConstraint>`)
- memory layout metadata for the reflected value:
  - `size`
  - `offset` (from owning object base)
  - `alignment`

Built-in property classes compute static `size`/`alignment` from their `Type` alias and receive `offset` from descriptor construction.

## Constraints

`PropertyConstraint` is the base metadata type for value constraints.

- `RangeConstraint<T>` supports optional inclusive `min`/`max`, optional `step`, and `clamp`.
- `Property::try_get_constraint<T>()` / `try_get_range_constraint<T>()` provide exact-type retrieval at runtime.
- `register_property` validates typed constraints at compile time (`Constraint::ValueType` must match the reflected member type when present).

For built-in numeric properties (`int`, `unsigned int`, `float`), range constraints are enforced in `set_value(...)`:

- out-of-range values are rejected when `clamp == false`
- out-of-range values are clamped to `min`/`max` when `clamp == true`

## Minimal Usage

```cpp
class Object : public reflect::ReflectRoot<Object>
{
public:
    int id{0};

    static void register_properties(reflect::ClassInfo& ci)
    {
        reflect::register_property<&Object::id>(ci, "id", "Object ID");
    }
};

DECLARE_REFLECTION(Core, Object);
DEFINE_REFLECTION(Object);

int main()
{
    // Finalize queued static registrations before class lookup/usage.
    reflect::ReflectionSystem::allow_auto_registration(false);
    reflect::ReflectionSystem::process_pending_registrations();

    const reflect::ClassInfo* cls =
      reflect::ReflectionSystem::find_class<Object>("Core.Object");
}
```

## Notes

- `ClassInfo::find_property(...)` supports lookup by internal property name (const overload walks super classes).
- `ReflectionSystem::find_class(...)` is root-scoped, allowing the same qualified class name in different root hierarchies.
