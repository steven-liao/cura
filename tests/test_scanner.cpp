/**
 * @file test_scanner.cpp
 * @brief Unit tests for CuraScanner
 */

#include <catch2/catch_test_macros.hpp>
#include "cura_scanner.hpp"

TEST_CASE("CuraScanner initialization", "[scanner]") {
    cura::CuraScanner scanner;

    SECTION("Default formats are set") {
        auto formats = scanner.get_supported_formats();
        REQUIRE(!formats.empty());
        REQUIRE(std::find(formats.begin(), formats.end(), ".jpg") != formats.end());
        REQUIRE(std::find(formats.begin(), formats.end(), ".png") != formats.end());
    }
}

TEST_CASE("CuraScanner custom formats", "[scanner]") {
    cura::CuraScanner scanner;

    SECTION("Can set custom formats") {
        scanner.set_supported_formats({".jpg", ".png"});
        auto formats = scanner.get_supported_formats();
        REQUIRE(formats.size() == 2);
    }
}

// TODO: Add more tests when fixtures are available
// - Test scanning directories
// - Test file filtering
// - Test magic byte detection