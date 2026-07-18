/**
 * Software Rasterizer Playground.
 *
 * Task system logging abstraction.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace task_system
{

/** Logging sink used by the task system library. */
class TaskLogger
{
public:
    virtual ~TaskLogger() = default;

    /** Emit an informational message. */
    virtual void log(std::string_view message) const = 0;

    /** Emit a warning message. */
    virtual void warn(std::string_view message) const = 0;

    /** Emit an error message. */
    virtual void error(std::string_view message) const = 0;

    template<typename... Args>
    void logf(
      std::format_string<Args...> format,
      Args&&... args) const
    {
        log(std::format(
          format,
          std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warnf(
      std::format_string<Args...> format,
      Args&&... args) const
    {
        warn(std::format(
          format,
          std::forward<Args>(args)...));
    }

    template<typename... Args>
    void errorf(
      std::format_string<Args...> format,
      Args&&... args) const
    {
        error(std::format(
          format,
          std::forward<Args>(args)...));
    }
};

/** Default no-op logger used when no logger is provided. */
class NullTaskLogger final
: public TaskLogger
{
public:
    [[nodiscard]]
    static const NullTaskLogger& instance()
    {
        static const NullTaskLogger logger{};
        return logger;
    }

    void log(std::string_view) const override
    {
    }

    void warn(std::string_view) const override
    {
    }

    void error(std::string_view) const override
    {
    }
};

}    // namespace task_system
