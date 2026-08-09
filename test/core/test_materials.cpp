#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "assets/material.h"

namespace fs = std::filesystem;

// Asset directory is defined via CMakeTests.txt.
// We also provide a fallback.
#ifndef ASSETS_SOURCE_DIR
#    define ASSETS_SOURCE_DIR "assets"
#endif

TEST(MaterialTests, Parse)
{
    const std::string material_json =
      "{\n"
      "  \"shader\": \"Test\",\n"
      "  \"textures\": [\"textures/wood.png\", \"textures/stone.png\"]\n"
      "}";

    assets::MaterialDesc desc;
    ASSERT_NO_THROW(desc = assets::load_material(material_json));

    EXPECT_EQ(desc.shader, "Test");

    const std::vector<std::filesystem::path> expected_textures = {
      "textures/wood.png",
      "textures/stone.png"};
    EXPECT_EQ(desc.textures, expected_textures);
}

TEST(MaterialTests, LoadAllMaterialAssets)
{
    const fs::path materials_dir = fs::path(ASSETS_SOURCE_DIR) / "materials";

    // Ensure the asset directory exists relative to the test runner working directory
    ASSERT_TRUE(fs::exists(materials_dir) && fs::is_directory(materials_dir))
      << "Materials directory does not exist: " << fs::absolute(materials_dir);

    size_t loaded_count = 0;

    for(const auto& entry: fs::recursive_directory_iterator(materials_dir))
    {
        if(entry.is_regular_file() && entry.path().extension() == ".json")
        {
            std::ifstream file(entry.path());
            ASSERT_TRUE(file.is_open()) << "Failed to open material file: " << entry.path();

            std::stringstream buffer;
            buffer << file.rdbuf();
            const std::string json_content = buffer.str();

            // Attempt deserialization for each file
            assets::MaterialDesc desc;
            EXPECT_NO_THROW({
                desc = assets::load_material(json_content);
            }) << "Failed to parse material file: "
               << entry.path();

            // Optional sanity checks
            EXPECT_FALSE(desc.shader.empty())
              << "Material has empty shader field in: " << entry.path();

            loaded_count++;
        }
    }

    // Guard against running against an empty directory by accident
    EXPECT_GT(loaded_count, 0u) << "No material JSON files were found to test!";
}