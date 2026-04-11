/**
 * @file cura_scanner.cpp
 * @brief File scanner implementation with parallel traversal
 */

#include "cura_scanner.hpp"
#include "../threading/cura_threadpool.hpp"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <set>

namespace cura {

// Magic byte signatures for common image formats
struct MagicSignature {
    std::string format;
    std::vector<uint8_t> signature;
    size_t offset;
};

static const std::vector<MagicSignature> MAGIC_SIGNATURES = {
    {"jpeg", {0xFF, 0xD8, 0xFF}, 0},                    // JPEG
    {"png", {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, 0}, // PNG
    {"gif", {0x47, 0x49, 0x46, 0x38}, 0},               // GIF (GIF8)
    {"bmp", {0x42, 0x4D}, 0},                           // BMP (BM)
    {"webp", {0x52, 0x49, 0x46, 0x46}, 0},              // WebP (RIFF, check for WEBP)
    {"tiff_le", {0x49, 0x49, 0x2A, 0x00}, 0},           // TIFF little-endian
    {"tiff_be", {0x4D, 0x4D, 0x00, 0x2A}, 0},           // TIFF big-endian
    {"heic", {0x66, 0x74, 0x79, 0x70, 0x68, 0x65, 0x69, 0x63}, 4}, // HEIC (ftyp heic at offset 4)
    {"cr2", {0x49, 0x49, 0x2A, 0x00}, 0},               // Canon CR2 (same as TIFF)
    {"nef", {0x4D, 0x4D, 0x00, 0x2A}, 0},               // Nikon NEF (same as TIFF)
    {"arw", {0x49, 0x49, 0x2A, 0x00}, 0},               // Sony ARW (same as TIFF)
};

CuraScanner::CuraScanner()
    : cancelled_(false)
    , verify_magic_bytes_(true)
    , min_file_size_(0) {
    // Default supported formats
    supported_formats_ = {
        ".jpg", ".jpeg", ".jfif", ".png", ".gif", ".bmp", ".webp",
        ".tiff", ".tif", ".heic", ".heif",
        ".raw", ".cr2", ".nef", ".arw"
    };
}

CuraScanner::~CuraScanner() = default;

std::vector<ImageInfo> CuraScanner::scan(
    const std::vector<std::string>& directories,
    ScanProgressCallback callback
) {
    std::vector<ImageInfo> results;
    cancelled_ = false;

    if (directories.empty()) {
        return results;
    }

    // Collect image files
    std::vector<std::filesystem::path> image_paths;
    size_t total_files = 0;

    for (const auto& dir : directories) {
        if (cancelled_) break;

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                dir,
                std::filesystem::directory_options::skip_permission_denied
            )) {
                if (cancelled_) break;
                if (entry.is_regular_file()) {
                    total_files++;
                    if (has_image_extension(entry.path())) {
                        image_paths.push_back(entry.path());
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            continue;
        }
    }

    if (cancelled_) {
        return results;
    }

    // Process files
    size_t processed = 0;
    for (const auto& path : image_paths) {
        if (cancelled_) break;

        try {
            auto file_size = std::filesystem::file_size(path);

            // Skip files smaller than minimum size
            if (min_file_size_ > 0 && file_size < min_file_size_) {
                processed++;
                continue;
            }

            ImageInfo info;
            info.path = path;
            info.file_size = file_size;
            info.extension = path.extension().string();
            info.width = 0;
            info.height = 0;

            // Verify by magic bytes if enabled
            if (verify_magic_bytes_) {
                info.is_valid_image = verify_image_magic_bytes(path);
            } else {
                info.is_valid_image = true;
            }

            if (info.is_valid_image) {
                results.push_back(info);
            }

            processed++;
            if (callback && processed % 10 == 0) {
                callback(processed, total_files, path.string());
            }
        } catch (const std::filesystem::filesystem_error&) {
            processed++;
            continue;
        }
    }

    // Final callback
    if (callback) {
        callback(processed, total_files, "");
    }

    return results;
}

void CuraScanner::set_supported_formats(const std::vector<std::string>& extensions) {
    supported_formats_ = extensions;
}

const std::vector<std::string>& CuraScanner::get_supported_formats() const {
    return supported_formats_;
}

void CuraScanner::cancel() {
    cancelled_ = true;
}

bool CuraScanner::is_cancelled() const {
    return cancelled_;
}

void CuraScanner::set_verify_magic_bytes(bool enable) {
    verify_magic_bytes_ = enable;
}

void CuraScanner::set_min_file_size(uint64_t min_size) {
    min_file_size_ = min_size;
}

bool CuraScanner::has_image_extension(const std::filesystem::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (const auto& supported : supported_formats_) {
        if (ext == supported) {
            return true;
        }
    }
    return false;
}

bool CuraScanner::verify_image_magic_bytes(const std::filesystem::path& path) const {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }

        // Read enough bytes to check all signatures
        std::vector<uint8_t> header(16);
        file.read(reinterpret_cast<char*>(header.data()), header.size());
        auto bytes_read = file.gcount();
        header.resize(static_cast<size_t>(bytes_read));

        if (header.empty()) {
            return false;
        }

        // Check against known signatures
        for (const auto& sig : MAGIC_SIGNATURES) {
            if (sig.offset + sig.signature.size() > header.size()) {
                continue;
            }

            bool matches = true;
            for (size_t i = 0; i < sig.signature.size(); ++i) {
                if (header[sig.offset + i] != sig.signature[i]) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                // Special case for WebP: check for "WEBP" after "RIFF"
                if (sig.format == "webp") {
                    if (header.size() >= 12 &&
                        header[8] == 'W' && header[9] == 'E' &&
                        header[10] == 'B' && header[11] == 'P') {
                        return true;
                    }
                    continue;
                }
                return true;
            }
        }

        // If no magic byte match but file has image extension, accept it
        // This handles cases where we might not know all magic bytes
        return has_image_extension(path);

    } catch (const std::exception&) {
        return false;
    }
}

