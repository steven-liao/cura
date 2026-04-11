/**
 * @file test_image.cpp
 * @brief Unit tests for CuraImageProcessor
 */

#include <catch2/catch_test_macros.hpp>
#include "cura_image.hpp"

TEST_CASE("CuraImageProcessor initialization", "[image]") {
    cura::CuraImageProcessor processor;

    SECTION("Processor can be created") {
        REQUIRE(true); // Just verify construction
    }
}

TEST_CASE("CuraImageProcessor grayscale conversion", "[image]") {
    cura::CuraImageProcessor processor;

    SECTION("RGB to grayscale") {
        cura::ImageData rgb;
        rgb.width = 2;
        rgb.height = 2;
        rgb.channels = 3;
        rgb.valid = true;
        rgb.pixels = {
            255, 0, 0,    // Red
            0, 255, 0,    // Green
            0, 0, 255,    // Blue
            128, 128, 128 // Gray
        };

        auto gray = processor.to_grayscale(rgb);
        REQUIRE(gray.valid);
        REQUIRE(gray.channels == 1);
        REQUIRE(gray.width == rgb.width);
        REQUIRE(gray.height == rgb.height);
    }

    SECTION("Invalid image returns invalid") {
        cura::ImageData invalid;
        invalid.valid = false;

        auto gray = processor.to_grayscale(invalid);
        REQUIRE(!gray.valid);
    }
}

TEST_CASE("CuraImageProcessor mirrored EXIF rotations", "[image]") {
    cura::CuraImageProcessor processor;

    cura::ImageData image;
    image.width = 2;
    image.height = 3;
    image.channels = 1;
    image.valid = true;
    image.pixels = {
        1, 2,
        3, 4,
        5, 6
    };

    SECTION("Orientation 5 transposes pixels") {
        auto oriented = processor.apply_orientation(image, 5);
        REQUIRE(oriented.valid);
        REQUIRE(oriented.width == 3);
        REQUIRE(oriented.height == 2);
        REQUIRE(oriented.pixels == std::vector<uint8_t>{1, 3, 5, 2, 4, 6});
    }

    SECTION("Orientation 7 mirrors across the anti-diagonal") {
        auto oriented = processor.apply_orientation(image, 7);
        REQUIRE(oriented.valid);
        REQUIRE(oriented.width == 3);
        REQUIRE(oriented.height == 2);
        REQUIRE(oriented.pixels == std::vector<uint8_t>{6, 4, 2, 5, 3, 1});
    }
}

// TODO: Add more tests with actual image files
// - Test image loading
// - Test dimension detection
// - Test thumbnail generation
// - Test resizing
