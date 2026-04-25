/**
 * @file cura_image.cpp
 * @brief Image processing implementation using stb_image
 */

#include "cura_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../../third_party/stb_image_resize2.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#undef max
#undef min
#endif

namespace cura {

// Helper to read file into memory with UTF-8 path support
static std::vector<uint8_t> read_file_to_memory(const std::string& file_path) {
    std::vector<uint8_t> data;

#ifdef _WIN32
    // Convert UTF-8 to wide string
    int width = MultiByteToWideChar(CP_UTF8, 0, file_path.c_str(), -1, nullptr, 0);
    if (width > 0) {
        std::wstring wide_path(width - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, file_path.c_str(), -1, &wide_path[0], width);

        std::ifstream file(wide_path, std::ios::binary | std::ios::ate);
        if (file) {
            auto size = file.tellg();
            file.seekg(0, std::ios::beg);
            data.resize(static_cast<size_t>(size));
            file.read(reinterpret_cast<char*>(data.data()), size);
        }
    }
#else
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (file) {
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        data.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
    }
#endif

    return data;
}

// Parse EXIF orientation from JPEG data
// Returns orientation value (1-8), or 0 if not found
static int get_exif_orientation(const uint8_t* data, int data_size) {
    // Check for JPEG signature
    if (data_size < 4 || data[0] != 0xFF || data[1] != 0xD8) {
        return 0; // Not a JPEG
    }

    // Search for EXIF marker (0xFFE1)
    int pos = 2;
    while (pos < data_size - 4) {
        if (data[pos] != 0xFF) break;

        uint8_t marker = data[pos + 1];
        if (marker == 0xE1) {
            // Found APP1 marker (EXIF)
            int segment_size = (data[pos + 2] << 8) | data[pos + 3];
            if (pos + 4 + segment_size > data_size) break;

            const uint8_t* exif_data = data + pos + 4;
            int exif_size = segment_size - 2;

            // Check for "Exif\0\0" header
            if (exif_size > 8 && exif_data[0] == 'E' && exif_data[1] == 'x' &&
                exif_data[2] == 'i' && exif_data[3] == 'f' &&
                exif_data[4] == 0 && exif_data[5] == 0) {

                const uint8_t* tiff = exif_data + 6;
                int tiff_size = exif_size - 6;

                if (tiff_size < 8) break;

                // Check byte order
                bool big_endian = (tiff[0] == 'M' && tiff[1] == 'M');
                bool little_endian = (tiff[0] == 'I' && tiff[1] == 'I');

                if (!big_endian && !little_endian) break;

                auto read16 = [&](const uint8_t* p) -> uint16_t {
                    return big_endian ? (p[0] << 8) | p[1] : p[0] | (p[1] << 8);
                };
                auto read32 = [&](const uint8_t* p) -> uint32_t {
                    return big_endian ? (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]
                                      : p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
                };

                uint32_t ifd_offset = read32(tiff + 4);
                if (ifd_offset + 2 > (uint32_t)tiff_size) break;

                const uint8_t* ifd = tiff + ifd_offset;
                uint16_t num_entries = read16(ifd);

                // Search for Orientation tag (0x0112)
                for (uint16_t i = 0; i < num_entries; ++i) {
                    int entry_pos = 2 + i * 12;
                    if (entry_pos + 12 > tiff_size) break;

                    const uint8_t* entry = ifd + entry_pos;
                    uint16_t tag = read16(entry);

                    if (tag == 0x0112) {
                        // Found orientation tag
                        uint16_t value = read16(entry + 8);
                        return static_cast<int>(value);
                    }
                }
            }
            break;
        } else if (marker == 0xDA) {
            // Start of scan - no more markers
            break;
        } else if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            // Standalone markers
            pos += 2;
        } else {
            // Skip this segment
            int segment_size = (data[pos + 2] << 8) | data[pos + 3];
            pos += 2 + segment_size;
        }
    }

    return 0;
}

