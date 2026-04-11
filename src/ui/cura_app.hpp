/**
 * @file cura_app.hpp
 * @brief Slint UI application integration
 */

#pragma once

#include <slint.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <optional>
#include "../core/cura_scanner.hpp"
#include "../core/cura_hasher.hpp"
#include "../core/cura_clusterer.hpp"
#include "../core/cura_fileops.hpp"

// Include generated Slint header (generated from cura_main.slint)
#include "cura_main.h"

namespace cura {

/**
 * @brief Application state exposed to Slint UI
 */
class CuraApp {
public:
    CuraApp();
    ~CuraApp();

    /**
     * @brief Run the Slint UI application
     */
    void run();

    // --- UI Callbacks (bound from Slint) ---

    /**
     * @brief Add a folder to scan
     * @param folder_path Folder path
     */
    void add_folder(const std::string& folder_path);

    /**
     * @brief Remove a folder from scan list
     * @param folder_path Folder path
     */
    void remove_folder(const std::string& folder_path);

    /**
     * @brief Get selected folders
     */
    const std::vector<std::string>& get_folders() const;

    /**
     * @brief Start scanning selected folders
     * @param enable_visual Enable visual similarity detection
     * @param similarity_threshold Threshold for visual matching
     */
    void start_scan(bool enable_visual = false, uint64_t similarity_threshold = 10);

    /**
     * @brief Cancel ongoing scan
     */
    void cancel_scan();

    /**
     * @brief Get scan progress (0.0 - 1.0)
     */
    double get_scan_progress() const;

    /**
     * @brief Get duplicate groups
     */
    const std::vector<DuplicateGroup>& get_duplicate_groups() const;

    /**
     * @brief Handle duplicates (delete or move)
     * @param file_paths Files to handle
     * @param action DELETE or MOVE_TO_DIR
     * @param target_dir Target directory for move
     */
    bool handle_duplicates(
        const std::vector<std::string>& file_paths,
        DuplicateAction action,
        const std::string& target_dir = ""
    );

    /**
     * @brief Undo last file operation
     */
    bool undo_last_operation();

    /**
     * @brief Check if undo is available
     */
    bool can_undo() const;

    // --- Settings ---

    /**
     * @brief Set visual similarity enabled
     */
    void set_visual_similarity_enabled(bool enabled);

    /**
     * @brief Get visual similarity enabled
     */
    bool is_visual_similarity_enabled() const;

    /**
     * @brief Set move target folder
     */
    void set_move_target_folder(const std::string& folder);

    /**
     * @brief Get move target folder
     */
    const std::string& get_move_target_folder() const;

private:
    // Core components
    CuraScanner scanner_;
    CuraHasher hasher_;
    CuraClusterer clusterer_;
    CuraFileOps file_ops_;

    // State
    std::vector<std::string> selected_folders_;
    std::vector<DuplicateGroup> duplicate_groups_;
    bool visual_similarity_enabled_;
    uint64_t similarity_threshold_;
    std::string move_target_folder_;
    double scan_progress_;
    std::atomic<bool> scanning_{false};
    std::atomic<bool> cancelled_{false};

    // Slint window handle
    std::optional<slint::ComponentHandle<CuraMainWindow>> window_;

    // Background scan thread
    std::thread scan_thread_;
    std::mutex state_mutex_;

    // Internal helpers
    void update_folder_list();
    void update_folder_list_ui();
    void update_ui_state();
    void run_scan_thread(bool enable_visual, uint64_t similarity_threshold,
                         const std::vector<std::string>& folders);
    void bind_callbacks();
    std::string format_size(uint64_t bytes) const;
};

} // namespace cura