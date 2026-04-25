/**
 * @file cura_clusterer.cpp
 * @brief Duplicate clustering implementation using Union-Find
 */

#include "cura_clusterer.hpp"
#include <algorithm>
#include <filesystem>

namespace cura {

CuraClusterer::CuraClusterer() = default;
CuraClusterer::~CuraClusterer() = default;

// UnionFind implementation
CuraClusterer::UnionFind::UnionFind(size_t n) {
    parent_.resize(n);
    rank_.resize(n, 0);
    for (size_t i = 0; i < n; ++i) {
        parent_[i] = i;
    }
}

size_t CuraClusterer::UnionFind::find(size_t x) {
    if (parent_[x] != x) {
        parent_[x] = find(parent_[x]); // Path compression
    }
    return parent_[x];
}

void CuraClusterer::UnionFind::union_sets(size_t x, size_t y) {
    size_t px = find(x);
    size_t py = find(y);

    if (px == py) return;

    // Union by rank
    if (rank_[px] < rank_[py]) {
        parent_[px] = py;
    } else if (rank_[px] > rank_[py]) {
        parent_[py] = px;
    } else {
        parent_[py] = px;
        rank_[px]++;
    }
}

std::vector<std::set<size_t>> CuraClusterer::UnionFind::get_groups() {
    std::unordered_map<size_t, std::set<size_t>> groups_map;
    for (size_t i = 0; i < parent_.size(); ++i) {
        groups_map[find(i)].insert(i);
    }

    std::vector<std::set<size_t>> groups;
    for (auto& [root, indices] : groups_map) {
        if (indices.size() > 1) { // Only return groups with duplicates
            groups.push_back(std::move(indices));
        }
    }

    return groups;
}

std::vector<DuplicateGroup> CuraClusterer::cluster(
    const std::vector<HashResult>& hash_results,
    bool enable_visual,
    uint64_t similarity_threshold,
    ClusterProgressCallback callback
) {
    std::vector<DuplicateGroup> groups;

    if (hash_results.empty()) {
        return groups;
    }

    UnionFind uf(hash_results.size());

    // Group by exact hash (mandatory, always done)
    std::unordered_map<uint64_t, std::vector<size_t>> exact_hash_map;
    for (size_t i = 0; i < hash_results.size(); ++i) {
        exact_hash_map[hash_results[i].exact_hash].push_back(i);
    }

    for (const auto& [hash, indices] : exact_hash_map) {
        for (size_t j = 1; j < indices.size(); ++j) {
            uf.union_sets(indices[0], indices[j]);
        }
    }

    // Group by visual similarity (if enabled)
    // This is O(n²) but necessary for similarity matching
    // Can be optimized with locality-sensitive hashing for large datasets
    if (enable_visual) {
        // Collect files that have perceptual hashes
        std::vector<size_t> visual_candidates;
        for (size_t i = 0; i < hash_results.size(); ++i) {
            if (hash_results[i].perceptual_hash != 0) {
                visual_candidates.push_back(i);
            }
        }

        const size_t candidate_count = visual_candidates.size();
        const size_t total_pairs = candidate_count > 1
            ? (candidate_count * (candidate_count - 1)) / 2
            : 0;
        size_t processed_pairs = 0;

        // Compare all pairs
        for (size_t i = 0; i < visual_candidates.size(); ++i) {
            for (size_t j = i + 1; j < visual_candidates.size(); ++j) {
                size_t idx_i = visual_candidates[i];
                size_t idx_j = visual_candidates[j];

                // Check both pHash and dHash
                uint64_t phash_dist = CuraHasher::hamming_distance(
                    hash_results[idx_i].perceptual_hash,
                    hash_results[idx_j].perceptual_hash
                );

                uint64_t dhash_dist = CuraHasher::hamming_distance(
                    hash_results[idx_i].difference_hash,
                    hash_results[idx_j].difference_hash
                );

                // Use minimum distance of both hashes
                uint64_t min_dist = std::min(phash_dist, dhash_dist);

                if (min_dist <= similarity_threshold) {
                    uf.union_sets(idx_i, idx_j);
                }

                ++processed_pairs;
                if (callback && (processed_pairs % 1000 == 0 || processed_pairs == total_pairs)) {
                    callback(processed_pairs, total_pairs);
                }
            }
        }
    } else if (callback) {
        callback(1, 1);
    }

    // Convert UnionFind groups to DuplicateGroups
    auto uf_groups = uf.get_groups();

    for (const auto& indices : uf_groups) {
        DuplicateGroup group;
        group.is_visual_duplicate = false;
        group.similarity_score = 0;

        // Collect files and calculate sizes
        group.total_size = 0;
        uint64_t best_file_size = 0;

        for (size_t idx : indices) {
            const auto& hash_result = hash_results[idx];
            group.files.push_back(hash_result.file_path);
            group.total_size += hash_result.file_size;

            // Check if this group is a visual duplicate (not exact match)
            if (hash_result.perceptual_hash != 0) {
                // Check if files in this group have different exact hashes
                // (meaning they're similar but not identical)
                // This will be determined after we have all files
            }
        }

        // Find the best file to keep
        group.best_file = find_best_file(hash_results, indices);

        // Calculate the best file's size
        for (size_t idx : indices) {
            if (hash_results[idx].file_path == group.best_file) {
                best_file_size = hash_results[idx].file_size;
                break;
            }
        }

        // Calculate save size: total - largest file (we keep the largest/best)
        group.save_size = group.total_size - best_file_size;

        // Determine if this is a visual duplicate group
        // (files with different exact hashes but similar perceptual hashes)
        if (indices.size() >= 2) {
            uint64_t first_hash = hash_results[*indices.begin()].exact_hash;
            bool all_same_hash = true;
            for (size_t idx : indices) {
                if (hash_results[idx].exact_hash != first_hash) {
                    all_same_hash = false;
                    break;
                }
            }
            group.is_visual_duplicate = !all_same_hash;

            // Calculate average similarity score for visual duplicates
            if (group.is_visual_duplicate && enable_visual) {
                uint64_t total_dist = 0;
                size_t count = 0;
                for (auto it1 = indices.begin(); it1 != indices.end(); ++it1) {
                    for (auto it2 = std::next(it1); it2 != indices.end(); ++it2) {
                        total_dist += CuraHasher::hamming_distance(
                            hash_results[*it1].perceptual_hash,
                            hash_results[*it2].perceptual_hash
                        );
                        ++count;
                    }
                }
                group.similarity_score = count > 0 ? total_dist / count : 0;
            }
        }

        groups.push_back(group);
    }

    // Sort groups by save size (descending)
    std::sort(groups.begin(), groups.end(),
              [](const DuplicateGroup& a, const DuplicateGroup& b) {
                  return a.save_size > b.save_size;
              });

    // Update stats
    stats_.total_files = hash_results.size();
    stats_.duplicate_groups = groups.size();
    stats_.duplicate_files = 0;
    stats_.total_save_size = 0;

    for (const auto& g : groups) {
        stats_.duplicate_files += g.files.size() - 1;
        stats_.total_save_size += g.save_size;
    }

    return groups;
}

CuraClusterer::Stats CuraClusterer::get_stats() const {
    return stats_;
}

std::string CuraClusterer::find_best_file(
    const std::vector<HashResult>& hash_results,
    const std::set<size_t>& indices
) {
    if (indices.empty()) return "";

    // Helper function to score filename (lower is better - more likely to be original)
    auto filename_score = [](const std::string& path) -> int {
        std::filesystem::path p(path);
        std::string filename = p.filename().string();

        // Convert to lowercase for checking
        std::string lower;
        for (char c : filename) {
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        int score = 0;

        // Penalize "copy" in filename
        if (lower.find("copy") != std::string::npos) {
            score += 100;
        }

        // Penalize numbered copies like "copy (2)", "copy (3)"
        size_t paren_pos = lower.find("copy (");
        if (paren_pos != std::string::npos) {
            // Extract number in parentheses
            size_t num_start = lower.find('(', paren_pos);
            size_t num_end = lower.find(')', num_start);
            if (num_start != std::string::npos && num_end != std::string::npos) {
                std::string num_str = lower.substr(num_start + 1, num_end - num_start - 1);
                try {
                    int copy_num = std::stoi(num_str);
                    score += copy_num * 10; // Higher number = worse score
                } catch (...) {}
            }
        }

        return score;
    };

    // Select the best file based on:
    // 1. Filename score (prefer original over "Copy" over "Copy (2)")
    // 2. Earliest modification time (original file is usually oldest)
    // 3. Largest file size (usually means less compression)
    // 4. Highest resolution

    size_t best_idx = *indices.begin();
    int best_filename_score = filename_score(hash_results[best_idx].file_path);
    std::filesystem::file_time_type best_time{};
    uint64_t best_file_size = 0;
    uint64_t best_resolution = 0;

    // Get initial best time
    try {
        best_time = std::filesystem::last_write_time(hash_results[best_idx].file_path);
    } catch (...) {}

    for (size_t idx : indices) {
        const auto& result = hash_results[idx];

        int current_score = filename_score(result.file_path);

        // Get file modification time
        std::filesystem::file_time_type mod_time{};
        try {
            mod_time = std::filesystem::last_write_time(result.file_path);
        } catch (...) {
            mod_time = best_time; // Default to best if can't read
        }

        // Calculate resolution
        uint64_t resolution = static_cast<uint64_t>(result.width) * result.height;

        bool is_better = false;

        // First priority: Lower filename score (original vs copy)
        if (current_score < best_filename_score) {
            is_better = true;
        } else if (current_score == best_filename_score) {
            // Second priority: Earlier modification time
            if (mod_time < best_time) {
                is_better = true;
            } else if (mod_time == best_time) {
                // Third priority: Larger file size
                if (result.file_size > best_file_size) {
                    is_better = true;
                } else if (result.file_size == best_file_size) {
                    // Fourth priority: Higher resolution
                    if (resolution > best_resolution) {
                        is_better = true;
                    }
                }
            }
        }

        if (is_better) {
            best_idx = idx;
            best_filename_score = current_score;
            best_time = mod_time;
            best_file_size = result.file_size;
            best_resolution = resolution;
        }
    }

    return hash_results[best_idx].file_path;
}

} // namespace cura
