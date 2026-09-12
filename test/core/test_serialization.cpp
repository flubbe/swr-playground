#include <cstring>

#include <gtest/gtest.h>

#include "serialization/archive.h"
#include "serialization/containers.h"
#include "serialization/file.h"
#include "serialization/json/writer.h"
#include "serialization/memory.h"

#include "../utils.h"

// NOTE There a lot of random/magic numbers in the tests.
// NOLINTBEGIN(readability-magic-numbers)

namespace
{

template<std::integral T>
std::array<std::uint8_t, sizeof(T)> to_little_endian(T value)
{
    using U = std::make_unsigned_t<T>;
    U u = static_cast<U>(value);

    std::array<std::uint8_t, sizeof(T)> bytes;
    for(std::size_t i = 0; i < sizeof(T); ++i)
    {
        bytes[i] = static_cast<std::uint8_t>(u & 0xff);
        u >>= 8;
    }

    return bytes;
}

std::array<std::uint8_t, sizeof(float)> to_little_endian(float f)
{
    static_assert(
      sizeof(float) == sizeof(std::uint32_t),
      "Float has to be the same size as std::uint32_t.");
    std::uint32_t i{0};
    std::memcpy(&i, &f, sizeof(i));
    return to_little_endian(i);
}

std::array<std::uint8_t, sizeof(double)> to_little_endian(double d)
{
    static_assert(
      sizeof(double) == sizeof(std::uint64_t),
      "Double has to be the same size as std::uint64_t.");
    std::uint64_t i{0};
    std::memcpy(&i, &d, sizeof(i));
    return to_little_endian(i);
}

template<std::integral T>
std::array<std::uint8_t, sizeof(T)> to_big_endian(T value)
{
    auto bytes = to_little_endian(value);
    std::ranges::reverse(bytes);
    return bytes;
}

std::array<std::uint8_t, sizeof(float)> to_big_endian(float f)
{
    static_assert(
      sizeof(float) == sizeof(std::uint32_t),
      "Float has to be the same size as std::uint32_t.");
    std::uint32_t i{0};
    std::memcpy(&i, &f, sizeof(i));
    return to_big_endian(i);
}

std::array<std::uint8_t, sizeof(double)> to_big_endian(double d)
{
    static_assert(
      sizeof(double) == sizeof(std::uint64_t),
      "Double has to be the same size as std::uint64_t.");
    std::uint64_t i{0};
    std::memcpy(&i, &d, sizeof(i));
    return to_big_endian(i);
}

}    // namespace

TEST(SerializationTests, VariableLengthEncodedIntegerRoundTrip)
{
    constexpr std::int64_t values[] = {
      0,
      1,
      -1,
      2,
      -2,
      63,
      -63,
      64,
      -64,
      127,
      -127,
      128,
      -128,
      255,
      -255,
      256,
      -256,
      16383,
      -16383,
      16384,
      -16384,
      std::numeric_limits<std::int32_t>::max(),
      std::numeric_limits<std::int32_t>::min(),
      std::numeric_limits<std::int64_t>::max(),
      std::numeric_limits<std::int64_t>::min(),
      0x123456789012345,
      -0x123456789012345};

    for(auto expected: values)
    {
        serial::MemoryWriteArchive write_ar{false};

        serial::VLEInt value{expected};
        write_ar & value;

        serial::MemoryReadArchive read_ar{
          write_ar.get_buffer(),
          write_ar.is_persistent(),
          write_ar.get_target_byte_order()};

        value.i = 0;
        read_ar & value;

        EXPECT_EQ(value.i, expected);
    }
}

TEST(SerializationTests, VariableLengthEncodedIntegerEncodedSize)
{
    struct TestCase
    {
        std::int64_t value;
        std::size_t expected_size;
    };

    constexpr TestCase cases[] = {
      {0, 1},
      {-1, 1},
      {1, 1},
      {-2, 1},
      {63, 1},
      {-64, 1},
      {64, 2},
      {-65, 2},
      {8191, 2},
      {-8192, 2},
      {8192, 3},
      {-8193, 3},
      {1048575, 3},
      {-1048576, 3},
      {1048576, 4},
      {-1048577, 4},
      {std::numeric_limits<std::int64_t>::max(), 10},
      {std::numeric_limits<std::int64_t>::min(), 10},
    };

    for(const auto& test: cases)
    {
        serial::MemoryWriteArchive ar{false};

        serial::VLEInt value{test.value};
        ar & value;

        EXPECT_EQ(ar.get_buffer().size(), test.expected_size)
          << "Value: " << test.value;
    }
}

