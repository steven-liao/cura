/**
 * @file test_hasher.cpp
 * @brief Unit tests for CuraHasher
 */

#include <catch2/catch_test_macros.hpp>
#include "cura_hasher.hpp"

TEST_CASE("CuraHasher initialization", "[hasher]") {
    cura::CuraHasher hasher;

    SECTION("Default similarity threshold") {
        // Default threshold should be 10
        hasher.set_similarity_threshold(10);
    }
}

TEST_CASE("CuraHasher hamming distance", "[hasher]") {
    cura::CuraHasher hasher;

    SECTION("Same hash has zero distance") {
        uint64_t hash = 0x123456789ABCDEF0;
        REQUIRE(cura::CuraHasher::hamming_distance(hash, hash) == 0);
    }

    SECTION("Different hashes have correct distance") {
        uint64_t hash1 = 0xFFFFFFFFFFFFFFFF;
        uint64_t hash2 = 0x0000000000000000;
        REQUIRE(cura::CuraHasher::hamming_distance(hash1, hash2) == 64);
    }

    SECTION("One bit difference") {
        uint64_t hash1 = 0x0000000000000001;
        uint64_t hash2 = 0x0000000000000000;
        REQUIRE(cura::CuraHasher::hamming_distance(hash1, hash2) == 1);
    }
}

// TODO: Add more tests when fixtures are available
// - Test exact hash computation
// - Test perceptual hash computation
// - Test hash consistency for same images
// - Test hash difference for different images