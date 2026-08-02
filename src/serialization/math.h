/**
 * Software Rasterizer Playground.
 *
 * Math type serialization support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <ml/all.h>

#include "serialization/archive.h"

namespace serial
{

/**
 * Serialize a `cnl::static_number<M, N>`.
 *
 * @param ar The archive to use.
 * @param n The vector.
 * @returns The input archive.
 */
template<int Digits, int Exponent>
Archive& operator&(
  Archive& ar,
  cnl::static_number<Digits, Exponent>& n)
{
    ar& cnl::unwrap(n);
    return ar;
}

/**
 * Serialize a `ml::tvec2`.
 *
 * @param ar The archive to use.
 * @param v The vector.
 * @returns The input archive.
 */
template<typename T>
Archive& operator&(
  Archive& ar,
  ml::tvec2<T>& v)
{
    ar & v.x;
    ar & v.y;

    return ar;
}

/**
 * Serialize a `ml::vec2`.
 *
 * @param ar The archive to use.
 * @param v The vector.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  ml::vec2& v)
{
    ar & v.x;
    ar & v.y;

    return ar;
}

/**
 * Serialize a `ml::vec3`.
 *
 * @param ar The archive to use.
 * @param v The vector.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  ml::vec3& v)
{
    ar & v.x;
    ar & v.y;
    ar & v.z;

    return ar;
}

/**
 * Serialize a `ml::vec4`.
 *
 * @param ar The archive to use.
 * @param v The vector.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  ml::vec4& v)
{
    ar & v.x;
    ar & v.y;
    ar & v.z;
    ar & v.w;

    return ar;
}

/**
 * Serialize a `ml::line<T>`.
 *
 * @param ar The archive to use.
 * @param l The line.
 * @returns The input archive.
 */
template<typename T>
inline Archive& operator&(
  Archive& ar,
  ml::line<T>& v)
{
    ar & v.pos;
    ar & v.dir;

    return ar;
}

/**
 * Serialize a `ml::mat3x3`.
 *
 * @param ar The archive to use.
 * @param m The matrix.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  ml::mat3x3& m)
{
    ar & m.rows[0] & m.rows[1] & m.rows[2];

    return ar;
}

/**
 * Serialize a `ml::mat4x4`.
 *
 * @param ar The archive to use.
 * @param m The matrix.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  ml::mat4x4& m)
{
    ar & m.rows[0] & m.rows[1] & m.rows[2] & m.rows[3];

    return ar;
}

/**
 * Serialize a `ml::plane`.
 *
 * @param ar The archive to use.
 * @param p The plane.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  ml::plane& p)
{
    ar& static_cast<ml::vec4&>(p);

    return ar;
}

}    // namespace serial
