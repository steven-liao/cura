/**
 * @file cura_app.cpp
 * @brief Slint UI application implementation
 */

#include "cura_app.hpp"
#include "../core/cura_image.hpp"
#include "../threading/cura_threadpool.hpp"
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <slint.h>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <comdef.h>
#undef DELETE  // Windows defines DELETE macro, conflicts with DuplicateAction::DELETE
#endif

namespace cura {

// Helper to convert filesystem path to UTF-8 string
static std::string path_to_utf8(const std::filesystem::path& p) {
    // Use u8string() for proper UTF-8 encoding, then convert to std::string
    auto u8str = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8str.data()), u8str.size());
}

#ifdef _WIN32
// Open a native Windows folder picker dialog
static std::string open_folder_dialog() {
    std::string result;

    // Initialize COM - don't uninitialize afterwards to avoid interfering with Slint
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool initialized = SUCCEEDED(hr);
    // RPC_E_CHANGED_MODE means COM was already initialized in a different mode
    // In that case, we shouldn't uninitialize it either

    IFileOpenDialog* pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) {
        // Set the options to pick folders
        DWORD dwOptions;
        hr = pFileOpen->GetOptions(&dwOptions);
        if (SUCCEEDED(hr)) {
            hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }

        // Show the dialog
        hr = pFileOpen->Show(nullptr);

        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileOpen->GetResult(&pItem);

            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr)) {
                    // Convert wide string to UTF-8
                    int width = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1,
                                                     nullptr, 0, nullptr, nullptr);
                    if (width > 0) {
                        result.resize(width - 1);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1,
                                           &result[0], width, nullptr, nullptr);
                    }
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }

    // Don't call CoUninitialize - let Slint handle COM state
    // Calling it here can interfere with Slint's event handling

    return result;
}
#else
// Placeholder for other platforms
static std::string open_folder_dialog() {
    return "";
}
#endif

CuraApp::CuraApp()
    : visual_similarity_enabled_(false)
    , similarity_threshold_(10)
    , scan_progress_(0.0) {
    // Create the main window
    window_.emplace(CuraMainWindow::create());

    // Bind callbacks to UI
    bind_callbacks();
    (*window_)->set_can_undo(false);
}

CuraApp::~CuraApp() {
    cancelled_ = true;
    scanner_.cancel();
    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }
}

void CuraApp::run() {
    // Run the Slint event loop
    (*window_)->run();
}

void CuraApp::bind_callbacks() {
    auto win = *window_;

    // Bind add-folder callback - opens native folder picker
    win->on_add_folder([this]() {
        std::string folder = open_folder_dialog();
        if (!folder.empty()) {
            add_folder(folder);
            update_folder_list_ui();
        }
    });

    // Bind remove-folder callback
    win->on_remove_folder([this](slint::SharedString folder) {
        remove_folder(std::string(folder));
        update_folder_list_ui();
    });

    // Bind start-scan callback
    win->on_start_scan([this]() {
        if (!selected_folders_.empty() && !scanning_) {
            start_scan(visual_similarity_enabled_, similarity_threshold_);
        }
    });

    // Bind cancel-scan callback
    win->on_cancel_scan([this]() {
        cancel_scan();
    });

    // Bind handle-duplicates callback
    win->on_handle_duplicates([this](std::shared_ptr<slint::Model<int>> ids,
                                      slint::SharedString action,
                                      slint::SharedString target) {
        // Get the group index from the model
        if (!ids || ids->row_count() == 0) return;

        auto data = ids->row_data(0);
        if (!data.has_value()) return;

        int group_index = data.value();
        if (group_index < 0 || group_index >= static_cast<int>(duplicate_groups_.size())) {
            return;
        }

        const auto& group = duplicate_groups_[group_index];
        std::string action_str = std::string(action);

        std::vector<std::string> files_to_handle;
        for (const auto& file : group.files) {
            if (file != group.best_file) {
                files_to_handle.push_back(file);
            }
        }

        if (files_to_handle.empty()) return;

        DuplicateAction op_action = (action_str == "delete") ? DuplicateAction::DELETE : DuplicateAction::MOVE_TO_DIR;

        std::string target_dir;
        if (action_str == "move") {
            // If no target folder set, open folder picker
            if (move_target_folder_.empty()) {
                target_dir = open_folder_dialog();
                if (target_dir.empty()) {
                    // User cancelled the folder picker
                    return;
                }
                move_target_folder_ = target_dir;
            } else {
                target_dir = move_target_folder_;
            }
        }

        bool success = handle_duplicates(files_to_handle, op_action, target_dir);

        if (success) {
            // Remove the handled group
            duplicate_groups_.erase(duplicate_groups_.begin() + group_index);

            // Recalculate total save size
            uint64_t total_save = 0;
            for (const auto& g : duplicate_groups_) {
                total_save += g.save_size;
            }

            // Update UI
            auto win = *window_;
            int group_count = static_cast<int>(duplicate_groups_.size());
            std::string save_str = format_size(total_save);

            win->set_total_groups(group_count);
            win->set_total_save(slint::SharedString(save_str));
            win->set_can_undo(can_undo());

            // Update duplicate groups model
            auto model = std::make_shared<slint::VectorModel<DuplicateGroupData>>();
            for (const auto& g : duplicate_groups_) {
                DuplicateGroupData data;
                data.best_file = slint::SharedString(path_to_utf8(std::filesystem::path(g.best_file)));
                data.save_size = slint::SharedString(format_size(g.save_size));
                data.is_visual = g.is_visual_duplicate;
                data.similarity_score = static_cast<int>(g.similarity_score);

                auto files_model = std::make_shared<slint::VectorModel<slint::SharedString>>();
                for (const auto& file : g.files) {
                    files_model->push_back(slint::SharedString(path_to_utf8(std::filesystem::path(file))));
                }
                data.files = files_model;

                // Create display-files model (max 3 for card display)
                auto display_model = std::make_shared<slint::VectorModel<slint::SharedString>>();
                const size_t max_display = 3;
                for (size_t i = 0; i < g.files.size() && i < max_display; ++i) {
                    display_model->push_back(slint::SharedString(path_to_utf8(std::filesystem::path(g.files[i]))));
                }
                data.display_files = display_model;

                model->push_back(data);
            }
            win->set_duplicate_groups(model);
        }
    });

    // Bind undo callback
    win->on_undo_last([this]() {
        undo_last_operation();
    });
}

