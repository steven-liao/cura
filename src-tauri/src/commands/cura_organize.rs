use std::collections::HashMap;
use std::path::{Path, PathBuf};
use chrono::{Datelike, Utc};
use tauri::AppHandle;
use crate::models::{CuraMediaFile, OrganizeStrategy, OrganizePreview, OrganizeResult, FileOperation, OperationType, DateFolderFormat};

/// Preview what organizing would do without executing
#[tauri::command]
pub async fn preview_organize(
    files: Vec<CuraMediaFile>,
    strategy: OrganizeStrategy,
    destination: String,
) -> Result<OrganizePreview, String> {
    eprintln!("[preview_organize] Received {} files, strategy: {:?}, dest: {}", files.len(), strategy, destination);

    if files.is_empty() {
        return Ok(OrganizePreview {
            operations: vec![],
            total_files: 0,
            would_overwrite: vec![],
        });
    }

    let mut operations = Vec::new();
    let mut would_overwrite = Vec::new();
    let mut dest_paths = HashMap::new();

    for (i, file) in files.iter().enumerate() {
        let dest_relative = compute_destination_path(file, &strategy);
        let dest_full = Path::new(&destination).join(&dest_relative);
        let dest_str = dest_full.to_string_lossy().to_string();

        eprintln!("[preview_organize] File {}: {} -> {}", i, file.file_name, dest_str);

        // Check for potential overwrites
        if dest_paths.contains_key(&dest_str) || std::fs::metadata(&dest_full).is_ok() {
            would_overwrite.push(file.file_name.clone());
        }

        dest_paths.insert(dest_str.clone(), file.id.clone());

        operations.push(FileOperation {
            source: file.path.clone(),
            destination: dest_str,
            operation_type: OperationType::Move,
        });
    }

    eprintln!("[preview_organize] Returning {} operations", operations.len());

    Ok(OrganizePreview {
        operations,
        total_files: files.len(),
        would_overwrite,
    })
}

/// Execute organization of files
#[tauri::command]
pub async fn execute_organize(
    _app: AppHandle,
    files: Vec<CuraMediaFile>,
    strategy: OrganizeStrategy,
    destination: String,
    copy_instead_of_move: bool,
) -> Result<OrganizeResult, String> {
    if files.is_empty() {
        return Ok(OrganizeResult {
            task_id: uuid::Uuid::new_v4().to_string(),
            files_moved: 0,
            files_copied: 0,
            errors: vec![],
            destination_path: destination,
        });
    }

    // Ensure destination exists
    tokio::fs::create_dir_all(&destination)
        .await
        .map_err(|e| format!("Failed to create destination directory: {}", e))?;

    let mut files_moved = 0;
    let mut files_copied = 0;
    let mut errors = Vec::new();

    for file in &files {
        let dest_relative = compute_destination_path(file, &strategy);
        let dest_full = Path::new(&destination).join(&dest_relative);

        // Create parent directory
        if let Some(parent) = dest_full.parent() {
            if let Err(e) = tokio::fs::create_dir_all(parent).await {
                errors.push(format!("Failed to create directory for {}: {}", file.file_name, e));
                continue;
            }
        }

        // Check if destination already exists
        if std::fs::metadata(&dest_full).is_ok() {
            errors.push(format!("Destination already exists for {} - skipping", file.file_name));
            continue;
        }

        // Perform move or copy
        if copy_instead_of_move {
            match tokio::fs::copy(&file.path, &dest_full).await {
                Ok(_) => { files_copied += 1; }
                Err(e) => {
                    errors.push(format!("Failed to copy {}: {}", file.file_name, e));
                }
            }
        } else {
            match tokio::fs::rename(&file.path, &dest_full).await {
                Ok(_) => { files_moved += 1; }
                Err(e) => {
                    errors.push(format!("Failed to move {}: {}", file.file_name, e));
                }
            }
        }
    }

    Ok(OrganizeResult {
        task_id: uuid::Uuid::new_v4().to_string(),
        files_moved,
        files_copied,
        errors,
        destination_path: destination,
    })
}

/// Select destination folder for organizing
#[tauri::command]
pub async fn select_organize_destination(
    app: tauri::AppHandle,
) -> Result<Option<String>, String> {
    use tauri_plugin_dialog::DialogExt;

    let folder = app.dialog()
        .file()
        .blocking_pick_folder();

    Ok(folder.map(|p| p.to_string()))
}

/// Compute the destination path for a file based on strategy
fn compute_destination_path(file: &CuraMediaFile, strategy: &OrganizeStrategy) -> PathBuf {
    let file_name = &file.file_name;

    match strategy {
        OrganizeStrategy::ByDate { format } => {
            let date = file.date_taken
                .map(|d| d.date_naive())
                .unwrap_or_else(|| Utc::now().date_naive());

            let folder_path = match format {
                DateFolderFormat::YearMonthDay => {
                    format!("{}/{:02}/{:02}", date.year(), date.month(), date.day())
                }
                DateFolderFormat::YearMonth => {
                    format!("{}/{:02}", date.year(), date.month())
                }
                DateFolderFormat::YearOnly => {
                    format!("{}", date.year())
                }
                DateFolderFormat::FlatDate => {
                    format!("{}-{:02}-{:02}", date.year(), date.month(), date.day())
                }
            };

            PathBuf::from(folder_path).join(file_name)
        }

        OrganizeStrategy::ByCamera => {
            let camera = file.camera_model
                .as_ref()
                .map(|c| sanitize_folder_name(c))
                .unwrap_or_else(|| "Unknown".to_string());

            PathBuf::from(camera).join(file_name)
        }

        OrganizeStrategy::ByType => {
            let folder = if file.is_image() {
                "Images"
            } else if file.is_video() {
                "Videos"
            } else {
                "Other"
            };

            PathBuf::from(folder).join(file_name)
        }

        OrganizeStrategy::ByLocation { granularity } => {
            // Placeholder - would need GPS data from EXIF
            let folder = match granularity {
                crate::models::LocationGranularity::Country => "ByCountry",
                crate::models::LocationGranularity::City => "ByCity",
                crate::models::LocationGranularity::Exact => "ByGPS",
            };
            PathBuf::from(folder).join(file_name)
        }

        OrganizeStrategy::Custom { pattern } => {
            // Simple pattern replacement - could be expanded
            let mut path = pattern.clone();
            if let Some(date) = file.date_taken {
                path = path.replace("{year}", &date.year().to_string());
                path = path.replace("{month}", &format!("{:02}", date.month()));
                path = path.replace("{day}", &format!("{:02}", date.day()));
            }
            if let Some(camera) = &file.camera_model {
                path = path.replace("{camera}", &sanitize_folder_name(camera));
            }
            path = path.replace("{type}", &file.extension.to_uppercase());

            PathBuf::from(path).join(file_name)
        }
    }
}

/// Sanitize a string for use as a folder name
fn sanitize_folder_name(name: &str) -> String {
    name.chars()
        .map(|c| match c {
            '/' | '\\' | ':' | '*' | '?' | '"' | '<' | '>' | '|' => '_',
            _ => c,
        })
        .collect::<String>()
        .trim()
        .to_string()
}
