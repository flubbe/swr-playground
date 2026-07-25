#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include <gtest/gtest.h>

#include "logging.h"

namespace
{

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream stream{path};
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::size_t count_matches(
  const std::string& content,
  std::string_view needle)
{
    std::size_t count = 0;
    std::size_t pos = 0;

    while((pos = content.find(needle, pos)) != std::string::npos)
    {
        ++count;
        pos += needle.size();
    }

    return count;
}

}    // namespace

TEST(LoggingTests, FileLogDevicePersistsMessages)
{
    const auto path = std::filesystem::temp_directory_path() / "swr_playground_file_log_device.log";
    std::filesystem::remove(path);

    {
        logging::FileLogDevice device{path};
        logging::initialize(&device);
        device.logf("Hello {}", "file");
        device.warningf("Warn {}", 7);
        logging::shutdown();
    }

    const std::string content = read_file(path);
    EXPECT_NE(content.find("Global: Log: Log opened at "), std::string::npos);
    EXPECT_NE(content.find("Global: Log: Hello file"), std::string::npos);
    EXPECT_NE(content.find("Global: Warn: Warn 7"), std::string::npos);
    EXPECT_NE(content.find("Global: Log: Log closed at "), std::string::npos);
    EXPECT_GT(content.rfind("Log closed at "), content.rfind("Warn 7"));

    std::filesystem::remove(path);
}

TEST(LoggingTests, FileLogDeviceReportsDroppedMessagesUnderBackpressure)
{
    const auto path = std::filesystem::temp_directory_path() / "swr_playground_file_log_device_backpressure.log";
    std::filesystem::remove(path);

    {
        logging::FileLogDevice device{
          path,
          logging::FileLogDeviceOptions{
            .max_pending_records = 4,
            .notify_batch_size = 1024,
            .flush_interval = std::chrono::milliseconds{500},
            .overflow_policy = logging::OverflowPolicy::DropNewest,
          }};

        logging::initialize(&device);

        for(int i = 0; i < 2000; ++i)
        {
            device.logf("Spam {}", i);
        }

        logging::shutdown();
    }

    const std::string content = read_file(path);
    EXPECT_NE(content.find("Global: Log: Spam 0"), std::string::npos);
    EXPECT_NE(content.find("Dropped "), std::string::npos);

    std::filesystem::remove(path);
}

TEST(LoggingTests, FileLogDeviceWritesLifecycleMarkersAtInitializationAndShutdown)
{
    const auto path = std::filesystem::temp_directory_path() / "swr_playground_file_log_device_lifecycle.log";
    std::filesystem::remove(path);

    swr::vector<logging::LogRecord> records;

    {
        logging::FileLogDevice device{path};
        logging::initialize(&device);
        device.logf("Between");
        logging::shutdown();
        device.get_records(records);
    }

    const std::string content = read_file(path);
    const std::regex lifecycle_regex{
      R"(Log (opened|closed) at \d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}\.)"};

    const auto opened = content.find("Global: Log: Log opened at ");
    const auto payload = content.find("Global: Log: Between");
    const auto closed = content.rfind("Global: Log: Log closed at ");

    ASSERT_NE(opened, std::string::npos);
    ASSERT_NE(payload, std::string::npos);
    ASSERT_NE(closed, std::string::npos);
    EXPECT_LT(opened, payload);
    EXPECT_LT(payload, closed);
    EXPECT_EQ(content.find("Global: Log: Log closed at "), closed);
    EXPECT_TRUE(std::regex_search(content, lifecycle_regex));

    ASSERT_EQ(records.size(), 3u);
    EXPECT_EQ(records.front().label, "Global");
    EXPECT_TRUE(std::regex_match(records.front().message, lifecycle_regex));
    EXPECT_EQ(records[1].label, "Global");
    EXPECT_EQ(records[1].message, "Between");
    EXPECT_EQ(records.back().label, "Global");
    EXPECT_TRUE(std::regex_match(records.back().message, lifecycle_regex));

    std::filesystem::remove(path);
}

TEST(LoggingTests, FileLogDeviceOverwritesExistingLogFileOnStart)
{
    const auto path = std::filesystem::temp_directory_path() / "swr_playground_file_log_device_overwrite.log";

    {
        std::ofstream seed{path};
        seed << "stale log line\n";
    }

    {
        logging::FileLogDevice device{path};
        logging::initialize(&device);
        device.logf("Fresh");
        logging::shutdown();
    }

    const std::string content = read_file(path);
    EXPECT_EQ(content.find("stale log line"), std::string::npos);
    EXPECT_NE(content.find("Global: Log: Fresh"), std::string::npos);

    std::filesystem::remove(path);
}

TEST(LoggingTests, FileLogDeviceShutdownIsIdempotent)
{
    const auto path = std::filesystem::temp_directory_path() / "swr_playground_file_log_device_idempotent_shutdown.log";
    std::filesystem::remove(path);

    {
        logging::FileLogDevice device{path};
        logging::initialize(&device);
        logging::shutdown();
        logging::shutdown();
    }

    const std::string content = read_file(path);
    EXPECT_EQ(count_matches(content, "Global: Log: Log opened at "), 1u);
    EXPECT_EQ(count_matches(content, "Global: Log: Log closed at "), 1u);

    std::filesystem::remove(path);
}
