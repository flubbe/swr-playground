#include <cstring>
#include <numeric>

#include <gtest/gtest.h>

#include "hasher.h"

TEST(HasherTests, EmptyHashIsDeterministic)
{
    Hasher h1;
    Hasher h2;

    EXPECT_EQ(h1.digest(), h2.digest());
}

TEST(HasherTests, SameInputProducesSameHash)
{
    Hasher h1;
    Hasher h2;

    std::string_view s = "Hello World";

    h1.update(std::span{s.data(), s.size()});
    h2.update(std::span{s.data(), s.size()});

    EXPECT_EQ(h1.digest(), h2.digest());
}

TEST(HasherTests, DifferentInputProducesDifferentHash)
{
    Hasher h1;
    Hasher h2;

    h1.update(std::span{"Hello", 5});
    h2.update(std::span{"Hallo", 5});

    EXPECT_NE(h1.digest(), h2.digest());
}

TEST(HasherTests, IncrementalHashMatchesSingleUpdate)
{
    Hasher h1;
    Hasher h2;

    std::string_view text = "Hello World";

    h1.update(std::span{text.data(), text.size()});

    for(char c: text)
    {
        h2.update(std::span{reinterpret_cast<const std::byte*>(&c), 1});
    }

    EXPECT_EQ(h1.digest(), h2.digest());
}

TEST(HasherTests, SeedChangesHash)
{
    Hasher h1(0);
    Hasher h2(42);

    h1.update(std::span{"Hello", 5});
    h2.update(std::span{"Hello", 5});

    EXPECT_NE(h1.digest(), h2.digest());
}

TEST(HasherTests, MultipleDigestCallsReturnSameValue)
{
    Hasher h;

    h.update(std::span{"Hello", 5});

    auto d1 = h.digest();
    auto d2 = h.digest();

    EXPECT_EQ(d1, d2);
}

TEST(HasherTests, HashingBytesIsDeterministic)
{
    std::vector<std::byte> data(1024);

    std::iota(
      reinterpret_cast<std::uint8_t*>(data.data()),
      reinterpret_cast<std::uint8_t*>(data.data()) + data.size(),
      0);

    Hasher h1;
    Hasher h2;

    h1.update(std::span{data.data(), data.size()});
    h2.update(std::span{data.data(), data.size()});

    EXPECT_EQ(h1.digest(), h2.digest());
}