TEST(SerializationTests, VariableLengthEncodedIntegerEncoding)
{
    struct TestCase
    {
        std::int64_t value;
        std::vector<std::byte> expected;
    };

    const std::array cases = {
      TestCase{0, {std::byte{0x00}}},
      TestCase{-1, {std::byte{0x01}}},
      TestCase{1, {std::byte{0x02}}},
      TestCase{-2, {std::byte{0x03}}},
      TestCase{2, {std::byte{0x04}}},
      TestCase{-3, {std::byte{0x05}}},
      TestCase{63, {std::byte{0x7e}}},
      TestCase{-64, {std::byte{0x7f}}},
      TestCase{64, {std::byte{0x80}, std::byte{0x01}}},
      TestCase{-65, {std::byte{0x81}, std::byte{0x01}}},
      TestCase{127, {std::byte{0xfe}, std::byte{0x01}}},
      TestCase{-128, {std::byte{0xff}, std::byte{0x01}}},
      TestCase{128, {std::byte{0x80}, std::byte{0x02}}},
    };

    for(const auto& test: cases)
    {
        serial::MemoryWriteArchive ar{false};

        serial::VLEInt value{test.value};
        ar & value;

        EXPECT_EQ(ar.get_buffer(), test.expected)
          << "Value: " << test.value;
    }
}

TEST(SerializationTests, VariableLengthEncodedIntegerRejectsMalformedInput)
{
    const std::vector<std::byte> buffer = {
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80},
      std::byte{0x80}};

    serial::MemoryReadArchive ar{
      buffer,
      false,
      std::endian::native};

    serial::VLEInt value{};

    EXPECT_THROW(ar & value, serial::SerializationError);
}

TEST(SerializationTests, BigEndianFileArchive)
{
    const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "big_endian.bin";
    FileCleanup cleanup{path};

    constexpr bool expected_bool = true;
    constexpr std::uint8_t expected_byte = 0x01;
    constexpr std::uint16_t expected_word = 0x1234;
    constexpr std::uint32_t expected_dword = 0x12345678;
    constexpr float expected_float = 1.234f;
    constexpr double expected_double = -123.4561234;
    constexpr std::int64_t expected_qword = 0x123456789012345;

    // write
    {
        serial::FileWriteArchive ar{path, std::endian::big};

        EXPECT_EQ(ar.get_target_byte_order(), std::endian::big);
        EXPECT_EQ(ar.is_persistent(), true);
        EXPECT_EQ(ar.is_reading(), false);
        EXPECT_EQ(ar.is_writing(), true);

        bool bo = true;
        std::uint8_t by = expected_bool;
        std::uint16_t w = expected_word;
        std::uint32_t dw = expected_dword;
        float f = expected_float;
        double d = expected_double;
        std::int64_t i = expected_qword;
        serial::VLEInt vi{i};

        ar & bo /* 1 byte */
          & by  /* 1 byte */
          & w   /* 2 bytes */
          & dw  /* 4 bytes */
          & f   /* 4 bytes */
          & d   /* 8 bytes */
          & vi; /* 9 bytes */

        ASSERT_EQ(ar.tell(), 29);
    }

    // validate file
    {
        std::ifstream file{path, std::ifstream::binary};
        ASSERT_TRUE(file);

        file.seekg(0, std::ios::end);
        std::size_t offs = file.tellg();
        file.seekg(0, std::ios::beg);
        offs -= file.tellg();
        ASSERT_EQ(offs, 29);

        std::vector<std::uint8_t> buf{std::istreambuf_iterator<char>(file), {}};
        ASSERT_EQ(buf.size(), 29);

        EXPECT_EQ(buf[0], expected_bool);
        EXPECT_EQ(buf[1], expected_byte);

        auto mem1 = to_big_endian(expected_word);
        EXPECT_TRUE(std::equal(mem1.begin(), mem1.end(), buf.begin() + 2));

        auto mem2 = to_big_endian(expected_dword);
        EXPECT_TRUE(std::equal(mem2.begin(), mem2.end(), buf.begin() + 4));

        auto mem3 = to_big_endian(expected_float);
        EXPECT_TRUE(std::equal(mem3.begin(), mem3.end(), buf.begin() + 8));

        auto mem4 = to_big_endian(expected_double);
        EXPECT_TRUE(std::equal(mem4.begin(), mem4.end(), buf.begin() + 12));
    }

    // read
    {
        serial::FileReadArchive ar{path, std::endian::big};

        EXPECT_EQ(ar.get_target_byte_order(), std::endian::big);
        EXPECT_EQ(ar.is_persistent(), true);
        EXPECT_EQ(ar.is_reading(), true);
        EXPECT_EQ(ar.is_writing(), false);

        bool bo{false};
        std::uint8_t by{0};
        std::uint16_t w{0};
        std::uint32_t dw{0};
        float f{0.f};
        double d{0.0};
        serial::VLEInt vi;

        ar & bo & by & w & dw & f & d & vi;

        ASSERT_EQ(ar.tell(), 29);

        EXPECT_EQ(bo, expected_bool);
        EXPECT_EQ(by, expected_byte);
        EXPECT_EQ(w, expected_word);
        EXPECT_EQ(dw, expected_dword);
        EXPECT_EQ(f, expected_float);
        EXPECT_EQ(d, expected_double);
        EXPECT_EQ(vi.i, expected_qword);
    }
}

