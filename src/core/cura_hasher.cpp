/**
 * @file cura_hasher.cpp
 * @brief Hash computation implementation using xxHash and perceptual hashing
 */

#include "cura_hasher.hpp"
#include "cura_image.hpp"
#include "../threading/cura_threadpool.hpp"

// xxHash implementation
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "../../third_party/xxhash.h"

#include <fstream>
#include <algorithm>
#include <cmath>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#undef max
#undef min
#endif

namespace cura {

// Helper to convert UTF-8 string to filesystem path
static std::filesystem::path utf8_to_path(const std::string& utf8_str) {
#ifdef _WIN32
    // On Windows, convert UTF-8 to wide string first
    int width = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (width > 0) {
        std::wstring wide_str(width - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], width);
        return std::filesystem::path(wide_str);
    }
    return std::filesystem::path(utf8_str);
#else
    return std::filesystem::path(utf8_str);
#endif
}

// Constants for hashing
static constexpr size_t CHUNK_SIZE = 64 * 1024;  // 64KB chunks for streaming
static constexpr int PHASH_SIZE = 32;             // Resize to 32x32 for pHash
static constexpr int DHASH_SIZE = 9;              // 9x8 for dHash

CuraHasher::CuraHasher()
    : similarity_threshold_(10) {
}

CuraHasher::~CuraHasher() = default;

std::vector<HashResult> CuraHasher::compute_hashes(
    const std::vector<std::string>& files,
    bool enable_perceptual,
    HashProgressCallback callback
) {
    std::vector<HashResult> results;
    results.reserve(files.size());

    if (files.empty()) {
        return results;
    }

    size_t total = files.size();
    size_t completed = 0;

    for (const auto& file : files) {
        HashResult result;
        result.file_path = file;

        // Compute exact hash (mandatory)
        result.exact_hash = compute_exact_hash(file);

        // Compute perceptual hashes if enabled
        if (enable_perceptual) {
            result.perceptual_hash = compute_perceptual_hash(file);
            result.difference_hash = compute_difference_hash(file);
        } else {
            result.perceptual_hash = 0;
            result.difference_hash = 0;
        }

        // Get file size
        try {
            result.file_size = std::filesystem::file_size(file);
        } catch (...) {
            result.file_size = 0;
        }

        // Get image dimensions
        CuraImageProcessor img_proc;
        int width, height;
        if (img_proc.load_dimensions(file, width, height)) {
            result.width = static_cast<uint32_t>(width);
            result.height = static_cast<uint32_t>(height);
        } else {
            result.width = 0;
            result.height = 0;
        }

        results.push_back(result);
        completed++;

        // Report progress every 5 files
        if (callback && completed % 5 == 0) {
            callback(completed, total, file);
        }
    }

    // Final callback
    if (callback) {
        callback(completed, total, files.back());
    }

    return results;
}

uint64_t CuraHasher::compute_exact_hash(const std::string& file_path) {
    return compute_xxhash(file_path);
}

uint64_t CuraHasher::compute_xxhash(const std::string& file_path) {
    auto path = utf8_to_path(file_path);
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return 0;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    uint64_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Initialize xxHash state
    XXH64_state_t* state = XXH64_createState();
    if (!state) {
        return 0;
    }

    if (XXH64_reset(state, 0) != XXH_OK) {
        XXH64_freeState(state);
        return 0;
    }

    // Stream file in chunks for memory efficiency
    std::vector<char> buffer(CHUNK_SIZE);
    while (file) {
        file.read(buffer.data(), buffer.size());
        auto bytes_read = file.gcount();
        if (bytes_read > 0) {
            XXH64_update(state, buffer.data(), static_cast<size_t>(bytes_read));
        }
    }

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    // Combine hash with file size to reduce collisions
    // (same content but different size = different hash)
    return hash ^ (file_size * 0x9e3779b97f4a7c15ULL);
}

uint64_t CuraHasher::compute_perceptual_hash(const std::string& file_path) {
    std::vector<uint8_t> pixels;
    int width, height;

    if (!load_image_for_hash(file_path, pixels, width, height)) {
        return 0;
    }

    return compute_phash_from_pixels(pixels, width, height);
}

uint64_t CuraHasher::compute_difference_hash(const std::string& file_path) {
    std::vector<uint8_t> pixels;
    int width, height;

    if (!load_image_for_hash(file_path, pixels, width, height)) {
        return 0;
    }

    return compute_dhash_from_pixels(pixels, width, height);
}

bool CuraHasher::load_image_for_hash(
    const std::string& file_path,
    std::vector<uint8_t>& pixels,
    int& width,
    int& height
) {
    CuraImageProcessor img_proc;

    // Load image resized to hash size
    auto image = img_proc.load_image(file_path, PHASH_SIZE);
    if (!image.valid) {
        return false;
    }

    // Convert to grayscale
    auto gray = img_proc.to_grayscale(image);
    if (!gray.valid) {
        return false;
    }

    pixels = std::move(gray.pixels);
    width = gray.width;
    height = gray.height;

    return true;
}

uint64_t CuraHasher::compute_phash_from_pixels(
    const std::vector<uint8_t>& pixels,
    int width,
    int height
) {
    if (pixels.empty() || width <= 0 || height <= 0) {
        return 0;
    }

    // Simple average-based perceptual hash
    // For a proper implementation, use DCT (Discrete Cosine Transform)
    // This is a simplified version that works well for basic similarity detection

    // Calculate average
    uint64_t sum = 0;
    for (const auto& pixel : pixels) {
        sum += pixel;
    }
    uint8_t average = static_cast<uint8_t>(sum / pixels.size());

    // Generate hash based on whether each pixel is above or below average
    uint64_t hash = 0;
    size_t hash_bits = std::min(static_cast<size_t>(64), pixels.size());

    for (size_t i = 0; i < hash_bits; ++i) {
        if (pixels[i] > average) {
            hash |= (1ULL << i);
        }
    }

    return hash;
}

uint64_t CuraHasher::compute_dhash_from_pixels(
    const std::vector<uint8_t>& pixels,
    int width,
    int height
) {
    if (pixels.empty() || width < 2 || height < 1) {
        return 0;
    }

    // Difference hash: compare each pixel to the next one in the row
    // This creates a gradient-based hash that's robust to brightness changes

    uint64_t hash = 0;
    int bit = 0;

    // Compare adjacent pixels in each row
    for (int y = 0; y < height && bit < 64; ++y) {
        for (int x = 0; x < width - 1 && bit < 64; ++x) {
            size_t idx = static_cast<size_t>(y * width + x);
            size_t next_idx = idx + 1;

            if (next_idx < pixels.size()) {
                if (pixels[idx] < pixels[next_idx]) {
                    hash |= (1ULL << bit);
                }
                ++bit;
            }
        }
    }

    return hash;
}

uint64_t CuraHasher::hamming_distance(uint64_t hash1, uint64_t hash2) {
    uint64_t xor_result = hash1 ^ hash2;

    // Count set bits (popcount)
    // Use built-in if available, otherwise fallback
#if defined(__GNUC__) || defined(__clang__)
    return static_cast<uint64_t>(__builtin_popcountll(xor_result));
#elif defined(_MSC_VER)
    return static_cast<uint64_t>(__popcnt64(xor_result));
#else
    // Fallback: Brian Kernighan's algorithm
    uint64_t count = 0;
    while (xor_result) {
        xor_result &= xor_result - 1;
        ++count;
    }
    return count;
#endif
}

void CuraHasher::set_similarity_threshold(uint64_t threshold) {
    similarity_threshold_ = threshold;
}

} // namespace cura