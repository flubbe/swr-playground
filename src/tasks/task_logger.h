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
struct TaskLogger
{
    /** Defaulted virtual destructor. */
    virtual ~TaskLogger() = default;

    /** Emit an informational message. */
    virtual void log(std::string_view message) const = 0;

    /** Emit a warning message. */
    virtual void warning(std::string_view message) const = 0;

    /** Emit an error message. */
    virtual void error(std::string_view message) const = 0;

    /**
     * Formatted logging.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void logf(
      std::format_string<Args...> format,
      Args&&... args) const
    {
        log(std::format(
          format,
          std::forward<Args>(args)...));
    }

    /**
     * Formatted warning.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void warningf(
      std::format_string<Args...> format,
      Args&&... args) const
    {
        warning(std::format(
          format,
          std::forward<Args>(args)...));
    }

    /**
     * Formatted error.
     *
     * @param format Format string.
     * @param args Arguments.
     */
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
struct NullTaskLogger final
: public TaskLogger
{
    /** Singleton instance accessor. */
    [[nodiscard]]
    static const NullTaskLogger& instance()
    {
        static const NullTaskLogger logger{};
        return logger;
    }

    void log(std::string_view) const override
    {
    }

    void warning(std::string_view) const override
    {
    }

    void error(std::string_view) const override
    {
    }
};

}    // namespace task_system