TEST(SerializationTests, LittleEndianFileArchive)
{
    const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "little_endian.bin";
    FileCleanup cleanup{path};

    constexpr bool expected_bool = true;
    constexpr std::uint8_t expected_byte = 0x01;
    constexpr std::uint16_t expected_word = 0x1234;
    constexpr std::uint32_t expected_dword = 0x12345678;
    constexpr float expected_float = 1.234f;
    constexpr double expected_double = -123.4561234;
    constexpr std::int64_t expected_qword = 0x123456789012345;

    // write
    {
        serial::FileWriteArchive ar{path, std::endian::little};

        EXPECT_EQ(ar.get_target_byte_order(), std::endian::little);
        EXPECT_EQ(ar.is_persistent(), true);
        EXPECT_EQ(ar.is_reading(), false);
        EXPECT_EQ(ar.is_writing(), true);

        bool bo = expected_bool;
        std::uint8_t by = expected_byte;
        std::uint16_t w = expected_word;
        std::uint32_t dw = expected_dword;
        float f = expected_float;
        double d = expected_double;
        std::int64_t i = expected_qword;
        serial::VLEInt vi{i};

        ar & bo & by & w & dw & f & d & vi;

        ASSERT_EQ(ar.tell(), 29);
    }
    {
        std::ifstream file{path, std::ifstream::binary};
        ASSERT_TRUE(file);

        file.seekg(0, std::ios::end);
        std::size_t offs = file.tellg();
        file.seekg(0, std::ios::beg);
        offs -= file.tellg();
        ASSERT_EQ(offs, 29);

        std::vector<std::uint8_t> buf{std::istreambuf_iterator<char>(file), {}};
        ASSERT_EQ(buf.size(), 29);

        EXPECT_EQ(buf[0], expected_bool);
        EXPECT_EQ(buf[1], expected_byte);

        auto mem1 = to_little_endian(expected_word);
        EXPECT_TRUE(std::equal(mem1.begin(), mem1.end(), buf.begin() + 2));

        auto mem2 = to_little_endian(expected_dword);
        EXPECT_TRUE(std::equal(mem2.begin(), mem2.end(), buf.begin() + 4));

        auto mem3 = to_little_endian(expected_float);
        EXPECT_TRUE(std::equal(mem3.begin(), mem3.end(), buf.begin() + 8));

        auto mem4 = to_little_endian(expected_double);
        EXPECT_TRUE(std::equal(mem4.begin(), mem4.end(), buf.begin() + 12));
    }
    {
        serial::FileReadArchive ar{path, std::endian::little};

        EXPECT_EQ(ar.get_target_byte_order(), std::endian::little);
        EXPECT_EQ(ar.is_persistent(), true);
        EXPECT_EQ(ar.is_reading(), true);
        EXPECT_EQ(ar.is_writing(), false);

        bool bo{false};
        std::uint8_t by{0};
        std::uint16_t w{0};
        std::uint32_t dw{0};
        float f{0.f};
        double d{0.0};

        ar & bo & by & w & dw & f & d;

        ASSERT_EQ(ar.tell(), 20);

        EXPECT_EQ(bo, expected_bool);
        EXPECT_EQ(by, expected_byte);
        EXPECT_EQ(w, expected_word);
        EXPECT_EQ(dw, expected_dword);
        EXPECT_EQ(f, expected_float);
        EXPECT_EQ(d, expected_double);
    }
}

