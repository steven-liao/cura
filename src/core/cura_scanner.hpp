/**
 * @file cura_scanner.hpp
 * @brief File scanner for detecting image files in directories
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <mutex>
#include <atomic>

namespace cura {

/**
 * @brief Image file metadata
 */
struct ImageInfo {
    std::filesystem::path path;
    uint64_t file_size;
    std::string extension;
    bool is_valid_image;
    int width;           // Image width (0 if unknown)
    int height;          // Image height (0 if unknown)
};

/**
 * @brief Progress callback for scanning
 * @param current Number of files scanned so far
 * @param total Estimated total files (0 if unknown)
 * @param current_file Path of current file being scanned
 */
using ScanProgressCallback = std::function<void(size_t current, size_t total, const std::string& current_file)>;

/**
 * @brief File scanner with parallel directory traversal
 *
 * High-performance scanner that uses multiple threads to traverse
 * directories and detect image files by extension and magic bytes.
 */
class CuraScanner {
public:
    CuraScanner();
    ~CuraScanner();

    /**
     * @brief Scan directories for image files
     * @param directories List of directories to scan
     * @param callback Progress callback (optional)
     * @return List of image file info
     */
    std::vector<ImageInfo> scan(
        const std::vector<std::string>& directories,
        ScanProgressCallback callback = nullptr
    );

    /**
     * @brief Set supported image formats
     * @param extensions List of extensions (e.g., {".jpg", ".png"})
     */
    void set_supported_formats(const std::vector<std::string>& extensions);

    /**
     * @brief Get supported image formats
     */
    const std::vector<std::string>& get_supported_formats() const;

    /**
     * @brief Cancel ongoing scan
     */
    void cancel();

    /**
     * @brief Check if scan is cancelled
     */
    bool is_cancelled() const;

    /**
     * @brief Enable/disable magic byte verification
     * @param enable True to enable magic byte checking
     */
    void set_verify_magic_bytes(bool enable);

    /**
     * @brief Set minimum file size filter
     * @param min_size Minimum file size in bytes (0 = no filter)
     */
    void set_min_file_size(uint64_t min_size);

private:
    std::vector<std::string> supported_formats_;
    std::atomic<bool> cancelled_;
    bool verify_magic_bytes_;
    uint64_t min_file_size_;
    std::mutex results_mutex_;

    /**
     * @brief Check if file has image extension
     */
    bool has_image_extension(const std::filesystem::path& path) const;

    /**
     * @brief Verify file is valid image by checking magic bytes
     * @param path File path
     * @return True if file appears to be a valid image
     */
    bool verify_image_magic_bytes(const std::filesystem::path& path) const;

    /**
     * @brief Detect image format from magic bytes
     * @param data First few bytes of file
     * @return Detected format string (e.g., "jpeg", "png") or empty if unknown
     */
    static std::string detect_format_from_magic(const std::vector<uint8_t>& data);

    /**
     * @brief Scan a single directory
     */
    void scan_directory(
        const std::string& directory,
        std::vector<ImageInfo>& results,
        ScanProgressCallback callback,
        std::atomic<size_t>& file_count
    );
};

} // namespace cura