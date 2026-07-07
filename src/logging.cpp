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
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <utility>

#include "logging.h"

namespace logging
{

namespace
{

/** Return a timestamp like `14:23:51.042`. */
std::string format_timestamp_short(
  std::chrono::system_clock::time_point timestamp)
{
    using namespace std::chrono;

    const auto sec = floor<seconds>(timestamp);
    const auto ms = duration_cast<milliseconds>(timestamp - sec);

    const std::time_t time = system_clock::to_time_t(sec);

    std::tm local_time{};
    localtime_r(&time, &local_time);

    return std::format(
      "{:02}:{:02}:{:02}.{:03}",
      local_time.tm_hour,
      local_time.tm_min,
      local_time.tm_sec,
      ms.count());
}

/** Return a timestamp like `2026-07-07 14:23:51.042`. */
std::string format_timestamp_long(
  std::chrono::system_clock::time_point timestamp)
{
    using namespace std::chrono;

    const auto sec = floor<seconds>(timestamp);
    const auto ms = duration_cast<milliseconds>(timestamp - sec);

    const std::time_t time = system_clock::to_time_t(sec);

    std::tm local_time{};
    localtime_r(&time, &local_time);

    return std::format(
      "{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
      local_time.tm_year + 1900,
      local_time.tm_mon + 1,
      local_time.tm_mday,
      local_time.tm_hour,
      local_time.tm_min,
      local_time.tm_sec,
      ms.count());
}

/** Ensure timestamp and cached display fields are populated for a record. */
LogRecord prepare_record(
  const LogRecord& record)
{
    LogRecord prepared = record;

    if(prepared.timestamp.time_since_epoch().count() == 0)
    {
        prepared.timestamp = std::chrono::system_clock::now();
    }

    if(prepared.timestamp_short.empty())
    {
        prepared.timestamp_short = format_timestamp_short(prepared.timestamp);
    }

    if(prepared.display_line.empty())
    {
        prepared.display_line = std::format(
          "{} {}: {}: {}",
          prepared.timestamp_short,
          prepared.label,
          prepared.level,
          prepared.message);
    }

    return prepared;
}

/** Create a warning record summarizing dropped file logger messages. */
LogRecord make_dropped_record(
  std::size_t dropped_records)
{
    return prepare_record(
      LogRecord{
        .label = "FileLogDevice",
        .level = LogLevel::Warn,
        .message = std::format(
          "Dropped {} log records due to file logging backpressure.",
          dropped_records),
      });
}

/** Create a standard lifecycle log record for startup/shutdown events. */
LogRecord make_lifecycle_record(
  std::string_view message)
{
    return prepare_record(
      LogRecord{
        .label = std::string{default_logger_label},
        .level = LogLevel::Log,
        .message = std::string{message},
      });
}

}    // namespace

/** LogDevice singleton */
static LogNull g_log_null;
static std::mutex g_log_device_mutex;
LogDevice* LogDevice::instance{&g_log_null};

/*
 * LogDevice singleton interface
 */

void LogDevice::set(
  LogDevice* instance)
{
    std::scoped_lock lock{g_log_device_mutex};

    LogDevice* next = instance != nullptr
                        ? instance
                        : &g_log_null;
    if(LogDevice::instance == next)
    {
        return;
    }

    LogDevice* previous = LogDevice::instance != nullptr
                            ? LogDevice::instance
                            : &g_log_null;
    previous->on_shutdown();

    LogDevice::instance = next;
    LogDevice::instance->on_initialized();
}

LogDevice& LogDevice::get()
{
    std::scoped_lock lock{g_log_device_mutex};
    assert(instance != nullptr);
    return *instance;
}

bool LogDevice::is_initialized()
{
    std::scoped_lock lock{g_log_device_mutex};
    return instance != nullptr;
}

void LogDevice::cleanup()
{
    std::scoped_lock lock{g_log_device_mutex};
    instance = &g_log_null;
}

/*
 * LogDevice.
 */

void LogDevice::write_record(
  const LogRecord& record)
{
    log_n(
      std::format(
        "[{}.{}] {}",
        record.label,
        to_string(record.level),
        record.message));
}

/*
 * BufferedLogDevice.
 */

void BufferedLogDevice::write_record(
  const LogRecord& record)
{
    LogRecord prepared = prepare_record(record);

    std::scoped_lock lock{mutex};
    records.push_back(std::move(prepared));
}

void BufferedLogDevice::log_n(
  std::string_view message)
{
    LogRecord record{
      .label = std::string{default_logger_label},
      .level = LogLevel::Log,
      .message = std::string{message},
    };

    write_record(record);
}

std::vector<LogRecord> BufferedLogDevice::get_records() const
{
    std::scoped_lock lock{mutex};
    return std::vector<LogRecord>{records.begin(), records.end()};
}

void BufferedLogDevice::clear()
{
    std::scoped_lock lock{mutex};
    records.clear();
}

/*
 * FileLogDevice.
 */

FileLogDevice::FileLogDevice(
  std::filesystem::path output_path,
  FileLogDeviceOptions options)
: output_path{std::move(output_path)}
, options{options}
{
    if(this->output_path.empty())
    {
        throw std::invalid_argument("FileLogDevice requires a non-empty path.");
    }

    if(this->output_path.has_parent_path())
    {
        std::filesystem::create_directories(this->output_path.parent_path());
    }

    file_stream.open(this->output_path, std::ios::out | std::ios::trunc);
    if(!file_stream.is_open())
    {
        throw std::runtime_error(
          std::format(
            "Failed to open log file '{}'.",
            this->output_path.string()));
    }

    writer_thread = std::jthread([this]() -> void
                                 { writer_loop(); });
}

FileLogDevice::~FileLogDevice()
{
    on_shutdown();

    {
        std::scoped_lock lock{file_mutex};
        stop_requested = true;
    }
    file_condition.notify_all();

    if(writer_thread.joinable())
    {
        writer_thread.join();
    }

    file_stream.flush();
}

void FileLogDevice::enqueue_record(
  LogRecord record,
  bool notify_immediately)
{
    bool should_notify = notify_immediately;

    {
        std::unique_lock lock{file_mutex};

        const auto can_enqueue = [this]() -> bool
        {
            return stop_requested || pending_records.size() < options.max_pending_records;
        };

        if(options.overflow_policy == OverflowPolicy::Block)
        {
            file_condition.wait(lock, can_enqueue);
            if(stop_requested)
            {
                ++dropped_records;
                return;
            }
        }

        if(pending_records.size() >= options.max_pending_records)
        {
            if(options.overflow_policy == OverflowPolicy::DropNewest)
            {
                ++dropped_records;
                return;
            }

            if(options.overflow_policy == OverflowPolicy::DropOldest)
            {
                pending_records.pop_front();
                ++dropped_records;
            }
        }

        pending_records.push_back(std::move(record));
        should_notify = should_notify || pending_records.size() >= options.notify_batch_size;
    }

    if(should_notify)
    {
        file_condition.notify_one();
    }
}

void FileLogDevice::emit_lifecycle_record(std::string_view message)
{
    LogRecord record = make_lifecycle_record(message);
    BufferedLogDevice::write_record(record);
    enqueue_record(std::move(record), true);
}

void FileLogDevice::on_initialized()
{
    {
        std::scoped_lock lock{file_mutex};
        if(session_active || stop_requested)
        {
            return;
        }

        session_active = true;
    }

    emit_lifecycle_record(
      std::format(
        "Log opened at {}.",
        format_timestamp_long(std::chrono::system_clock::now())));
}

void FileLogDevice::on_shutdown()
{
    {
        std::scoped_lock lock{file_mutex};
        if(!session_active || stop_requested)
        {
            return;
        }

        session_active = false;
    }

    emit_lifecycle_record(
      std::format(
        "Log closed at {}.",
        format_timestamp_long(std::chrono::system_clock::now())));
}

void FileLogDevice::write_record(const LogRecord& record)
{
    LogRecord prepared = prepare_record(record);
    BufferedLogDevice::write_record(prepared);
    enqueue_record(std::move(prepared));
}

void FileLogDevice::writer_loop()
{
    std::deque<LogRecord> batch;

    while(true)
    {
        std::size_t dropped = 0;

        {
            std::unique_lock lock{file_mutex};
            file_condition.wait_for(
              lock,
              options.flush_interval,
              [this]() -> bool
              {
                  return stop_requested
                         || !pending_records.empty()
                         || dropped_records > 0;
              });

            pending_records.swap(batch);
            dropped = std::exchange(dropped_records, 0);

            if(stop_requested && batch.empty() && dropped == 0)
            {
                break;
            }
        }

        file_condition.notify_all();

        for(const LogRecord& record: batch)
        {
            file_stream << record.display_line << '\n';
        }
        batch.clear();

        if(dropped > 0)
        {
            file_stream << make_dropped_record(dropped).display_line << '\n';
        }

        file_stream.flush();
    }
}

/*
 * logging interface.
 */

void initialize(
  LogDevice* device)
{
    LogDevice::set(device);
}

void shutdown()
{
    LogDevice::set(&g_log_null);
}

void set(
  LogDevice* device)
{
    LogDevice::set(device);
}

[[nodiscard]]
LogDevice& get()
{
    return LogDevice::get();
}

}    // namespace logging