std::string CuraScanner::detect_format_from_magic(const std::vector<uint8_t>& data) {
    for (const auto& sig : MAGIC_SIGNATURES) {
        if (sig.offset + sig.signature.size() > data.size()) {
            continue;
        }

        bool matches = true;
        for (size_t i = 0; i < sig.signature.size(); ++i) {
            if (data[sig.offset + i] != sig.signature[i]) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return sig.format;
        }
    }
    return "";
}

void CuraScanner::scan_directory(
    const std::string& directory,
    std::vector<ImageInfo>& results,
    ScanProgressCallback callback,
    std::atomic<size_t>& file_count
) {
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
            directory,
            std::filesystem::directory_options::skip_permission_denied
        )) {
            if (cancelled_) break;

            if (entry.is_regular_file() && has_image_extension(entry.path())) {
                try {
                    auto file_size = std::filesystem::file_size(entry.path());

                    if (min_file_size_ > 0 && file_size < min_file_size_) {
                        continue;
                    }

                    ImageInfo info;
                    info.path = entry.path();
                    info.file_size = file_size;
                    info.extension = entry.path().extension().string();
                    info.width = 0;
                    info.height = 0;

                    if (verify_magic_bytes_) {
                        info.is_valid_image = verify_image_magic_bytes(entry.path());
                    } else {
                        info.is_valid_image = true;
                    }

                    if (info.is_valid_image) {
                        std::lock_guard<std::mutex> lock(results_mutex_);
                        results.push_back(info);
                    }

                    size_t count = ++file_count;
                    if (callback) {
                        callback(count, 0, entry.path().string());
                    }
                } catch (const std::filesystem::filesystem_error&) {
                    continue;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Skip directories we can't access
    }
}

} // namespace cura