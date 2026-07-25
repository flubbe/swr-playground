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

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#include <boost/circular_buffer.hpp>

#include "containers/deque.h"
#include "containers/string.h"
#include "containers/vector.h"

namespace logging
{

/** Default label for messages emitted through the global log interface. */
inline constexpr std::string_view default_logger_label{"Global"};

/** Log level. */
enum class LogLevel
{
    Log,
    Warn,
    Error,
};

/** A log record, which is an entry in the structured log. */
struct LogRecord
{
    std::chrono::system_clock::time_point timestamp{}; /** Timestamp at record emission. */
    swr::string label;                                 /** Record label. */
    LogLevel level{LogLevel::Log};                     /** Log level. */
    swr::string message;                               /** Log message. */

    /*
     * Caches.
     */

    swr::string timestamp_short; /** Formatted short form timestamp (lazy). */
    swr::string display_line;    /** Display-ready log line (lazy). */

    /** Constructors. */
    LogRecord() = default;
    LogRecord(const LogRecord&) = default;
    LogRecord(LogRecord&&) = default;

    /**
     * Initialize a log record with label, level and message.
     *
     * @param label Label.
     * @param level Log level.
     * @param message Logged message.
     */
    LogRecord(
      std::string_view label,
      LogLevel level,
      std::string_view message)
    : label{label}
    , level{level}
    , message{message}
    {
    }

    /** Assignments. */
    LogRecord& operator=(const LogRecord&) = default;
    LogRecord& operator=(LogRecord&&) = default;
};

/** Return the log level as a readable string. */
[[nodiscard]]
constexpr const char* to_string(LogLevel level)
{
    switch(level)
    {
    case LogLevel::Log:
        return "Log";
    case LogLevel::Warn:
        return "Warn";
    case LogLevel::Error:
        return "Error";
    }

    return "Unknown";
}

/** Generic text logging device */
class LogDevice
{
    /** Singleton. */
    static LogDevice* instance;

protected:
    /** Log with newline at end. */
    virtual void log_n(std::string_view message) = 0;

    /** Structured log sink extension point. */
    virtual void write_record(const LogRecord& record);

    /** Hook called when the device becomes active through the global logger. */
    virtual void on_initialized()
    {
    }

    /** Hook called before the device is removed from the global logger. */
    virtual void on_shutdown()
    {
    }

public:
    /** Default, virtual destructor */
    virtual ~LogDevice() = default;

    /**
     * Write a log record. If the timestamp field is empty, it is added here.
     *
     * @param record The log record to write.
     */
    void write(const LogRecord& record)
    {
        LogRecord stamped = record;
        if(stamped.timestamp.time_since_epoch().count() == 0)
        {
            stamped.timestamp = std::chrono::system_clock::now();
        }
        write_record(stamped);
    }

    /**
     * Log to the global logger.
     *
     * @param format Format string.
     * @param args Format arguments.
     */
    void log(
      std::string_view format,
      std::format_args args)
    {
        write(
          LogRecord{
            default_logger_label,
            LogLevel::Log,
            std::vformat(format, args)});
    }

    /**
     * Formatted logging.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void logf(
      std::string_view format,
      const Args&... args)
    {
        log(format, std::make_format_args(args...));
    }

    /**
     * Formatted warning.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    void warning(
      std::string_view format,
      std::format_args args)
    {
        write(
          LogRecord{
            default_logger_label,
            LogLevel::Warn,
            std::vformat(format, args),
          });
    }

    /**
     * Formatted warning.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void warningf(
      std::string_view format,
      const Args&... args)
    {
        warning(format, std::make_format_args(args...));
    }

    /**
     * Formatted error.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    void error(
      std::string_view format,
      std::format_args args)
    {
        write(
          LogRecord{
            default_logger_label,
            LogLevel::Error,
            std::vformat(format, args),
          });
    }

    /**
     * Formatted error.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void errorf(
      std::string_view format,
      const Args&... args)
    {
        error(format, std::make_format_args(args...));
    }

    /**
     * Set new global instance. To reset the global log device to the null device,
     * pass `nullptr`.
     *
     * @note Does not clean up memory. Does not check if `instance` is valid.
     * @param instance The new log device instance, or `nullptr`.
     */
    static void set(LogDevice* instance);

