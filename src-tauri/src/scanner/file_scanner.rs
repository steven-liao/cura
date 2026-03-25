use walkdir::WalkDir;
use std::path::Path;

const IMAGE_EXTENSIONS: &[&str] = &[
    "jpg", "jpeg", "png", "gif", "bmp", "webp",
    "heic", "heif", "tiff", "tif", "raw", "cr2", "nef", "arw", "dng"
];

const VIDEO_EXTENSIONS: &[&str] = &[
    "mp4", "mov", "avi", "mkv", "wmv", "flv", "webm", "m4v", "3gp"
];

/// Check if a file extension indicates a media file (image or video)
pub fn is_media_file(path: &Path) -> bool {
    let ext = path.extension()
        .and_then(|e| e.to_str())
        .map(|e| e.to_lowercase());

    match ext {
        Some(e) => IMAGE_EXTENSIONS.contains(&e.as_str()) || VIDEO_EXTENSIONS.contains(&e.as_str()),
        None => false,
    }
}

/// Recursively scan folders for media files
pub fn scan_folders_for_media(folders: &[String]) -> Vec<std::path::PathBuf> {
    let mut files = Vec::new();

    for folder in folders {
        for entry in WalkDir::new(folder)
            .follow_links(true)
            .into_iter()
            .filter_map(|e| e.ok())
        {
            let path = entry.path();
            if path.is_file() && is_media_file(path) {
                files.push(path.to_path_buf());
            }
        }
    }

    files
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_is_media_file_image() {
        assert!(is_media_file(Path::new("test.jpg")));
        assert!(is_media_file(Path::new("test.JPG")));
        assert!(is_media_file(Path::new("test.png")));
        assert!(is_media_file(Path::new("test.heic")));
    }

    #[test]
    fn test_is_media_file_video() {
        assert!(is_media_file(Path::new("test.mp4")));
        assert!(is_media_file(Path::new("test.MOV")));
        assert!(is_media_file(Path::new("test.avi")));
    }

    #[test]
    fn test_is_media_file_non_media() {
        assert!(!is_media_file(Path::new("test.txt")));
        assert!(!is_media_file(Path::new("test.pdf")));
        assert!(!is_media_file(Path::new("test")));
    }

    /// Integration test: Scan D:/Images folder and report results
    #[test]
    fn test_scan_d_images_folder() {
        let folder = "D:/Images".to_string();
        let files = scan_folders_for_media(&[folder]);

        println!("\n=== Scan Test Results for D:/Images ===");
        println!("Total media files found: {}", files.len());

        // Categorize files
        let mut images = 0;
        let mut videos = 0;
        let mut total_size = 0u64;

        for path in &files {
            let ext = path.extension()
                .and_then(|e| e.to_str())
                .map(|e| e.to_lowercase())
                .unwrap_or_default();

            if IMAGE_EXTENSIONS.contains(&ext.as_str()) {
                images += 1;
            } else if VIDEO_EXTENSIONS.contains(&ext.as_str()) {
                videos += 1;
            }

            if let Ok(metadata) = std::fs::metadata(path) {
                total_size += metadata.len();
            }

            // Print first 10 files for inspection
            if files.len() <= 10 || files.iter().position(|p| p == path).unwrap() < 10 {
                let size = std::fs::metadata(path)
                    .map(|m| format!("{} bytes", m.len()))
                    .unwrap_or_else(|_| "unknown".to_string());
                println!("  - {} ({})", path.display(), size);
            }
        }

        if files.len() > 10 {
            println!("  ... and {} more files", files.len() - 10);
        }

        println!("\nSummary:");
        println!("  Images: {}", images);
        println!("  Videos: {}", videos);
        println!("  Total size: {:.2} MB", total_size as f64 / 1_048_576.0);
        println!("=======================================\n");

        // Test passes if we successfully scanned (even if folder is empty)
        // This is an integration test, not a unit test with fixed expectations
    }
}