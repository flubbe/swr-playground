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

#include "containers/vector.h"
#include "tasks/task_system.h"
#include "staged_data.h"

namespace startup_tasks
{

/**
 * Create all startup tasks.
 *
 * @param scene Storage for the created/loaded data during startup.
 */
[[nodiscard]]
swr::vector<task_system::TaskSpec> create_startup_tasks(
  StagedStartupScene& scene);

} /* namespace startup_tasks */
