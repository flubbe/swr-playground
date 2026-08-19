/**
 * Software Rasterizer Playground.
 *
 * Serializers for container types.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <array>
#include <filesystem>

#include "serialization/archive.h"

namespace serial
{

/**
 * Serialize a string.
 *
 * @param ar The archive to use.
 * @param s The string to serialize.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  swr::string& s)
{
    VLEInt len{numeric_cast<std::int64_t>(s.length())};
    ar & len;
    if(ar.is_reading())
    {
        s.resize(len.i);
    }
    for(std::int64_t i = 0; i < len.i; ++i)
    {
        ar& s[i];
    }
    return ar;
}

/**
 * Serialize an `std::array`.
 *
 * @tparam T Element type.
 * @tparam N The array length.
 * @param ar The archive to use.
 * @param v The array to be serialized.
 * @returns The input archive.
 */
template<
  typename T,
  std::size_t N>
Archive& operator&(
  Archive& ar,
  std::array<T, N>& v)
{
    for(std::size_t i = 0; i < N; ++i)
    {
        ar& v[i];
    }

    return ar;
}

/**
 * Serialize a templated `swr::vector`.
 *
 * @param ar The archive to use.
 * @param v The vector to be serialized.
 * @returns The input archive.
 */
template<
  typename T,
  typename Allocator>
Archive& operator&(
  Archive& ar,
  std::vector<T, Allocator>& v)
{
    VLEInt len{numeric_cast<std::int64_t>(v.size())};
    ar & len;

    if(ar.is_reading())
    {
        v.resize(len.i);
    }

    for(std::int64_t i = 0; i < len.i; ++i)
    {
        ar& v[i];
    }

    return ar;
}

/**
 * Serialize `std::optional`.
 *
 * @param ar The archive to use.
 * @param o The optional to be serialized.
 * @returns The input archive.
 */
template<typename T>
Archive& operator&(
  Archive& ar,
  std::optional<T>& o)
{
    bool has_value = o.has_value();
    ar & has_value;

    if(!has_value)
    {
        if(ar.is_reading())
        {
            o = std::nullopt;
        }
        return ar;
    }

    if(ar.is_reading())
    {
        T v;
        ar & v;
        o = std::move(v);
    }
    else
    {
        ar&(*o);
    }

    return ar;
}

/**
 * Serialize `swr::unique_ptr`.
 *
 * @param ar The archive to use.
 * @param ptr The unique pointer to be serialized.
 * @returns The input archive.
 */
template<typename T>
Archive& operator&(
  Archive& ar,
  swr::unique_ptr<T>& ptr)
{
    bool has_value = (ptr != nullptr);
    ar & has_value;

    if(!has_value)
    {
        if(ar.is_reading())
        {
            ptr = nullptr;
        }
        return ar;
    }

    if(ar.is_reading())
    {
        ptr = swr::make_unique<T>();
    }

    ar&(*ptr);

    return ar;
}

/**
 * Serialize `std::pair`.
 *
 * @param ar The archive to use.
 * @param p The pair to be serialized.
 * @returns The input archive.
 */
template<typename S, typename T>
Archive& operator&(
  Archive& ar,
  std::pair<S, T>& p)
{
    ar& std::get<0>(p);
    ar& std::get<1>(p);
    return ar;
}

/**
 * Helper for `std::tuple` serialization.
 *
 * @param ar The archive to use.
 * @param t The tuple to be serialized.
 */
template<
  typename Archive,
  typename Tuple,
  std::size_t... Is>
void serialize_tuple(
  Archive& ar,
  Tuple& t,
  std::index_sequence<Is...>)
{
    (void)std::initializer_list<int>{
      (ar & std::get<Is>(t), 0)...};
}

/**
 * Serialize `std::tuple`.
 *
 * @param ar The archive to use.
 * @param t The tuple to be serialized.
 * @returns The input archive.
 */
template<typename... Args>
Archive& operator&(
  Archive& ar,
  std::tuple<Args...>& t)
{
    serialize_tuple(ar, t, std::index_sequence_for<Args...>{});
    return ar;
}

/**
 * Serialize a path.
 *
 * @param ar The archive to use.
 * @param p The path to serialize.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  std::filesystem::path& p)
{
    auto value = swr::string_from(p.generic_string());
    ar & value;
    p = std::filesystem::path{value};

    return ar;
}

}    // namespace serial
