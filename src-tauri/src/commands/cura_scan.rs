use serde::{Deserialize, Serialize};
use tauri::{AppHandle, Emitter, State};
use uuid::Uuid;

use crate::models::{CuraMediaFile, CuraProgress, CuraProgressPhase};
use crate::cura_state::CuraState;
use crate::scanner::{scan_folders_for_media, compute_hashes, extract_exif, compute_perceptual_hash};

/// Returns the application version
#[tauri::command]
pub fn get_version() -> String {
    env!("CARGO_PKG_VERSION").to_string()
}

/// Result from selecting folders
#[derive(Debug, Serialize, Deserialize)]
pub struct SelectFoldersResult {
    pub folders: Vec<String>,
    pub count: usize,
}

/// Opens a folder selection dialog and returns the selected paths
#[tauri::command]
pub async fn select_folders(
    app: tauri::AppHandle,
) -> Result<SelectFoldersResult, String> {
    use tauri_plugin_dialog::DialogExt;

    let folder = app.dialog()
        .file()
        .blocking_pick_folder();

    match folder {
        Some(path) => {
            let folder_path = path.to_string();
            Ok(SelectFoldersResult {
                count: 1,
                folders: vec![folder_path],
            })
        }
        None => Ok(SelectFoldersResult {
            folders: vec![],
            count: 0,
        }),
    }
}

/// Result from a scan operation
#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ScanResult {
    pub scan_id: String,
    pub files: Vec<CuraMediaFile>,
    pub total_files: usize,
}

/// Start scanning folders for media files
#[tauri::command]
pub async fn start_scan(
    app: AppHandle,
    state: State<'_, CuraState>,
    folders: Vec<String>,
) -> Result<ScanResult, String> {
    if folders.is_empty() {
        return Err("No folders provided".to_string());
    }

    let scan_id = Uuid::new_v4();
    let cancel_token = state.create_cancel_token(scan_id).await;

    // Emit progress: Initializing
    let _ = app.emit("scan-progress", CuraProgress {
        phase: CuraProgressPhase::Initializing,
        current: 0,
        total: 0,
        current_file: None,
        speed: None,
        eta_seconds: None,
    });

    // Run the blocking scan in a separate thread
    let app_clone = app.clone();
    let cancel_token_clone = cancel_token.clone();
    let folders_clone = folders.clone();

    let result = tokio::task::spawn_blocking(move || {
        // Phase 1: Scan directories
        let _ = app_clone.emit("scan-progress", CuraProgress {
            phase: CuraProgressPhase::Scanning,
            current: 0,
            total: folders_clone.len(),
            current_file: None,
            speed: None,
            eta_seconds: None,
        });

        let files = scan_folders_for_media(&folders_clone);
        let total_files = files.len();

        if total_files == 0 {
            let _ = app_clone.emit("scan-progress", CuraProgress {
                phase: CuraProgressPhase::Complete,
                current: 0,
                total: 0,
                current_file: Some("No media files found".to_string()),
                speed: None,
                eta_seconds: None,
            });
            return Ok(ScanResult {
                scan_id: scan_id.to_string(),
                files: vec![],
                total_files: 0,
            });
        }

        // Emit progress with total found
        let _ = app_clone.emit("scan-progress", CuraProgress {
            phase: CuraProgressPhase::Scanning,
            current: folders_clone.len(),
            total: total_files,
            current_file: Some(format!("Found {} files", total_files)),
            speed: None,
            eta_seconds: None,
        });

        // Phase 2: Process files (hashing + metadata)
        let mut media_files: Vec<CuraMediaFile> = Vec::with_capacity(total_files);
        let start_time = std::time::Instant::now();

        for (idx, path) in files.iter().enumerate() {
            // Check for cancellation
            if cancel_token_clone.is_cancelled() {
                let _ = app_clone.emit("scan-progress", CuraProgress {
                    phase: CuraProgressPhase::Cancelled,
                    current: idx,
                    total: total_files,
                    current_file: None,
                    speed: None,
                    eta_seconds: None,
                });
                return Err("Scan cancelled".to_string());
            }

            let mut media_file = CuraMediaFile::from_path(path);

            // Compute hashes (skip for very large files >50MB)
            if media_file.file_size < 50 * 1024 * 1024 {
                if let Ok(hashes) = compute_hashes(path) {
                    media_file.md5_hash = Some(hashes.md5);
                    media_file.sha256_hash = Some(hashes.sha256);
                }
            }

            // Extract EXIF for images (skip for large files >10MB)
            if media_file.is_image() && media_file.file_size < 10 * 1024 * 1024 {
                if let Some(exif) = extract_exif(path) {
                    media_file.date_taken = exif.date_taken;
                    media_file.width = exif.width;
                    media_file.height = exif.height;
                    media_file.camera_model = exif.camera_model;
                }
            }

            // TODO: Perceptual hash disabled for performance - re-enable with thumbnail generation

            media_files.push(media_file);

            // Emit progress every 10 files or at end
            if idx % 10 == 0 || idx == total_files - 1 {
                let elapsed = start_time.elapsed().as_secs_f64();
                let speed = if elapsed > 0.0 { (idx + 1) as f64 / elapsed } else { 0.0 };
                let eta = if speed > 0.0 {
                    Some(((total_files - idx - 1) as f64 / speed) as u64)
                } else {
                    None
                };

                let current_file = path.file_name()
                    .map(|n| n.to_string_lossy().to_string());

                let _ = app_clone.emit("scan-progress", CuraProgress {
                    phase: CuraProgressPhase::Hashing,
                    current: idx + 1,
                    total: total_files,
                    current_file,
                    speed: Some(speed),
                    eta_seconds: eta,
                });
            }
        }

        // Complete
        let _ = app_clone.emit("scan-progress", CuraProgress {
            phase: CuraProgressPhase::Complete,
            current: total_files,
            total: total_files,
            current_file: None,
            speed: None,
            eta_seconds: None,
        });

        Ok(ScanResult {
            scan_id: scan_id.to_string(),
            files: media_files,
            total_files,
        })
    }).await;

    // Cleanup cancel token
    state.remove_cancel_token(&scan_id).await;

    result.map_err(|e| e.to_string())?
}

/// Cancel an active scan
#[tauri::command]
pub async fn cancel_scan(
    state: State<'_, CuraState>,
    scan_id: String,
) -> Result<(), String> {
    let id = Uuid::parse_str(&scan_id).map_err(|e| e.to_string())?;

    if state.cancel_scan(&id).await {
        Ok(())
    } else {
        Err("Scan not found".to_string())
    }
}