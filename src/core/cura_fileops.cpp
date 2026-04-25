/**
 * @file cura_fileops.cpp
 * @brief File operations implementation for handling duplicates and organizing by date
 */

#include "cura_fileops.hpp"
#include "cura_image.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
// Undef Windows macros that conflict with our enum
#ifdef DELETE
#undef DELETE
#endif
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

CuraFileOps::CuraFileOps() = default;
CuraFileOps::~CuraFileOps() = default;

bool CuraFileOps::operation_supports_undo(DuplicateAction action) const {
#ifdef _WIN32
    return action == DuplicateAction::MOVE_TO_DIR;
#else
    return true;
#endif
}

OperationResult CuraFileOps::execute(
    const std::vector<std::string>& files,
    DuplicateAction action,
    const std::string& target_dir
) {
    OperationResult result;
    result.action = action;
    result.target_dir = target_dir;
    result.success = true;
    result.affected_files.clear();
    moved_files_.clear();

    if (files.empty()) {
        return result;
    }

    // For MOVE_TO_DIR, ensure target exists
    if (action == DuplicateAction::MOVE_TO_DIR && !target_dir.empty()) {
        if (!ensure_directory_exists(target_dir)) {
            result.success = false;
            result.error_message = "Cannot create target directory: " + target_dir;
            return result;
        }
    }

    // Process each file
    for (const auto& file : files) {
        bool success = false;

        if (action == DuplicateAction::DELETE) {
            success = move_to_trash(file);
        } else if (action == DuplicateAction::MOVE_TO_DIR) {
            success = move_to_directory(file, target_dir);
        }

        if (success) {
            result.affected_files.push_back(file);
        } else {
            result.success = false;
            result.error_message = "Failed to process: " + file;
        }
    }

    // Store operation for undo only when this platform can restore it.
    if (!operation_supports_undo(action)) {
        last_operation_.reset();
        moved_files_.clear();
    } else if (!result.affected_files.empty()) {
        last_operation_ = result;
    }

    return result;
}

bool CuraFileOps::undo_last_operation() {
    if (!can_undo()) {
        return false;
    }

    bool all_restored = true;

    for (const auto& [original_path, current_path] : moved_files_) {
        if (!restore_file(original_path, current_path)) {
            all_restored = false;
        }
    }

    if (all_restored) {
        last_operation_.reset();
        moved_files_.clear();
    }

    return all_restored;
}

bool CuraFileOps::can_undo() const {
    return last_operation_.has_value() && !moved_files_.empty();
}