TEST(SerializationTests, EmptyString)
{
    const auto path =
      std::filesystem::temp_directory_path() / "empty_string.bin";
    FileCleanup cleanup{path};

    {
        serial::FileWriteArchive ar{path, std::endian::little};

        std::string s;
        ar & s;
    }

    {
        serial::FileReadArchive ar{path, std::endian::little};

        std::string s = "not empty";
        ar & s;

        EXPECT_TRUE(s.empty());
    }
}

TEST(SerializationTests, Strings)
{
    const std::filesystem::path little_path =
      std::filesystem::temp_directory_path() / "little_endian.bin";
    FileCleanup little_cleanup{little_path};

    const std::filesystem::path big_path =
      std::filesystem::temp_directory_path() / "big_endian.bin";
    FileCleanup big_cleanup{big_path};

    {
        serial::FileWriteArchive ar{little_path, std::endian::little};
        std::string s1 = "Hello, ";
        std::string s2 = "World!";
        ar & s1 & s2;
    }
    {
        serial::FileReadArchive ar{little_path, std::endian::little};
        std::string s1 = "Hello, ";
        std::string s2 = "World!";
        ar & s1 & s2;

        EXPECT_EQ(s1, "Hello, ");
        EXPECT_EQ(s2, "World!");
    }
    {
        serial::FileWriteArchive ar{big_path, std::endian::big};
        std::string s1 = "Hello, ";
        std::string s2 = "World!";
        ar & s1 & s2;
    }
    {
        serial::FileReadArchive ar{big_path, std::endian::big};
        std::string s1 = "Hello, ";
        std::string s2 = "World!";
        ar & s1 & s2;

        EXPECT_EQ(s1, "Hello, ");
        EXPECT_EQ(s2, "World!");
    }
}

TEST(SerializationTests, LongString)
{
    const auto path =
      std::filesystem::temp_directory_path() / "long_string.bin";
    FileCleanup cleanup{path};

    const std::string expected(4096, 'x');

    {
        serial::FileWriteArchive ar{path, std::endian::little};

        std::string s = expected;
        ar & s;
    }

    {
        serial::FileReadArchive ar{path, std::endian::little};

        std::string s;
        ar & s;

        EXPECT_EQ(s, expected);
    }
}

TEST(SerializationTests, EmptyVector)
{
    const auto path =
      std::filesystem::temp_directory_path() / "empty_vector.bin";
    FileCleanup cleanup{path};

    {
        serial::FileWriteArchive ar{path, std::endian::little};

        std::vector<int> v;
        ar & v;
    }

    {
        serial::FileReadArchive ar{path, std::endian::little};

        std::vector<int> v = {1, 2, 3};
        ar & v;

        EXPECT_TRUE(v.empty());
    }
}

TEST(SerializationTests, IntegerVector)
{
    const auto path =
      std::filesystem::temp_directory_path() / "integer_vector.bin";
    FileCleanup cleanup{path};

    const std::vector<std::int32_t> expected = {
      0,
      1,
      -1,
      42,
      -12345,
      std::numeric_limits<std::int32_t>::min(),
      std::numeric_limits<std::int32_t>::max()};

    {
        serial::FileWriteArchive ar{path, std::endian::little};

        auto v = expected;
        ar & v;
    }

    {
        serial::FileReadArchive ar{path, std::endian::little};

        std::vector<std::int32_t> v;
        ar & v;

        EXPECT_EQ(v, expected);
    }
}

