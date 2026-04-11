/**
 * @file test_fileops.cpp
 * @brief Unit tests for CuraFileOps
 */

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "cura_fileops.hpp"

namespace {

std::filesystem::path make_temp_dir() {
    auto dir = std::filesystem::temp_directory_path() / "cura-tests-fileops";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

void write_file(const std::filesystem::path& path, const char* content) {
    std::ofstream file(path, std::ios::binary);
    file << content;
}

}

TEST_CASE("CuraFileOps initialization", "[fileops]") {
    cura::CuraFileOps fileops;

    SECTION("No undo available initially") {
        REQUIRE(!fileops.can_undo());
    }
}

TEST_CASE("CuraFileOps directory creation", "[fileops]") {
    cura::CuraFileOps fileops;

    SECTION("Can create directory") {
        auto temp_dir = std::filesystem::temp_directory_path() / "cura-tests-create-dir";
        std::filesystem::remove_all(temp_dir);

        bool result = fileops.ensure_directory_exists(temp_dir.string());
        REQUIRE(result);

        std::filesystem::remove_all(temp_dir);
    }
}

TEST_CASE("CuraFileOps execute", "[fileops]") {
    cura::CuraFileOps fileops;

    SECTION("Empty file list succeeds") {
        auto result = fileops.execute({}, cura::DuplicateAction::DELETE);
        REQUIRE(result.success);
    }

    SECTION("Move operations can be undone") {
        auto temp_dir = make_temp_dir();
        auto source_dir = temp_dir / "source";
        auto target_dir = temp_dir / "target";
        std::filesystem::create_directories(source_dir);

        auto source_file = source_dir / "photo.jpg";
        write_file(source_file, "pixels");

        auto result = fileops.execute(
            { source_file.string() },
            cura::DuplicateAction::MOVE_TO_DIR,
            target_dir.string()
        );

        REQUIRE(result.success);
        REQUIRE(fileops.can_undo());
        REQUIRE(std::filesystem::exists(target_dir / "photo.jpg"));

        REQUIRE(fileops.undo_last_operation());
        REQUIRE(std::filesystem::exists(source_file));
        REQUIRE(!fileops.can_undo());

        std::filesystem::remove_all(temp_dir);
    }
}

// TODO: Add more tests with actual files
// - Test delete operation
// - Test move operation
// - Test undo operation
// - Test error handling