    /** Singleton interface getter. */
    static LogDevice& get();

    /** Check if an instance is available. */
    static bool is_initialized();

    /** Reset singleton to the fallback null device. */
    static void cleanup();
};

/** A logger, writing to a log device. */
class Logger
{
    /** Log device used by this logger. */
    LogDevice& log_device;

    /** Logger label. */
    swr::string label;

public:
    /**
     * Construct a logger for a given label and target log device.
     *
     * @param label Logger label.
     * @param log_device Log device. Defaults to the global log device.
     */
    explicit Logger(
      std::string_view label,
      LogDevice& log_device = LogDevice::get())
    : log_device{log_device}
    , label{label}
    {
    }

    /**
     * Formatted logging.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    void log(
      std::string_view format,
      std::format_args args) const
    {
        log_device.write(
          LogRecord{
            label,
            LogLevel::Log,
            std::vformat(format, args),
          });
    }

    /**
     * Formatted logging.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void logf(
      std::string_view format,
      const Args&... args) const
    {
        log(format, std::make_format_args(args...));
    }

    /**
     * Formatted warning.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    void warning(
      std::string_view format,
      std::format_args args) const
    {
        log_device.write(
          LogRecord{
            label,
            LogLevel::Warn,
            std::vformat(format, args),
          });
    }

    /**
     * Formatted warning.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void warningf(
      std::string_view format,
      const Args&... args) const
    {
        warning(format, std::make_format_args(args...));
    }

    /**
     * Formatted error.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    void error(
      std::string_view format,
      std::format_args args) const
    {
        log_device.write(
          LogRecord{
            label,
            LogLevel::Error,
            std::vformat(format, args),
          });
    }

    /**
     * Formatted error.
     *
     * @param format Format string.
     * @param args Arguments.
     */
    template<typename... Args>
    void errorf(
      std::string_view format,
      const Args&... args) const
    {
        error(format, std::make_format_args(args...));
    }
};

/**
 * Formatted logging to the global logger.
 *
 * @param format Format string.
 * @param args Arguments.
 */
template<typename... Args>
void logf(
  std::string_view format,
  const Args&... args)
{
    LogDevice::get().log(
      format,
      std::make_format_args(args...));
}

/**
 * Formatted warning emission to the global logger.
 *
 * @param format Format string.
 * @param args Arguments.
 */
template<typename... Args>
void warningf(
  std::string_view format,
  const Args&... args)
{
    LogDevice::get().warning(
      format,
      std::make_format_args(args...));
}

/**
 * Formatted error emission to the global logger.
 *
 * @param format Format string.
 * @param args Arguments.
 */
template<typename... Args>
void errorf(
  std::string_view format,
  const Args&... args)
{
    LogDevice::get().error(
      format,
      std::make_format_args(args...));
}

/** Log empty line to the global logger. */
inline void log_n()
{
    LogDevice::get().logf("");
}

/*
 * Log devices.
 */

/** Fall-back null log device. Does not log. */
class LogNull : public LogDevice
{
protected:
    void log_n(std::string_view) override
    {
    }
};

/** Log device that stores messages in a growing line buffer. */
class BufferedLogDevice : public LogDevice
{
    // TODO make configurable
    static constexpr std::size_t max_buffered_records = 5000;

    mutable std::mutex mutex;
    boost::circular_buffer<LogRecord> records{max_buffered_records};

protected:
    void write_record(const LogRecord& record) override;
    void log_n(std::string_view message) override;

public:
    /** Get a thread-safe snapshot of stored log records. */
    void get_records(swr::vector<LogRecord>& records) const;

    /** Clear all stored log lines. */
    void clear();
};

/** Overflow behavior when the file logger queue is full. */
enum class OverflowPolicy
{
    /** Block producer threads until queue capacity is available again. */
    Block,

