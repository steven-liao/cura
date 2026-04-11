/**
 * @file cura_hasher.hpp
 * @brief Hash computation for duplicate detection
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <filesystem>

namespace cura {

/**
 * @brief Hash result for a single file
 */
struct HashResult {
    std::string file_path;
    uint64_t exact_hash;       // xxHash (mandatory)
    uint64_t perceptual_hash;  // pHash (optional, 0 if disabled)
    uint64_t difference_hash;  // dHash (optional, 0 if disabled)
    uint32_t width;
    uint32_t height;
    uint64_t file_size;
};

/**
 * @brief Progress callback for hashing
 */
using HashProgressCallback = std::function<void(size_t current, size_t total, const std::string& current_file)>;

/**
 * @brief Hash engine for duplicate detection
 */
class CuraHasher {
public:
    CuraHasher();
    ~CuraHasher();

    /**
     * @brief Compute hashes for multiple files
     * @param files List of file paths
     * @param enable_perceptual Enable perceptual hash (optional)
     * @param callback Progress callback (optional)
     * @return List of hash results
     */
    std::vector<HashResult> compute_hashes(
        const std::vector<std::string>& files,
        bool enable_perceptual = false,
        HashProgressCallback callback = nullptr
    );

    /**
     * @brief Compute exact hash for a single file (xxHash)
     * @param file_path Path to file
     * @return 64-bit hash value
     */
    uint64_t compute_exact_hash(const std::string& file_path);

    /**
     * @brief Compute perceptual hash for a single file (pHash)
     * @param file_path Path to image file
     * @return 64-bit perceptual hash value
     */
    uint64_t compute_perceptual_hash(const std::string& file_path);

    /**
     * @brief Compute difference hash for a single file (dHash)
     * @param file_path Path to image file
     * @return 64-bit difference hash value
     */
    uint64_t compute_difference_hash(const std::string& file_path);

    /**
     * @brief Calculate hamming distance between two hashes
     * @param hash1 First hash
     * @param hash2 Second hash
     * @return Hamming distance (number of different bits)
     */
    static uint64_t hamming_distance(uint64_t hash1, uint64_t hash2);

    /**
     * @brief Set similarity threshold for perceptual matching
     * @param threshold Maximum hamming distance to consider similar
     */
    void set_similarity_threshold(uint64_t threshold);

private:
    uint64_t similarity_threshold_;

    // Internal helpers
    bool load_image_for_hash(const std::string& file_path, std::vector<uint8_t>& pixels, int& width, int& height);
    uint64_t compute_xxhash(const std::string& file_path);
    uint64_t compute_phash_from_pixels(const std::vector<uint8_t>& pixels, int width, int height);
    uint64_t compute_dhash_from_pixels(const std::vector<uint8_t>& pixels, int width, int height);
};

} // namespace cura