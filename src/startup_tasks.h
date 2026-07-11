/**
 * Software Rasterizer Playground.
 *
 * Startup task definitions.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <vector>

#include "startup_scene.h"
#include "tasks/task_system.h"

namespace startup_tasks
{

/**
 * Create all startup tasks.
 *
 * @param scene Storage for the created/loaded data during startup.
 */
[[nodiscard]]
std::vector<task_system::TaskSpec> create_startup_tasks(
  PreparedStartupScene& scene);

} /* namespace startup_tasks */
