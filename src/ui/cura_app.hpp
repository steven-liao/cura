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

    // --- Organize by Date Feature ---

    /**
     * @brief Add a folder to organize list
     * @param folder_path Folder path
     */
    void organize_add_folder(const std::string& folder_path);

    /**
     * @brief Remove a folder from organize list
     * @param folder_path Folder path
     */
    void organize_remove_folder(const std::string& folder_path);

    /**
     * @brief Get organize folders
     */
    const std::vector<std::string>& get_organize_folders() const;

    /**
     * @brief Set date granularity for organizing
     * @param granularity "year", "month", or "day"
     */
    void organize_set_granularity(const std::string& granularity);

    /**
     * @brief Get date granularity
     */
    const std::string& get_organize_granularity() const;

    /**
     * @brief Set organize target folder
     * @param folder Target folder path
     */
    void organize_set_target_folder(const std::string& folder);

    /**
     * @brief Get organize target folder
     */
    const std::string& get_organize_target_folder() const;

    /**
     * @brief Start organizing files by date
     */
    void organize_start();

    /**
     * @brief Cancel ongoing organize operation
     */
    void organize_cancel();

    /**
     * @brief Undo last organize operation
     */
    bool organize_undo();

    /**
     * @brief Check if organize undo is available
     */
    bool organize_can_undo() const;

    /**
     * @brief Get organize progress (0.0 - 1.0)
     */
    double get_organize_progress() const;

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
    std::string scan_status_{"Scanning Photos..."};
    std::atomic<bool> scanning_{false};
    std::atomic<bool> cancelled_{false};
    std::atomic<uint64_t> review_model_generation_{0};

    // Organize state
    std::vector<std::string> organize_folders_;
    std::string organize_granularity_{"month"};
    std::string organize_target_folder_;
    std::atomic<bool> organizing_{false};
    std::atomic<bool> organize_cancelled_{false};
    std::atomic<int> organize_files_processed_{0};
    std::atomic<int> organize_total_files_{0};
    std::string organize_current_file_;
    std::thread organize_thread_;

    // Slint window handle
    std::optional<slint::ComponentHandle<CuraMainWindow>> window_;

    // Background scan thread
    std::thread scan_thread_;
    std::mutex state_mutex_;
    std::shared_ptr<slint::VectorModel<DuplicateGroupData>> duplicate_groups_model_;

    // Internal helpers
    void update_folder_list();
    void update_folder_list_ui();
    void update_ui_state();
    void run_scan_thread(bool enable_visual, uint64_t similarity_threshold,
                         const std::vector<std::string>& folders);
    void run_organize_thread(const std::vector<std::string>& folders,
                              const std::string& target_dir,
                              DateGranularity granularity);
    void bind_callbacks();
    std::shared_ptr<slint::VectorModel<DuplicateGroupData>> build_duplicate_groups_model() const;
    void populate_review_thumbnails_async(uint64_t generation);
    std::string format_size(uint64_t bytes) const;
};

} // namespace cura