TEST(SerializationTests, StringVector)
{
    const std::filesystem::path little_path =
      std::filesystem::temp_directory_path() / "little_endian.bin";
    FileCleanup little_cleanup{little_path};

    const std::filesystem::path big_path =
      std::filesystem::temp_directory_path() / "big_endian.bin";
    FileCleanup big_cleanup{big_path};

    {
        serial::FileWriteArchive ar{little_path, std::endian::little};
        std::vector<std::string> v = {"Hello, ", "World!"};
        ar & v;
    }
    {
        serial::FileReadArchive ar{little_path, std::endian::little};
        std::vector<std::string> v;

        ar & v;

        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(v[0], "Hello, ");
        EXPECT_EQ(v[1], "World!");
    }
    {
        serial::FileWriteArchive ar{big_path, std::endian::big};
        std::vector<std::string> v = {"Hello, ", "World!"};
        ar & v;
    }
    {
        serial::FileReadArchive ar{big_path, std::endian::big};
        std::vector<std::string> v;

        ar & v;

        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(v[0], "Hello, ");
        EXPECT_EQ(v[1], "World!");
    }
}

TEST(SerializationTests, NestedVectors)
{
    const auto path =
      std::filesystem::temp_directory_path() / "nested_vectors.bin";
    FileCleanup cleanup{path};

    const std::vector<std::vector<int>> expected = {
      {},
      {1},
      {1, 2, 3},
      {4, 5},
    };

    {
        serial::FileWriteArchive ar{path, std::endian::little};

        auto v = expected;
        ar & v;
    }

    {
        serial::FileReadArchive ar{path, std::endian::little};

        std::vector<std::vector<int>> v;
        ar & v;

        EXPECT_EQ(v, expected);
    }
}

TEST(SerializationTests, MemoryArchiveRoundTrip)
{
    serial::MemoryWriteArchive write_ar{false};

    constexpr bool expected_bool = true;
    constexpr std::uint32_t expected_uint = 0x12345678;
    constexpr float expected_float = 1.234f;
    constexpr double expected_double = -123.4561234;
    constexpr std::int64_t expected_int = 0x123456789012345;

    std::string expected_string = "Hello, World!";
    std::vector<std::string> expected_vector = {
      "Hello",
      "",
      "World"};

    bool b = expected_bool;
    std::uint32_t u = expected_uint;
    float f = expected_float;
    double d = expected_double;
    serial::VLEInt i{expected_int};

    write_ar & b & u & f & d & i & expected_string & expected_vector;

    serial::MemoryReadArchive read_ar{
      write_ar.get_buffer(),
      write_ar.is_persistent(),
      write_ar.get_target_byte_order()};

    b = false;
    u = 0;
    f = 0.f;
    d = 0.0;
    i.i = 0;

    std::string s;
    std::vector<std::string> v;

    read_ar & b & u & f & d & i & s & v;

    EXPECT_EQ(b, expected_bool);
    EXPECT_EQ(u, expected_uint);
    EXPECT_EQ(f, expected_float);
    EXPECT_EQ(d, expected_double);
    EXPECT_EQ(i.i, expected_int);
    EXPECT_EQ(s, expected_string);
    EXPECT_EQ(v, expected_vector);
}

TEST(SerializationTests, MemoryArchiveTell)
{
    serial::MemoryWriteArchive write_ar{false};

    bool b = true;
    std::uint32_t u = 0x12345678;
    serial::VLEInt i{127};

    EXPECT_EQ(write_ar.tell(), 0);

    write_ar & b;
    EXPECT_EQ(write_ar.tell(), 1);

    write_ar & u;
    EXPECT_EQ(write_ar.tell(), 5);

    write_ar & i;
    EXPECT_EQ(write_ar.tell(), 7);

    serial::MemoryReadArchive read_ar{
      write_ar.get_buffer(),
      false,
      write_ar.get_target_byte_order()};

    EXPECT_EQ(read_ar.tell(), 0);

    read_ar & b;
    EXPECT_EQ(read_ar.tell(), 1);

    read_ar & u;
    EXPECT_EQ(read_ar.tell(), 5);

    read_ar & i;
    EXPECT_EQ(read_ar.tell(), 7);
}

