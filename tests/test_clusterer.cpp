/**
 * @file test_clusterer.cpp
 * @brief Unit tests for CuraClusterer
 */

#include <catch2/catch_test_macros.hpp>
#include "cura_clusterer.hpp"

TEST_CASE("CuraClusterer initialization", "[clusterer]") {
    cura::CuraClusterer clusterer;

    SECTION("Empty input produces empty output") {
        std::vector<cura::HashResult> empty;
        auto groups = clusterer.cluster(empty, false);
        REQUIRE(groups.empty());
    }
}

TEST_CASE("CuraClusterer exact hash grouping", "[clusterer]") {
    cura::CuraClusterer clusterer;

    SECTION("Identical hashes are grouped") {
        std::vector<cura::HashResult> results = {
            {"file1.jpg", 12345, 0, 0, 100, 100, 1000},
            {"file2.jpg", 12345, 0, 0, 100, 100, 1000},
            {"file3.jpg", 67890, 0, 0, 100, 100, 1000}
        };

        auto groups = clusterer.cluster(results, false);
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].files.size() == 2);
    }
}

TEST_CASE("CuraClusterer stats", "[clusterer]") {
    cura::CuraClusterer clusterer;

    SECTION("Stats are updated after clustering") {
        std::vector<cura::HashResult> results = {
            {"file1.jpg", 12345, 0, 0, 100, 100, 1000},
            {"file2.jpg", 12345, 0, 0, 100, 100, 1000}
        };

        clusterer.cluster(results, false);
        auto stats = clusterer.get_stats();
        REQUIRE(stats.total_files == 2);
        REQUIRE(stats.duplicate_groups == 1);
        REQUIRE(stats.duplicate_files == 1);
    }
}

// TODO: Add more tests
// - Test visual similarity grouping
// - Test mixed exact and visual grouping
// - Test best file selection