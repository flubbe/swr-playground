/**
 * Software Rasterizer Playground.
 *
 * Basic platform support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

/** Global platform initialization. */
bool platform_init(
  int argc,
  char* argv[]);

/** Global platform shutdown. */
void platform_shutdown();
