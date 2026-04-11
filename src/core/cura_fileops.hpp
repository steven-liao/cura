/**
 * @file cura_fileops.hpp
 * @brief File operations for handling duplicates (delete/move)
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace cura {

/**
 * @brief Action to take on duplicate files
 */
enum class DuplicateAction {
    DELETE,       // Move to trash/recycle bin
    MOVE_TO_DIR   // Move to specified folder
};

/**
 * @brief Result of a file operation
 */
struct OperationResult {
    std::vector<std::string> affected_files;
    DuplicateAction action;
    std::string target_dir;          // For MOVE_TO_DIR
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
    bool restore_file(const std::string& original_path, const std::string& current_path);
};

} // namespace cura
