#include <gtest/gtest.h>

#include "file_manager.h"

// Asset directory is defined via CMakeTests.txt.
// We also provide a fallback.
#ifndef ASSETS_SOURCE_DIR
#    define ASSETS_SOURCE_DIR "assets"
#endif

TEST(FileManagerTests, Construction)
{
    ASSERT_NO_THROW(FileManager{});
}

TEST(FileManagerTests, Resolve)
{
    FileManager file_mgr{};
    ASSERT_NO_THROW(file_mgr.add_search_path(ASSETS_SOURCE_DIR));
    EXPECT_THROW((void)file_mgr.resolve_read("test"), FileError);         // file/dir does not exist
    EXPECT_THROW((void)file_mgr.resolve_read("materials"), FileError);    // directory
    EXPECT_NO_THROW((void)file_mgr.resolve_read("materials/floor/floor.json"));
}

TEST(FileManagerTests, Read)
{
    FileManager file_mgr{};
    ASSERT_NO_THROW(file_mgr.add_search_path(ASSETS_SOURCE_DIR));

    std::unique_ptr<serial::FileArchive> ar;
    ASSERT_NO_THROW(ar = file_mgr.open_read("materials/floor/floor.json"));
    ASSERT_NE(ar, nullptr);
    EXPECT_GT(ar->size(), 0);

    std::string contents(ar->size(), '\0');
    std::span<std::byte> bytes{
      std::as_writable_bytes(
        std::span{contents.data(), contents.size()})};
    ar->serialize(bytes);

    EXPECT_TRUE(std::ranges::all_of(
      contents,
      [](char c)
      {
          return static_cast<unsigned char>(c) < 128;
      }));
}

TEST(FileManagerTests, Write)
{
    const auto temp_dir =
      std::filesystem::temp_directory_path() / "swr_file_manager_test";

    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    FileManager file_mgr{};
    ASSERT_NO_THROW(file_mgr.set_writable_root(temp_dir));

    const auto path = "test.bin";
    std::array<std::byte, 4> data{
      std::byte{0x01},
      std::byte{0x02},
      std::byte{0x03},
      std::byte{0x04}};

    ASSERT_NO_THROW({
        auto ar = file_mgr.open_write(path);
        ASSERT_NE(ar, nullptr);
        ar->serialize(std::span{data.data(), data.size()});
    });

    const auto written_path = temp_dir / path;
    ASSERT_TRUE(std::filesystem::is_regular_file(written_path));

    EXPECT_EQ(
      std::filesystem::file_size(written_path),
      data.size());

    std::ifstream file{written_path, std::ios::binary};
    ASSERT_TRUE(file);

    std::array<std::byte, 4> result{};
    file.read(
      reinterpret_cast<char*>(result.data()),
      result.size());

    EXPECT_EQ(result, data);

    std::filesystem::remove_all(temp_dir);
}