// Parse EXIF date from JPEG data
// Returns date string in format "YYYY:MM:DD HH:MM:SS", or empty if not found
// Priority: 0x9003 (DateTimeOriginal) > 0x0132 (DateTime) > 0x9004 (CreateDate)
static std::string get_exif_date_string(const uint8_t* data, int data_size) {
    // Check for JPEG signature
    if (data_size < 4 || data[0] != 0xFF || data[1] != 0xD8) {
        return ""; // Not a JPEG
    }

    // Search for EXIF marker (0xFFE1)
    int pos = 2;
    while (pos < data_size - 4) {
        if (data[pos] != 0xFF) break;

        uint8_t marker = data[pos + 1];
        if (marker == 0xE1) {
            // Found APP1 marker (EXIF)
            int segment_size = (data[pos + 2] << 8) | data[pos + 3];
            if (pos + 4 + segment_size > data_size) break;

            const uint8_t* exif_data = data + pos + 4;
            int exif_size = segment_size - 2;

            // Check for "Exif\0\0" header
            if (exif_size > 8 && exif_data[0] == 'E' && exif_data[1] == 'x' &&
                exif_data[2] == 'i' && exif_data[3] == 'f' &&
                exif_data[4] == 0 && exif_data[5] == 0) {

                const uint8_t* tiff = exif_data + 6;
                int tiff_size = exif_size - 6;

                if (tiff_size < 8) break;

                // Check byte order
                bool big_endian = (tiff[0] == 'M' && tiff[1] == 'M');
                bool little_endian = (tiff[0] == 'I' && tiff[1] == 'I');

                if (!big_endian && !little_endian) break;

                auto read16 = [&](const uint8_t* p) -> uint16_t {
                    return big_endian ? (p[0] << 8) | p[1] : p[0] | (p[1] << 8);
                };
                auto read32 = [&](const uint8_t* p) -> uint32_t {
                    return big_endian ? (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]
                                      : p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
                };

                uint32_t ifd_offset = read32(tiff + 4);
                if (ifd_offset + 2 > (uint32_t)tiff_size) break;

                const uint8_t* ifd = tiff + ifd_offset;
                uint16_t num_entries = read16(ifd);

                // Priority order for date tags
                const uint16_t date_tags[] = {0x9003, 0x0132, 0x9004}; // DateTimeOriginal, DateTime, CreateDate

                for (const uint16_t target_tag : date_tags) {
                    // Search for date tag
                    for (uint16_t i = 0; i < num_entries; ++i) {
                        int entry_pos = 2 + i * 12;
                        if (entry_pos + 12 > tiff_size) break;

                        const uint8_t* entry = ifd + entry_pos;
                        uint16_t tag = read16(entry);

                        if (tag == target_tag) {
                            // Found date tag
                            uint16_t type = read16(entry + 2);
                            uint32_t count = read32(entry + 4);

                            // ASCII string type is 2, count should be 20 for date format
                            if (type == 2 && count >= 20) {
                                // Get value offset
                                uint32_t value_offset = read32(entry + 8);

                                // If value fits in 4 bytes, it's stored inline
                                const uint8_t* value_ptr;
                                if (value_offset + count <= (uint32_t)tiff_size) {
                                    value_ptr = tiff + value_offset;
                                } else {
                                    // Value stored inline (for small values)
                                    value_ptr = entry + 8;
                                }

                                // Extract date string (format: "YYYY:MM:DD HH:MM:SS")
                                if (value_ptr + 19 <= tiff + tiff_size) {
                                    std::string date_str(value_ptr, value_ptr + 19);
                                    // Validate format
                                    if (date_str.length() >= 10 &&
                                        date_str[4] == ':' && date_str[7] == ':') {
                                        return date_str;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        } else if (marker == 0xDA) {
            // Start of scan - no more markers
            break;
        } else if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            // Standalone markers
            pos += 2;
        } else {
            // Skip this segment
            int segment_size = (data[pos + 2] << 8) | data[pos + 3];
            pos += 2 + segment_size;
        }
    }

    return "";
}

// Parse EXIF date string "YYYY:MM:DD HH:MM:SS" into ImageDate
static ImageDate parse_exif_date_string(const std::string& date_str) {
    ImageDate date;
    date.valid = false;

    if (date_str.length() < 10) {
        return date;
    }

    try {
        // Format: "YYYY:MM:DD HH:MM:SS"
        date.year = std::stoi(date_str.substr(0, 4));
        date.month = std::stoi(date_str.substr(5, 2));
        date.day = std::stoi(date_str.substr(8, 2));

        // Validate values
        if (date.year >= 1900 && date.year <= 2100 &&
            date.month >= 1 && date.month <= 12 &&
            date.day >= 1 && date.day <= 31) {
            date.valid = true;
        }
    } catch (...) {
        date.valid = false;
    }

    return date;
}

// Get file modification time as fallback
static ImageDate get_file_date(const std::string& file_path) {
    ImageDate date;
    date.valid = false;

    try {
#ifdef _WIN32
        // Convert UTF-8 to wide string
        int width = MultiByteToWideChar(CP_UTF8, 0, file_path.c_str(), -1, nullptr, 0);
        if (width > 0) {
            std::wstring wide_path(width - 1, 0);
            MultiByteToWideChar(CP_UTF8, 0, file_path.c_str(), -1, &wide_path[0], width);

            WIN32_FILE_ATTRIBUTE_DATA file_attr;
            if (GetFileAttributesExW(wide_path.c_str(), GetFileExInfoStandard, &file_attr)) {
                // Convert FILETIME to year/month/day
                FILETIME ft = file_attr.ftLastWriteTime;

                // Convert to SYSTEMTIME
                SYSTEMTIME st;
                if (FileTimeToSystemTime(&ft, &st)) {
                    date.year = st.wYear;
                    date.month = st.wMonth;
                    date.day = st.wDay;
                    date.valid = true;
                }
            }
        }
#else
        // Use std::filesystem for non-Windows
        auto ftime = std::filesystem::last_write_time(file_path);
        // Convert file_time_type to system_clock
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() +
            std::chrono::system_clock::now()
        );
        auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
        std::tm* tm_val = std::localtime(&time_t_val);
        if (tm_val) {
            date.year = tm_val->tm_year + 1900;
            date.month = tm_val->tm_mon + 1;
            date.day = tm_val->tm_mday;
            date.valid = true;
        }
#endif
    } catch (...) {
        date.valid = false;
    }

    return date;
}

// Extract date from image file (EXIF preferred, file time fallback)
ImageDate extract_image_date(const std::string& file_path) {
    // First, try to read file and extract EXIF date
    auto file_data = read_file_to_memory(file_path);

    if (!file_data.empty()) {
        std::string date_str = get_exif_date_string(file_data.data(), static_cast<int>(file_data.size()));
        if (!date_str.empty()) {
            ImageDate date = parse_exif_date_string(date_str);
            if (date.valid) {
                return date;
            }
        }
    }

    // Fallback to file modification time
    return get_file_date(file_path);
}

// Apply EXIF orientation transformation to image pixels
static ImageData apply_exif_orientation(const ImageData& image, int orientation) {
    if (!image.valid || orientation < 2 || orientation > 8) {
        return image;
    }

    ImageData oriented = image;
    const bool swaps_dimensions = orientation >= 5 && orientation <= 8;
    oriented.width = swaps_dimensions ? image.height : image.width;
    oriented.height = swaps_dimensions ? image.width : image.height;
    oriented.pixels.assign(
        static_cast<size_t>(oriented.width) * oriented.height * image.channels,
        0
    );

    auto source_index = [&](int x, int y) -> size_t {
        return static_cast<size_t>((y * image.width + x) * image.channels);
    };
    auto target_index = [&](int x, int y) -> size_t {
        return static_cast<size_t>((y * oriented.width + x) * image.channels);
    };

    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            int new_x = x;
            int new_y = y;

            switch (orientation) {
                case 2:
                    new_x = image.width - 1 - x;
                    break;
                case 3:
                    new_x = image.width - 1 - x;
                    new_y = image.height - 1 - y;
                    break;
                case 4:
                    new_y = image.height - 1 - y;
                    break;
                case 5:
                    new_x = y;
                    new_y = x;
                    break;
                case 6:
                    new_x = image.height - 1 - y;
                    new_y = x;
                    break;
                case 7:
                    new_x = image.height - 1 - y;
                    new_y = image.width - 1 - x;
                    break;
                case 8:
                    new_x = y;
                    new_y = image.width - 1 - x;
                    break;
                default:
                    break;
            }

            const size_t src = source_index(x, y);
            const size_t dst = target_index(new_x, new_y);
            std::copy_n(
                image.pixels.begin() + static_cast<std::ptrdiff_t>(src),
                image.channels,
                oriented.pixels.begin() + static_cast<std::ptrdiff_t>(dst)
            );
        }
    }

    return oriented;
}

CuraImageProcessor::CuraImageProcessor() = default;
CuraImageProcessor::~CuraImageProcessor() = default;

ImageData CuraImageProcessor::load_image(const std::string& file_path, int max_size) {
    return decode_stb_image(file_path, max_size);
}

bool CuraImageProcessor::load_dimensions(const std::string& file_path, int& width, int& height) {
    auto file_data = read_file_to_memory(file_path);
    if (file_data.empty()) {
        return false;
    }

    int channels;
    int result = stbi_info_from_memory(file_data.data(), static_cast<int>(file_data.size()),
                                        &width, &height, &channels);
    return result != 0;
}

ImageData CuraImageProcessor::generate_thumbnail(const std::string& file_path, int thumbnail_size) {
    // Load image with size limit
    ImageData full = load_image(file_path, thumbnail_size * 2);

    if (!full.valid) {
        ImageData invalid;
        invalid.valid = false;
        return invalid;
    }

    // Calculate thumbnail dimensions maintaining aspect ratio
    int thumb_width, thumb_height;
    if (full.width > full.height) {
        thumb_width = thumbnail_size;
        thumb_height = static_cast<int>(static_cast<float>(full.height) * thumbnail_size / full.width);
    } else {
        thumb_height = thumbnail_size;
        thumb_width = static_cast<int>(static_cast<float>(full.width) * thumbnail_size / full.height);
    }

    // Ensure minimum size
    thumb_width = std::max(1, thumb_width);
    thumb_height = std::max(1, thumb_height);

    return resize(full, thumb_width, thumb_height);
}

ImageData CuraImageProcessor::resize(const ImageData& image, int target_width, int target_height) {
    if (!image.valid || image.pixels.empty()) {
        ImageData invalid;
        invalid.valid = false;
        return invalid;
    }

    // Ensure valid dimensions
    target_width = std::max(1, target_width);
    target_height = std::max(1, target_height);

    ImageData resized;
    resized.width = target_width;
    resized.height = target_height;
    resized.channels = image.channels;
    resized.valid = true;

    resized.pixels.resize(static_cast<size_t>(target_width) * target_height * image.channels);

    // Use stb_image_resize for high-quality resizing
    stbir_resize_uint8_linear(
        image.pixels.data(), image.width, image.height, 0,
        resized.pixels.data(), target_width, target_height, 0,
        static_cast<stbir_pixel_layout>(image.channels)
    );

    return resized;
}

ImageData CuraImageProcessor::to_grayscale(const ImageData& image) {
    if (!image.valid || image.pixels.empty()) {
        ImageData invalid;
        invalid.valid = false;
        return invalid;
    }

    ImageData gray;
    gray.width = image.width;
    gray.height = image.height;
    gray.channels = 1;
    gray.valid = true;

    gray.pixels.resize(static_cast<size_t>(image.width) * image.height);

    if (image.channels >= 3) {
        // RGB to grayscale using luminance formula
        for (size_t i = 0; i < gray.pixels.size(); ++i) {
            size_t idx = i * image.channels;
            // ITU-R BT.601 formula
            float luminance =
                0.299f * image.pixels[idx] +
                0.587f * image.pixels[idx + 1] +
                0.114f * image.pixels[idx + 2];
            gray.pixels[i] = static_cast<uint8_t>(std::clamp(luminance, 0.0f, 255.0f));
        }
    } else if (image.channels == 1) {
        // Already grayscale
        gray.pixels = image.pixels;
    } else if (image.channels == 2) {
        // Grayscale with alpha - take first channel
        for (size_t i = 0; i < gray.pixels.size(); ++i) {
            gray.pixels[i] = image.pixels[i * 2];
        }
    } else {
        // Fallback: take first channel
        for (size_t i = 0; i < gray.pixels.size(); ++i) {
            gray.pixels[i] = image.pixels[i * image.channels];
        }
    }

    return gray;
}

ImageData CuraImageProcessor::apply_orientation(const ImageData& image, int orientation) {
    return apply_exif_orientation(image, orientation);
}

bool CuraImageProcessor::is_valid_image(const std::string& file_path) {
    int width, height, channels;
    return stbi_info(file_path.c_str(), &width, &height, &channels) != 0;
}

ImageData CuraImageProcessor::decode_stb_image(const std::string& file_path, int max_size) {
    ImageData data;

    // Read file into memory (handles UTF-8 paths)
    auto file_data = read_file_to_memory(file_path);
    if (file_data.empty()) {
        data.valid = false;
        return data;
    }

    int width, height, channels;

    // Load from memory
    uint8_t* pixels = nullptr;

    if (max_size > 0) {
        // Load at reduced resolution using stb_image's built-in scaling
        pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()),
                                        &width, &height, &channels, 0);

        if (!pixels) {
            data.valid = false;
            return data;
        }

        ImageData decoded;
        decoded.width = width;
        decoded.height = height;
        decoded.channels = channels;
        decoded.valid = true;
        decoded.pixels.assign(
            pixels,
            pixels + static_cast<size_t>(width) * height * channels
        );
        stbi_image_free(pixels);

        const int orientation = get_exif_orientation(file_data.data(), static_cast<int>(file_data.size()));
        if (orientation > 1) {
            decoded = apply_exif_orientation(decoded, orientation);
        }

        // Check if we need to resize
        if (decoded.width > max_size || decoded.height > max_size) {
            int new_width, new_height;
            if (decoded.width > decoded.height) {
                new_width = max_size;
                new_height = static_cast<int>(static_cast<float>(decoded.height) * max_size / decoded.width);
            } else {
                new_height = max_size;
                new_width = static_cast<int>(static_cast<float>(decoded.width) * max_size / decoded.height);
            }

            new_width = std::max(1, new_width);
            new_height = std::max(1, new_height);

            // Resize using stb_image_resize
            std::vector<uint8_t> resized_pixels(
                static_cast<size_t>(new_width) * new_height * decoded.channels
            );

            stbir_resize_uint8_linear(
                decoded.pixels.data(), decoded.width, decoded.height, 0,
                resized_pixels.data(), new_width, new_height, 0,
                static_cast<stbir_pixel_layout>(decoded.channels)
            );

            data.pixels = std::move(resized_pixels);
            data.width = new_width;
            data.height = new_height;
            data.channels = decoded.channels;
        } else {
            // No resize needed
            data = std::move(decoded);
        }
    } else {
        // Load at full resolution
        pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()),
                                        &width, &height, &channels, 0);

        if (!pixels) {
            data.valid = false;
            return data;
        }

        data.pixels.resize(static_cast<size_t>(width) * height * channels);
        std::copy(pixels, pixels + static_cast<size_t>(width) * height * channels, data.pixels.begin());
        stbi_image_free(pixels);
        data.width = width;
        data.height = height;
        data.channels = channels;

        const int orientation = get_exif_orientation(file_data.data(), static_cast<int>(file_data.size()));
        if (orientation > 1) {
            data = apply_exif_orientation(data, orientation);
        }
    }

    if (data.channels == 0) {
        data.channels = channels;
    }
    data.valid = true;

    return data;
}

} // namespace cura