void CuraApp::add_folder(const std::string& folder_path) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (std::filesystem::exists(folder_path) &&
        std::filesystem::is_directory(folder_path)) {
        if (std::find(selected_folders_.begin(), selected_folders_.end(),
                      folder_path) == selected_folders_.end()) {
            selected_folders_.push_back(folder_path);
        }
    }
}

void CuraApp::remove_folder(const std::string& folder_path) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    auto it = std::find(selected_folders_.begin(), selected_folders_.end(),
                        folder_path);
    if (it != selected_folders_.end()) {
        selected_folders_.erase(it);
    }
}

void CuraApp::update_folder_list() {
    // Update folder list in UI
}

void CuraApp::update_folder_list_ui() {
    auto win = *window_;

    // Create a model for the folder list
    auto model = std::make_shared<slint::VectorModel<slint::SharedString>>();

    for (const auto& folder : selected_folders_) {
        model->push_back(slint::SharedString(folder));
    }

    win->set_selected_folders(model);
}

const std::vector<std::string>& CuraApp::get_folders() const {
    return selected_folders_;
}

void CuraApp::start_scan(bool enable_visual, uint64_t similarity_threshold) {
    if (scanning_) return;
    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }

    scanning_ = true;
    cancelled_ = false;
    visual_similarity_enabled_ = enable_visual;
    similarity_threshold_ = similarity_threshold;

    // Copy folders to scan (thread-safe)
    std::vector<std::string> folders_to_scan;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        folders_to_scan = selected_folders_;
    }

    if (folders_to_scan.empty()) {
        scanning_ = false;
        return;
    }

    // Switch to progress view
    auto win = *window_;
    win->set_current_view(slint::SharedString("progress"));
    win->set_scan_progress(0.0);
    win->set_files_scanned(0);
    win->set_total_files(0);

    // Run scan in background thread
    scan_thread_ = std::thread([this, enable_visual, similarity_threshold, folders_to_scan]() {
        run_scan_thread(enable_visual, similarity_threshold, folders_to_scan);
    });
}