bool CuraFileOps::ensure_directory_exists(const std::string& dir) {
    try {
        auto path = utf8_to_path(dir);
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

bool CuraFileOps::move_to_trash(const std::string& file) {
    try {
        auto source_path = utf8_to_path(file);
        if (!std::filesystem::exists(source_path)) {
            return false;
        }

#ifdef _WIN32
        // Windows: Use SHFileOperation to move to Recycle Bin
        std::wstring wide_path = std::filesystem::canonical(source_path).wstring();
        wide_path += L'\0';  // Double null-terminated

        SHFILEOPSTRUCTW shfos = {0};
        shfos.wFunc = FO_DELETE;
        shfos.pFrom = wide_path.c_str();
        shfos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;

        int result = SHFileOperationW(&shfos);

        if (result == 0) {
            // Track for potential undo (Windows handles this via Recycle Bin)
            moved_files_[file] = "RECYCLE_BIN";
            return true;
        }
        return false;
#else
        // Linux/macOS: Try to use gio trash or move to .Trash
        // For simplicity, we'll move files to a .cura_trash folder
        std::filesystem::path trash_dir = source_path.parent_path() / ".cura_trash";

        if (!std::filesystem::exists(trash_dir)) {
            std::filesystem::create_directory(trash_dir);
        }

        std::filesystem::path target = trash_dir / source_path.filename();
        std::filesystem::path original_target = target;

        // Handle name conflicts
        int counter = 1;
        while (std::filesystem::exists(target)) {
            target = trash_dir / (source_path.stem().string() + "_" + std::to_string(counter) + source_path.extension().string());
            counter++;
        }

        std::filesystem::rename(source_path, target);
        moved_files_[file] = target.string();
        return true;
#endif
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

bool CuraFileOps::move_to_directory(const std::string& file, const std::string& target_dir) {
    try {
        auto source_path = utf8_to_path(file);
        if (!std::filesystem::exists(source_path)) {
            return false;
        }

        auto target_dir_path = utf8_to_path(target_dir);
        std::filesystem::path target = target_dir_path / source_path.filename();

        // Handle name conflicts
        int counter = 1;
        while (std::filesystem::exists(target)) {
            target = target_dir_path / (source_path.stem().string() + "_" + std::to_string(counter) + source_path.extension().string());
            counter++;
        }

        // Move the file
        std::filesystem::rename(source_path, target);

        // Track for undo
        moved_files_[file] = target.string();

        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

bool CuraFileOps::restore_file(const std::string& original_path, const std::string& current_path) {
    try {
#ifdef _WIN32
        // Windows: Files in Recycle Bin need special handling
        // For now, we can't easily restore from Recycle Bin programmatically
        // User can restore manually from Recycle Bin
        if (current_path == "RECYCLE_BIN") {
            return false;  // Can't auto-restore from Recycle Bin
        }
#endif

        auto current = utf8_to_path(current_path);
        if (!std::filesystem::exists(current)) {
            return false;
        }

        auto original = utf8_to_path(original_path);
        // Move back to original location
        std::filesystem::rename(current, original);
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

// Generate date-based folder path
static std::string generate_date_folder(const std::string& base_dir,
                                         const ImageDate& date,
                                         DateGranularity granularity) {
    std::ostringstream path;
    path << base_dir << "/" << date.year;

    if (granularity >= DateGranularity::MONTH) {
        path << "/" << std::setw(2) << std::setfill('0') << date.month;
    }
    if (granularity >= DateGranularity::DAY) {
        path << "/" << std::setw(2) << std::setfill('0') << date.day;
    }

    return path.str();
}

OperationResult CuraFileOps::organize_by_date(
    const std::vector<std::string>& files,
    const std::string& target_base_dir,
    DateGranularity granularity,
    ProgressCallback progress_cb
) {
    OperationResult result;
    result.action = DuplicateAction::ORGANIZE_BY_DATE;
    result.target_dir = target_base_dir;
    result.granularity = granularity;
    result.success = true;
    result.affected_files.clear();
    moved_files_.clear();

    if (files.empty()) {
        return result;
    }

    // Ensure base directory exists
    if (!ensure_directory_exists(target_base_dir)) {
        result.success = false;
        result.error_message = "Cannot create target base directory: " + target_base_dir;
        return result;
    }

    size_t total = files.size();
    size_t current = 0;

    // Process each file
    for (const auto& file : files) {
        ++current;

        // Report progress
        if (progress_cb) {
            progress_cb(current, total, file);
        }

        bool success = move_file_to_date_folder(file, target_base_dir, granularity);

        if (success) {
            result.affected_files.push_back(file);
        } else {
            result.success = false;
            // Continue processing other files even if one fails
            result.error_message = "Failed to process: " + file;
        }
    }

    // Store operation for undo
    if (!result.affected_files.empty()) {
        last_operation_ = result;
    } else {
        last_operation_.reset();
        moved_files_.clear();
    }

    return result;
}

bool CuraFileOps::move_file_to_date_folder(const std::string& file,
                                            const std::string& target_base_dir,
                                            DateGranularity granularity) {
    try {
        auto source_path = utf8_to_path(file);
        if (!std::filesystem::exists(source_path)) {
            return false;
        }

        // Extract date from image
        ImageDate date = extract_image_date(file);

        if (!date.valid) {
            // Move to "undated" folder if no valid date
            std::string undated_folder = target_base_dir + "/undated";
            ensure_directory_exists(undated_folder);
            return move_to_directory(file, undated_folder);
        }

        // Generate date-based folder path
        std::string date_folder = generate_date_folder(target_base_dir, date, granularity);

        // Ensure date folder exists
        if (!ensure_directory_exists(date_folder)) {
            return false;
        }

        // Move file to date folder
        auto target_dir_path = utf8_to_path(date_folder);
        std::filesystem::path target = target_dir_path / source_path.filename();

        // Handle name conflicts - files from different folders may have same name
        int counter = 1;
        while (std::filesystem::exists(target)) {
            std::ostringstream new_name;
            new_name << source_path.stem().string() << "_" << counter << source_path.extension().string();
            target = target_dir_path / new_name.str();
            counter++;
        }

        // Move the file
        std::filesystem::rename(source_path, target);

        // Track for undo
        moved_files_[file] = target.string();

        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

} // namespace cura
