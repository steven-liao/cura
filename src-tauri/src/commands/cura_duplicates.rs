use std::collections::HashMap;
use tauri::AppHandle;
use crate::models::{CuraMediaFile, DuplicateGroup, DuplicateType, DuplicateResult};

/// Find duplicates in a list of media files
#[tauri::command]
pub fn find_duplicates(
    _app: AppHandle,
    files: Vec<CuraMediaFile>,
    similarity_threshold: Option<u32>,
) -> Result<DuplicateResult, String> {
    eprintln!("[find_duplicates] Received {} files", files.len());

    // Debug: Log how many files have hashes
    let with_sha256 = files.iter().filter(|f| f.sha256_hash.is_some()).count();
    let with_md5 = files.iter().filter(|f| f.md5_hash.is_some()).count();
    eprintln!("[find_duplicates] Files with SHA256: {}, with MD5: {}", with_sha256, with_md5);

    // Debug: Log first few files
    for (i, file) in files.iter().take(3).enumerate() {
        eprintln!("[find_duplicates] File {}: {} - sha256: {:?}",
            i, file.file_name, file.sha256_hash.as_ref().map(|h| &h[..16.min(h.len())]));
    }

    if files.is_empty() {
        return Ok(DuplicateResult::new());
    }

    let mut result = DuplicateResult::new();
    let threshold = similarity_threshold.unwrap_or(10); // Default Hamming distance threshold

    // Group 1: Exact duplicates by SHA-256 hash
    let mut exact_hash_groups: HashMap<String, Vec<CuraMediaFile>> = HashMap::new();
    for file in &files {
        if let Some(hash) = &file.sha256_hash {
            exact_hash_groups.entry(hash.clone())
                .or_default()
                .push(file.clone());
        }
    }

    for (hash, group_files) in exact_hash_groups {
        if group_files.len() > 1 {
            let wasted_space = calculate_wasted_space(&group_files);
            result.exact_duplicates.push(DuplicateGroup {
                id: uuid::Uuid::new_v4().to_string(),
                duplicate_type: DuplicateType::Exact,
                hash: Some(hash),
                files: group_files.clone(),
                wasted_space,
                removable_count: group_files.len() - 1,
            });
            result.total_wasted_space += wasted_space;
        }
    }

    // Group 2: Same name, different content
    let mut name_groups: HashMap<String, Vec<CuraMediaFile>> = HashMap::new();
    for file in &files {
        let key = file.file_name.to_lowercase();
        name_groups.entry(key)
            .or_default()
            .push(file.clone());
    }

    for (name, group_files) in name_groups {
        if group_files.len() > 1 {
            // Only include if they're NOT exact duplicates (different hashes)
            let unique_hashes: std::collections::HashSet<_> = group_files.iter()
                .filter_map(|f| f.sha256_hash.clone())
                .collect();

            if unique_hashes.len() > 1 {
                let wasted_space = calculate_wasted_space(&group_files);
                result.name_matches.push(DuplicateGroup {
                    id: uuid::Uuid::new_v4().to_string(),
                    duplicate_type: DuplicateType::NameMatch,
                    hash: None,
                    files: group_files,
                    wasted_space,
                    removable_count: 0, // User decides which to keep
                });
            }
        }
    }

    // Group 3: Similar images by perceptual hash
    let images_with_phash: Vec<_> = files.iter()
        .filter(|f| f.perceptual_hash.is_some() && f.is_image())
        .cloned()
        .collect();

    if images_with_phash.len() > 1 {
        let mut similar_groups: Vec<Vec<CuraMediaFile>> = Vec::new();
        let mut used = std::collections::HashSet::new();

        for (i, file1) in images_with_phash.iter().enumerate() {
            if used.contains(&i) {
                continue;
            }

            let mut group = vec![file1.clone()];
            used.insert(i);

            for (j, file2) in images_with_phash.iter().enumerate().skip(i + 1) {
                if used.contains(&j) {
                    continue;
                }

                if let (Some(h1), Some(h2)) = (file1.perceptual_hash, file2.perceptual_hash) {
                    let distance = hamming_distance(h1, h2);
                    if distance <= threshold {
                        group.push(file2.clone());
                        used.insert(j);
                    }
                }
            }

            if group.len() > 1 {
                similar_groups.push(group);
            }
        }

        for group in similar_groups {
            let wasted_space = calculate_wasted_space(&group);
            result.similar_images.push(DuplicateGroup {
                id: uuid::Uuid::new_v4().to_string(),
                duplicate_type: DuplicateType::Similar,
                hash: None,
                files: group.clone(),
                wasted_space,
                removable_count: group.len() - 1,
            });
            result.total_wasted_space += wasted_space;
        }
    }

    result.total_groups = result.exact_duplicates.len()
        + result.similar_images.len()
        + result.name_matches.len();

    Ok(result)
}

/// Calculate Hamming distance between two 64-bit hashes
fn hamming_distance(hash1: u64, hash2: u64) -> u32 {
    (hash1 ^ hash2).count_ones()
}

/// Calculate wasted space in a duplicate group (sum of all files except the largest)
fn calculate_wasted_space(files: &[CuraMediaFile]) -> u64 {
    if files.len() <= 1 {
        return 0;
    }

    let total_size: u64 = files.iter().map(|f| f.file_size).sum();
    let largest_size = files.iter().map(|f| f.file_size).max().unwrap_or(0);

    // Wasted space is total minus the largest file (which you'd keep)
    total_size - largest_size
}

/// Delete a file from disk (marks it for deletion in the UI)
#[tauri::command]
pub async fn delete_file(path: String) -> Result<(), String> {
    tokio::fs::remove_file(&path)
        .await
        .map_err(|e| format!("Failed to delete file: {}", e))
}

/// Move a file to a different location
#[tauri::command]
pub async fn move_file(source: String, destination: String) -> Result<(), String> {
    tokio::fs::rename(&source, &destination)
        .await
        .map_err(|e| format!("Failed to move file: {}", e))
}