void CuraApp::run_scan_thread(bool enable_visual, uint64_t similarity_threshold,
                               const std::vector<std::string>& folders) {
    try {
        // Phase 1: Scan directories
        auto images = scanner_.scan(folders,
            [this](size_t current, size_t total, const std::string& file) {
                if (cancelled_) return;

                scan_progress_ = (total > 0) ? static_cast<double>(current) / total : 0.0;

                double progress = scan_progress_ * 0.3;
                slint::invoke_from_event_loop([this, progress, current, total]() {
                    if (window_.has_value()) {
                        (*window_)->set_scan_progress(progress);
                        (*window_)->set_files_scanned(static_cast<int>(current));
                        (*window_)->set_total_files(static_cast<int>(total));
                    }
                });
            });

        std::cout << "=== SCANNING ===" << std::endl;
        std::cout << "Folders scanned:" << std::endl;
        for (const auto& f : folders) {
            std::cout << "  " << f << std::endl;
        }
        std::cout << "Images found: " << images.size() << std::endl;
        for (const auto& img : images) {
            std::cout << "  " << img.path.string() << " (" << img.file_size << " bytes)" << std::endl;
        }

        if (cancelled_) {
            slint::invoke_from_event_loop([this]() {
                if (window_.has_value()) {
                    (*window_)->set_current_view(slint::SharedString("setup"));
                }
                scanning_ = false;
            });
            return;
        }

        // Check if any images were found
        if (images.empty()) {
            slint::invoke_from_event_loop([this]() {
                if (window_.has_value()) {
                    (*window_)->set_current_view(slint::SharedString("review"));
                    (*window_)->set_scan_progress(1.0);
                    (*window_)->set_total_groups(0);
                    (*window_)->set_total_save(slint::SharedString("0 B"));
                    (*window_)->set_can_undo(can_undo());
                }
                scanning_ = false;
            });
            return;
        }

        // Phase 2: Compute hashes
        std::vector<std::string> files;
        files.reserve(images.size());
        for (const auto& img : images) {
            files.push_back(path_to_utf8(img.path));
        }

        size_t total_files = files.size();
        auto hashes = hasher_.compute_hashes(files, enable_visual,
            [this, total_files](size_t current, size_t total, const std::string& file) {
                if (cancelled_) return;

                scan_progress_ = 0.3 + (total > 0 ? static_cast<double>(current) / total : 0.0) * 0.6;

                double progress = scan_progress_;
                slint::invoke_from_event_loop([this, progress]() {
                    if (window_.has_value()) {
                        (*window_)->set_scan_progress(progress);
                    }
                });
            });

        if (cancelled_) {
            slint::invoke_from_event_loop([this]() {
                if (window_.has_value()) {
                    (*window_)->set_current_view(slint::SharedString("setup"));
                }
                scanning_ = false;
            });
            return;
        }

        // Phase 3: Cluster duplicates
        duplicate_groups_ = clusterer_.cluster(hashes, enable_visual, similarity_threshold);

        std::cout << "=== SCAN RESULTS ===" << std::endl;
        std::cout << "Total files scanned: " << images.size() << std::endl;
        std::cout << "Total hash results: " << hashes.size() << std::endl;
        std::cout << "Duplicate groups found: " << duplicate_groups_.size() << std::endl;

        // Count files with hash 0 (failed to hash)
        int zero_hash_count = 0;
        for (const auto& h : hashes) {
            if (h.exact_hash == 0) {
                zero_hash_count++;
                std::cout << "  Zero hash for: " << h.file_path << std::endl;
            }
        }
        std::cout << "Files with zero hash: " << zero_hash_count << std::endl;

        // Show hash distribution
        std::unordered_map<uint64_t, int> hash_counts;
        for (const auto& h : hashes) {
            hash_counts[h.exact_hash]++;
        }
        int unique_hashes = 0;
        int duplicate_hashes = 0;
        for (const auto& [hash, count] : hash_counts) {
            if (hash != 0 && count > 1) {
                duplicate_hashes++;
                std::cout << "  Hash " << std::hex << hash << std::dec << " appears " << count << " times" << std::endl;
            }
            if (hash != 0) unique_hashes++;
        }
        std::cout << "Unique hashes: " << unique_hashes << std::endl;
        std::cout << "Hashes with duplicates: " << duplicate_hashes << std::endl;

        std::cout << "====================" << std::endl;

        // Calculate total save size
        uint64_t total_save = 0;
        for (const auto& group : duplicate_groups_) {
            total_save += group.save_size;
        }

        // Switch to review view
        int group_count = static_cast<int>(duplicate_groups_.size());
        std::string save_str = format_size(total_save);

        slint::invoke_from_event_loop([this, group_count, save_str]() {
            if (window_.has_value()) {
                (*window_)->set_current_view(slint::SharedString("review"));
                (*window_)->set_scan_progress(1.0);
                (*window_)->set_total_groups(group_count);
                (*window_)->set_total_save(slint::SharedString(save_str));
                (*window_)->set_can_undo(can_undo());

                // Populate duplicate groups model
                auto model = std::make_shared<slint::VectorModel<DuplicateGroupData>>();
                CuraImageProcessor img_proc;
                for (const auto& group : duplicate_groups_) {
                    DuplicateGroupData data;
                    data.best_file = slint::SharedString(path_to_utf8(std::filesystem::path(group.best_file)));
                    data.save_size = slint::SharedString(format_size(group.save_size));
                    data.is_visual = group.is_visual_duplicate;
                    data.similarity_score = static_cast<int>(group.similarity_score);

                    // Create files model (all files for popup)
                    auto files_model = std::make_shared<slint::VectorModel<slint::SharedString>>();
                    for (const auto& file : group.files) {
                        files_model->push_back(slint::SharedString(path_to_utf8(std::filesystem::path(file))));
                    }
                    data.files = files_model;

                    // Create display-files model (max 3 for card display)
                    auto display_model = std::make_shared<slint::VectorModel<slint::SharedString>>();
                    const size_t max_display = 3;
                    for (size_t i = 0; i < group.files.size() && i < max_display; ++i) {
                        display_model->push_back(slint::SharedString(path_to_utf8(std::filesystem::path(group.files[i]))));
                    }
                    data.display_files = display_model;

                    // Generate thumbnail
                    ImageData thumb = img_proc.generate_thumbnail(group.best_file, 64);
                    if (thumb.valid && thumb.channels >= 3) {
                        // Convert to Slint image
                        if (thumb.channels == 3) {
                            slint::SharedPixelBuffer<slint::Rgb8Pixel> buffer(
                                static_cast<uint32_t>(thumb.width),
                                static_cast<uint32_t>(thumb.height),
                                reinterpret_cast<slint::Rgb8Pixel*>(thumb.pixels.data())
                            );
                            data.thumbnail = slint::Image(buffer);
                        } else if (thumb.channels == 4) {
                            slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(
                                static_cast<uint32_t>(thumb.width),
                                static_cast<uint32_t>(thumb.height),
                                reinterpret_cast<slint::Rgba8Pixel*>(thumb.pixels.data())
                            );
                            data.thumbnail = slint::Image(buffer);
                        }
                    }

                    model->push_back(data);
                }
                (*window_)->set_duplicate_groups(model);
            }
            scanning_ = false;
        });

    } catch (const std::exception& e) {
        // On error, go back to setup
        slint::invoke_from_event_loop([this]() {
            if (window_.has_value()) {
                (*window_)->set_current_view(slint::SharedString("setup"));
            }
            scanning_ = false;
        });
    }
}