TEST(SerializationTests, MemoryArchiveLargeBuffer)
{
    std::vector<std::uint32_t> expected(10000);

    for(std::size_t i = 0; i < expected.size(); ++i)
    {
        expected[i] = static_cast<std::uint32_t>(i);
    }

    serial::MemoryWriteArchive write_ar{false};

    write_ar & expected;

    serial::MemoryReadArchive read_ar{
      write_ar.get_buffer(),
      false,
      write_ar.get_target_byte_order()};

    std::vector<std::uint32_t> actual;

    read_ar & actual;

    EXPECT_EQ(actual, expected);
}

TEST(SerializationTests, MemoryArchiveReadPastEnd)
{
    serial::MemoryWriteArchive write_ar{false};

    std::uint32_t expected = 42;
    write_ar & expected;

    serial::MemoryReadArchive read_ar{
      write_ar.get_buffer(),
      false,
      write_ar.get_target_byte_order()};

    std::uint32_t a = 0;
    std::uint32_t b = 0;

    read_ar & a;

    EXPECT_EQ(a, expected);

    EXPECT_THROW(read_ar & b, serial::SerializationError);
}

TEST(SerializationTests, MemoryArchiveEmptyBuffer)
{
    serial::MemoryWriteArchive write_ar{false};

    EXPECT_TRUE(write_ar.get_buffer().empty());

    serial::MemoryReadArchive read_ar{
      write_ar.get_buffer(),
      false,
      write_ar.get_target_byte_order()};

    std::uint32_t value = 0;

    EXPECT_THROW(read_ar & value, serial::SerializationError);
}

TEST(SerializationTests, MemoryArchiveByteOrder)
{
    constexpr std::uint16_t expected_word = 0x1234;
    constexpr std::uint32_t expected_dword = 0x12345678;
    constexpr float expected_float = 1.234f;
    constexpr double expected_double = -123.4561234;

    serial::MemoryWriteArchive little_write{true, std::endian::little};
    serial::MemoryWriteArchive big_write{true, std::endian::big};

    std::uint16_t w = expected_word;
    std::uint32_t dw = expected_dword;
    float f = expected_float;
    double d = expected_double;

    little_write & w & dw & f & d;

    w = expected_word;
    dw = expected_dword;
    f = expected_float;
    d = expected_double;

    big_write & w & dw & f & d;

    EXPECT_NE(little_write.get_buffer(), big_write.get_buffer());

    {
        serial::MemoryReadArchive ar{
          little_write.get_buffer(),
          true,
          std::endian::little};

        w = 0;
        dw = 0;
        f = 0.f;
        d = 0.0;

        ar & w & dw & f & d;

        EXPECT_EQ(w, expected_word);
        EXPECT_EQ(dw, expected_dword);
        EXPECT_EQ(f, expected_float);
        EXPECT_EQ(d, expected_double);
    }

    {
        serial::MemoryReadArchive ar{
          big_write.get_buffer(),
          true,
          std::endian::big};

        w = 0;
        dw = 0;
        f = 0.f;
        d = 0.0;

        ar & w & dw & f & d;

        EXPECT_EQ(w, expected_word);
        EXPECT_EQ(dw, expected_dword);
        EXPECT_EQ(f, expected_float);
        EXPECT_EQ(d, expected_double);
    }
}

TEST(SerializationTests, MemoryArchiveNonPersistentUsesNativeByteOrder)
{
    serial::MemoryWriteArchive native1{false, std::endian::little};
    serial::MemoryWriteArchive native2{false, std::endian::big};

    std::uint32_t a = 0x12345678;
    std::uint32_t b = 0x12345678;

    native1 & a;
    native2 & b;

    EXPECT_EQ(native1.get_target_byte_order(), std::endian::native);
    EXPECT_EQ(native2.get_target_byte_order(), std::endian::native);

    EXPECT_EQ(native1.get_buffer(), native2.get_buffer());
}

