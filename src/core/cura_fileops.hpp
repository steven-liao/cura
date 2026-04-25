/**
 * @file cura_fileops.hpp
 * @brief File operations for handling duplicates (delete/move) and organizing by date
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>

namespace cura {

/**
 * @brief Action to take on duplicate files
 */
enum class DuplicateAction {
    DELETE,          // Move to trash/recycle bin
    MOVE_TO_DIR,     // Move to specified folder
    ORGANIZE_BY_DATE // Move to date-based folder structure
};

/**
 * @brief Date folder granularity for organize-by-date feature
 */
enum class DateGranularity {
    YEAR,   // YYYY
    MONTH,  // YYYY/MM (default)
    DAY     // YYYY/MM/DD
};

/**
 * @brief Progress callback for long operations
 * @param current Current file index
 * @param total Total files to process
 * @param current_file Current file path being processed
 */
using ProgressCallback = std::function<void(size_t current, size_t total, const std::string& current_file)>;

/**
 * @brief Result of a file operation
 */
struct OperationResult {
    std::vector<std::string> affected_files;
    DuplicateAction action;
    std::string target_dir;          // For MOVE_TO_DIR and ORGANIZE_BY_DATE
    DateGranularity granularity;     // For ORGANIZE_BY_DATE
    bool success;
    std::string error_message;       // If not successful
};

/**
 * @brief File operations handler
 */
class CuraFileOps {
public:
    CuraFileOps();
    ~CuraFileOps();

    /**
     * @brief Execute file operation on duplicate files
     * @param files List of file paths to handle
     * @param action Action to perform (DELETE or MOVE_TO_DIR)
     * @param target_dir Target directory for MOVE_TO_DIR (optional)
     * @return Operation result
     */
    OperationResult execute(
        const std::vector<std::string>& files,
        DuplicateAction action,
        const std::string& target_dir = ""
    );

    /**
     * @brief Organize files into date-based folder structure
     * @param files List of file paths to organize
     * @param target_base_dir Base directory for date folders
     * @param granularity Date folder granularity (YEAR/MONTH/DAY)
     * @param progress_cb Optional progress callback
     * @return Operation result
     */
    OperationResult organize_by_date(
        const std::vector<std::string>& files,
        const std::string& target_base_dir,
        DateGranularity granularity = DateGranularity::MONTH,
        ProgressCallback progress_cb = nullptr
    );

    /**
     * @brief Undo the last operation
     * @return True if undo was successful
     */
    bool undo_last_operation();

    /**
     * @brief Check if undo is available
     */
    bool can_undo() const;

    /**
     * @brief Create target directory if it doesn't exist
     * @param dir Directory path
     * @return True if directory exists or was created
     */
    bool ensure_directory_exists(const std::string& dir);

private:
    std::optional<OperationResult> last_operation_;

    // Track where files were moved for undo
    std::unordered_map<std::string, std::string> moved_files_;

    bool operation_supports_undo(DuplicateAction action) const;
    bool move_to_trash(const std::string& file);
    bool move_to_directory(const std::string& file, const std::string& target_dir);
    bool move_file_to_date_folder(const std::string& file,
                                   const std::string& target_base_dir,
                                   DateGranularity granularity);
    bool restore_file(const std::string& original_path, const std::string& current_path);
};

} // namespace cura