void CuraApp::cancel_scan() {
    cancelled_ = true;
    scanner_.cancel();

    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }

    if (window_.has_value()) {
        (*window_)->set_current_view(slint::SharedString("setup"));
    }
    scanning_ = false;
}

double CuraApp::get_scan_progress() const {
    return scan_progress_;
}

const std::vector<DuplicateGroup>& CuraApp::get_duplicate_groups() const {
    return duplicate_groups_;
}

bool CuraApp::handle_duplicates(
    const std::vector<std::string>& file_paths,
    DuplicateAction action,
    const std::string& target_dir
) {
    auto result = file_ops_.execute(file_paths, action, target_dir);
    if (window_.has_value()) {
        (*window_)->set_can_undo(can_undo());
    }
    return result.success;
}

bool CuraApp::undo_last_operation() {
    bool success = file_ops_.undo_last_operation();
    if (window_.has_value()) {
        (*window_)->set_can_undo(can_undo());
    }
    return success;
}

bool CuraApp::can_undo() const {
    return file_ops_.can_undo();
}

void CuraApp::set_visual_similarity_enabled(bool enabled) {
    visual_similarity_enabled_ = enabled;
}

bool CuraApp::is_visual_similarity_enabled() const {
    return visual_similarity_enabled_;
}

void CuraApp::set_move_target_folder(const std::string& folder) {
    move_target_folder_ = folder;
}

const std::string& CuraApp::get_move_target_folder() const {
    return move_target_folder_;
}

void CuraApp::update_ui_state() {
    // Update UI from current state
}

std::string CuraApp::format_size(uint64_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return oss.str();
}

} // namespace cura