TEST(SerializationTests, ConstantSerializer)
{
    const std::uint32_t expected = 0x12345678;

    serial::MemoryWriteArchive write_ar{false};

    write_ar& serial::ConstantSerializer{expected};

    serial::MemoryReadArchive read_ar{
      write_ar.get_buffer(),
      false,
      write_ar.get_target_byte_order()};

    std::uint32_t actual{};
    read_ar & actual;

    EXPECT_EQ(actual, expected);
}

TEST(JsonWriterTests, Empty)
{
    serial::json::JsonWriter writer;
    EXPECT_EQ(writer.get(), "");
}

TEST(JsonWriterTests, RootLevelNumber)
{
    serial::json::JsonWriter writer;
    writer.write_val(42);
    EXPECT_EQ(writer.get(), "42");
}

TEST(JsonWriterTests, RootLevelFloat)
{
    serial::json::JsonWriter writer;
    writer.write_val(3.14159);
    EXPECT_EQ(writer.get(), "3.14159");
}

TEST(JsonWriterTests, RootLevelString)
{
    serial::json::JsonWriter writer;
    writer.write_val("Hello World");
    EXPECT_EQ(writer.get(), "\"Hello World\"");
}

TEST(JsonWriterTests, RootLevelBooleanAndNull)
{
    {
        serial::json::JsonWriter writer;
        writer.write_val(true);
        EXPECT_EQ(writer.get(), "true");
    }
    {
        serial::json::JsonWriter writer;
        writer.write_null();
        EXPECT_EQ(writer.get(), "null");
    }
}

TEST(JsonWriterTests, EmptyObject)
{
    serial::json::JsonWriter writer;
    writer.begin_object();
    writer.end_object();
    EXPECT_EQ(writer.get(), "{}");
}

TEST(JsonWriterTests, EmptyArray)
{
    serial::json::JsonWriter writer;
    writer.begin_array();
    writer.end_array();
    EXPECT_EQ(writer.get(), "[]");
}

TEST(JsonWriterTests, CompactedObject)
{
    serial::json::JsonWriter writer(4, true);
    writer.begin_object();
    writer.write_key_value("name", "a");
    writer.write_key_value("id", 101);
    writer.write_key_value("active", true);
    writer.end_object();

    EXPECT_EQ(writer.get(), R"({"name":"a","id":101,"active":true})");
}

TEST(JsonWriterTests, FormattedObject)
{
    serial::json::JsonWriter writer(2, false);
    writer.begin_object();
    writer.write_key_value("width", 1920);
    writer.write_key_value("height", 1080);
    writer.end_object();

    const std::string expected =
      "{\n"
      "  \"width\": 1920,\n"
      "  \"height\": 1080\n"
      "}";

    EXPECT_EQ(writer.get(), expected);
}

TEST(JsonWriterTests, FormattedArray)
{
    serial::json::JsonWriter writer(2, false);
    writer.begin_array();
    writer.write_val(10);
    writer.write_val(20);
    writer.write_val(30);
    writer.end_array();

    const std::string expected =
      "[\n"
      "  10,\n"
      "  20,\n"
      "  30\n"
      "]";

    EXPECT_EQ(writer.get(), expected);
}

TEST(JsonWriterTests, NestedObjectAndArray)
{
    serial::json::JsonWriter writer(2, true);

    writer.begin_object();
    writer.write_key("user");
    writer.begin_object();
    writer.write_key_value("name", "Renderer");
    writer.end_object();

    writer.write_key("matrix");
    writer.begin_array();
    writer.write_val(1.0);
    writer.write_val(0.0);
    writer.write_val(0.0);
    writer.write_val(1.0);
    writer.end_array();
    writer.end_object();

    EXPECT_EQ(writer.get(), R"({"user":{"name":"Renderer"},"matrix":[1,0,0,1]})");
}

TEST(JsonWriterTests, StringEscaping)
{
    serial::json::JsonWriter writer;
    writer.write_val("Line 1\nLine 2\t\"Quotes\" & \\Backslash\\");
    EXPECT_EQ(writer.get(), R"("Line 1\nLine 2\t\"Quotes\" & \\Backslash\\")");
}

// NOLINTEND(readability-magic-numbers)
