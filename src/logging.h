/**
 * Software Rasterizer Playground.
 *
 * Logging.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <format>
#include <string>
#include <vector>

namespace logging
{

/** generic text logging device */
class LogDevice
{
    /** singleton */
    static LogDevice* instance;

protected:
    /** log with newline at end. */
    virtual void log_n(const std::string& message) = 0;

public:
    /** empty destructor */
    virtual ~LogDevice()
    {
    }

    void log(
      const char* format,
      std::format_args args)
    {
        const std::string msg = std::vformat(format, args);
        log_n(std::format("Log: {}", msg));
    }

    template<typename... Args>
    void logf(
      const char* format,
      const Args&... args)
    {
        log(format, std::make_format_args(args...));
    }

    void warning(
      const char* format,
      std::format_args args)
    {
        const std::string msg = std::vformat(format, args);
        log_n(std::format("Warning: {}", msg));
    }

    template<typename... Args>
    void warningf(
      const char* format,
      const Args&... args)
    {
        warning(format, std::make_format_args(args...));
    }

    void error(
      const char* format,
      std::format_args args)
    {
        const std::string msg = std::vformat(format, args);
        log_n(std::format("Error: {}", msg));
    }

    template<typename... Args>
    void errorf(
      const char* format,
      const Args&... args)
    {
        error(format, std::make_format_args(args...));
    }

    /** set new global instance. note that this does not clean up memory. does not check if `instance` is valid. */
    static void set(LogDevice* instance);

    /** singleton interface getter. */
    static LogDevice& get();

    /** check if an instance is available. */
    static bool is_initialized();

    /** reset singleton to the fallback null device. */
    static void cleanup();
};

/** formatted log interface. */
template<typename... Args>
void logf(
  const char* format,
  const Args&... args)
{
    LogDevice::get().log(
      format,
      std::make_format_args(args...));
}

/** formatted log interface. */
template<typename... Args>
void warningf(
  const char* format,
  const Args&... args)
{
    LogDevice::get().warning(
      format,
      std::make_format_args(args...));
}

/** formatted log interface. */
template<typename... Args>
void errorf(
  const char* format,
  const Args&... args)
{
    LogDevice::get().error(
      format,
      std::make_format_args(args...));
}

/** log empty line. */
inline void log_n()
{
    LogDevice::get().logf("");
}

/** Fall-back null log device. Does not log. */
class LogNull : public LogDevice
{
protected:
    void log_n(const std::string&) override
    {
    }
};

/** Log device that stores messages in a growing line buffer. */
class BufferedLogDevice : public LogDevice
{
    std::vector<std::string> lines;

protected:
    void log_n(const std::string& message) override;

public:
    /** Get stored log lines. */
    const std::vector<std::string>& get_lines() const;

    /** Clear all stored log lines. */
    void clear();
};

/** Logging initialization. */
void log_init(LogDevice* device = nullptr);

/** Logging shutdown. */
void log_shutdown();

/** enable log by setting a (non-null) log device. disable it by passing `nullptr`. */
void set_log(LogDevice* device);

}    // namespace logging