    /** Drop the newly submitted record and keep already queued records. */
    DropNewest,

    /** Drop the oldest queued record to make room for the new record. */
    DropOldest,
};

/** Configuration for asynchronous file logging. */
struct FileLogDeviceOptions
{
    /** Maximum number of records waiting to be written to disk. */
    std::size_t max_pending_records{8192};

    /** Wake the writer early once at least this many records are queued. */
    std::size_t notify_batch_size{64};

    /** Maximum delay between periodic background flush attempts. */
    std::chrono::milliseconds flush_interval{250};

    /** Queue overflow behavior for producers. */
    OverflowPolicy overflow_policy{OverflowPolicy::Block};
};

/**
 * Buffered log device that also persists log lines to a file on a background thread.
 *
 * Backpressure behavior is configured through FileLogDeviceOptions::overflow_policy
 * (block producer, drop newest, or drop oldest). Dropped records are summarized in
 * the log stream once capacity becomes available again.
 */
class FileLogDevice : public BufferedLogDevice
{
    /** Output log file path. */
    std::filesystem::path output_path;

    /** Runtime options for asynchronous file logging. */
    FileLogDeviceOptions options;

    /** Whether lifecycle open/close markers are currently active. */
    bool session_active{false};

    /** Mutex guarding the file queue and writer state. */
    std::mutex file_mutex;

    /** Condition variable used to wake or block the writer thread. */
    std::condition_variable file_condition;

    /** Queue of pending records waiting to be written to disk. */
    swr::deque<LogRecord> pending_records;

    /** Output stream used by the background writer. */
    std::ofstream file_stream;

    /** Background writer thread flushing queued records to disk. */
    std::jthread writer_thread;

    /** Stop flag for terminating the background writer loop. */
    bool stop_requested{false};

    /** Number of dropped records accumulated since the last flush. */
    std::size_t dropped_records{0};

    /** Emit a lifecycle marker record (open/close) to buffers and file queue. */
    void emit_lifecycle_record(std::string_view message);

    /** Enqueue one record for asynchronous disk persistence. */
    void enqueue_record(LogRecord record, bool notify_immediately = false);

    /** Background loop that drains pending records and flushes them to disk. */
    void writer_loop();

protected:
    /** Handle activation of this device as the global logger target. */
    void on_initialized() override;

    /** Handle deactivation of this device as the global logger target. */
    void on_shutdown() override;

    /** Write a structured record to the in-memory buffer and file queue. */
    void write_record(const LogRecord& record) override;

public:
    /** Construct a file-backed buffered log device with asynchronous writing. */
    explicit FileLogDevice(
      std::filesystem::path output_path,
      FileLogDeviceOptions options = {});

    /** Stop the writer thread and flush all pending records. */
    ~FileLogDevice() override;

    /** Return the configured output path for this file logger. */
    [[nodiscard]]
    const std::filesystem::path& path() const noexcept
    {
        return output_path;
    }
};

/** Logging initialization. */
void initialize(LogDevice* device = nullptr);

/** Logging shutdown. */
void shutdown();

/** enable log by setting a (non-null) log device. disable it by passing `nullptr`. */
void set(LogDevice* device);

/** get the log device instance. */
[[nodiscard]]
LogDevice& get();

}    // namespace logging

namespace std
{

template<>
struct formatter<logging::LogLevel, char>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(
      const logging::LogLevel& level,
      FormatContext& ctx) const
    {
        return format_to(
          ctx.out(),
          "{}",
          logging::to_string(level));
    }
};

template<>
struct formatter<logging::LogRecord, char>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(
      const logging::LogRecord& record,
      FormatContext& ctx) const
    {
        if(!record.timestamp_short.empty())
        {
            return format_to(
              ctx.out(),
              "{} {}: {}: {}",
              record.timestamp_short,
              record.label,
              record.level,
              record.message);
        }

        return format_to(
          ctx.out(),
          "{}: {}: {}",
          record.label,
          record.level,
          record.message);
    }
};

}    // namespace std
