/**
 * @file cura_clusterer.hpp
 * @brief Duplicate grouping using Union-Find algorithm
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include "cura_hasher.hpp"

namespace cura {

/**
 * @brief A group of duplicate files
 */
struct DuplicateGroup {
    std::vector<std::string> files;     // All files in the group
    std::string best_file;              // Recommended file to keep
    uint64_t total_size;                // Total size of duplicates
    uint64_t save_size;                 // Size that can be saved
    bool is_visual_duplicate;           // True if detected by perceptual hash
    uint64_t similarity_score;          // Hamming distance (for visual duplicates)
};

using ClusterProgressCallback = std::function<void(size_t current, size_t total)>;

/**
 * @brief Duplicate clusterer using Union-Find
 */
class CuraClusterer {
public:
    CuraClusterer();
    ~CuraClusterer();

    /**
     * @brief Cluster files into duplicate groups
     * @param hash_results List of hash results from CuraHasher
     * @param enable_visual Include visual similarity matches
     * @param similarity_threshold Max hamming distance for visual matches
     * @return List of duplicate groups
     */
    std::vector<DuplicateGroup> cluster(
        const std::vector<HashResult>& hash_results,
        bool enable_visual = false,
        uint64_t similarity_threshold = 10,
        ClusterProgressCallback callback = nullptr
    );

    /**
     * @brief Get statistics from clustering
     */
    struct Stats {
        size_t total_files;
        size_t duplicate_groups;
        size_t duplicate_files;
        uint64_t total_save_size;
    };
    Stats get_stats() const;

private:
    Stats stats_;

    // Union-Find implementation
    class UnionFind {
    public:
        UnionFind(size_t n);
        size_t find(size_t x);
        void union_sets(size_t x, size_t y);
        std::vector<std::set<size_t>> get_groups();

    private:
        std::vector<size_t> parent_;
        std::vector<size_t> rank_;
    };

    // Find best file to keep in a group
    std::string find_best_file(const std::vector<HashResult>& hash_results, const std::set<size_t>& indices);
};

} // namespace cura
