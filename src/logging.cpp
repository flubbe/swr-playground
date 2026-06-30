/**
 * Software Rasterizer Playground.
 *
 * Logging.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <cassert>

#include "logging.h"

namespace logging
{

/** LogDevice singleton */
static LogNull g_log_null;
LogDevice* LogDevice::instance{&g_log_null};

/*
 * singleton interface
 */

void LogDevice::set(LogDevice* instance)
{
    if(instance)
    {
        LogDevice::instance = instance;
    }
    else
    {
        LogDevice::instance = &g_log_null;
    }
}

LogDevice& LogDevice::get()
{
    assert(is_initialized());
    return *instance;
}

bool LogDevice::is_initialized()
{
    return instance != nullptr;
}

void LogDevice::cleanup()
{
    instance = &g_log_null;
}

void BufferedLogDevice::log_n(
  std::string_view message)
{
    lines.emplace_back(std::string{message});
}

const std::vector<std::string>& BufferedLogDevice::get_lines() const
{
    return lines;
}

void BufferedLogDevice::clear()
{
    lines.clear();
}

void log_init(
  LogDevice* device)
{
    LogDevice::set(device);
}

void log_shutdown()
{
    LogDevice::set(&g_log_null);
}

void set_log(
  LogDevice* device)
{
    LogDevice::set(device);
}

}    // namespace logging
