/**
 * Software Rasterizer Playground.
 *
 * Property reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "property.h"

namespace reflect
{

Property::Property(
  std::string name,
  std::string label,
  PropertyFlags flags)
: name{std::move(name)}
, label{std::move(label)}
, flags{flags}
{
}

}    // namespace reflect