/**
 * @file cura_image.hpp
 * @brief Image processing utilities
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace cura {

/**
 * @brief Image data structure
 */
struct ImageData {
    std::vector<uint8_t> pixels;
    int width;
    int height;
    int channels;
    bool valid;
};

/**
 * @brief Date extracted from image (EXIF or file time)
 */
struct ImageDate {
    int year;
    int month;
    int day;
    bool valid;
};

/**
 * @brief Extract date from image file
 * @param file_path Path to image file
 * @return ImageDate structure with year/month/day
 *
 * Priority: EXIF DateTimeOriginal (0x9003) > EXIF DateTime (0x0132) > file modification time
 */
ImageDate extract_image_date(const std::string& file_path);

/**
 * @brief Image processor for decoding and resizing
 */
class CuraImageProcessor {
public:
    CuraImageProcessor();
    ~CuraImageProcessor();

    /**
     * @brief Load an image file
     * @param file_path Path to image file
     * @param max_size Maximum dimension to load (resize if larger)
     * @return Image data
     */
    ImageData load_image(const std::string& file_path, int max_size = 0);

    /**
     * @brief Load image dimensions only (fast)
     * @param file_path Path to image file
     * @param width Output width
     * @param height Output height
     * @return True if successful
     */
    bool load_dimensions(const std::string& file_path, int& width, int& height);

    /**
     * @brief Generate thumbnail for an image
     * @param file_path Path to image file
     * @param thumbnail_size Target thumbnail size
     * @return Thumbnail image data
     */
    ImageData generate_thumbnail(const std::string& file_path, int thumbnail_size = 128);

    /**
     * @brief Resize image to target dimensions
     * @param image Source image
     * @param target_width Target width
     * @param target_height Target height
     * @return Resized image
     */
    ImageData resize(const ImageData& image, int target_width, int target_height);

    /**
     * @brief Convert image to grayscale
     * @param image Source image
     * @return Grayscale image (1 channel)
     */
    ImageData to_grayscale(const ImageData& image);

    /**
     * @brief Apply EXIF orientation to an already decoded image
     * @param image Source image
     * @param orientation EXIF orientation value (1-8)
     * @return Oriented image data
     */
    ImageData apply_orientation(const ImageData& image, int orientation);

    /**
     * @brief Check if file is a valid image
     * @param file_path Path to file
     * @return True if valid image
     */
    bool is_valid_image(const std::string& file_path);

private:
    // Decode using stb_image
    ImageData decode_stb_image(const std::string& file_path, int max_size);
};

} // namespace cura
