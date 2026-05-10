#include <gtest/gtest.h>

#include "utils.h"

TEST(UtilsTests, ToLowerCopyHandlesEmptyString)
{
    EXPECT_EQ(to_lower_copy(""), "");
}

TEST(UtilsTests, ToLowerCopyLeavesLowercaseUnchanged)
{
    EXPECT_EQ(to_lower_copy("already_lower"), "already_lower");
}

TEST(UtilsTests, ToLowerCopyConvertsUppercaseAscii)
{
    EXPECT_EQ(to_lower_copy("MiXeD_CASE"), "mixed_case");
}

TEST(UtilsTests, ToLowerCopyLeavesNonAlphabeticCharactersUnchanged)
{
    EXPECT_EQ(to_lower_copy("A1! Z_"), "a1! z_");
}
