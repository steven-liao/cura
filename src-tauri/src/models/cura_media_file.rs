use serde::{Deserialize, Serialize};
use chrono::{DateTime, Utc};

/// Represents a media file (photo or video) in the Cura system
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CuraMediaFile {
    /// Unique identifier for this file
    pub id: String,
    /// Full path to the file
    pub path: String,
    /// File name without path
    pub file_name: String,
    /// File size in bytes
    pub file_size: u64,
    /// File extension (lowercase, without dot)
    pub extension: String,
    /// MD5 hash of the file content
    pub md5_hash: Option<String>,
    /// SHA-256 hash of the file content
    pub sha256_hash: Option<String>,
    /// Perceptual hash for similarity detection
    pub perceptual_hash: Option<u64>,
    /// Date the photo/video was taken (from EXIF or file metadata)
    pub date_taken: Option<DateTime<Utc>>,
    /// Width in pixels (for images/videos)
    pub width: Option<u32>,
    /// Height in pixels (for images/videos)
    pub height: Option<u32>,
    /// Camera model (from EXIF)
    pub camera_model: Option<String>,
    /// Whether this file is flagged as blurry
    pub is_blurry: bool,
    /// Whether this file is the "leader" of a burst group
    pub is_burst_leader: bool,
    /// When this file was indexed
    pub indexed_at: DateTime<Utc>,
}

impl CuraMediaFile {
    /// Create a new CuraMediaFile from a path
    pub fn from_path(path: &std::path::Path) -> Self {
        let file_name = path
            .file_name()
            .map(|n| n.to_string_lossy().to_string())
            .unwrap_or_default();

        let extension = path
            .extension()
            .map(|e| e.to_string_lossy().to_lowercase())
            .unwrap_or_default();

        let file_size = std::fs::metadata(path)
            .map(|m| m.len())
            .unwrap_or(0);

        Self {
            id: uuid::Uuid::new_v4().to_string(),
            path: path.to_string_lossy().to_string(),
            file_name,
            file_size,
            extension,
            md5_hash: None,
            sha256_hash: None,
            perceptual_hash: None,
            date_taken: None,
            width: None,
            height: None,
            camera_model: None,
            is_blurry: false,
            is_burst_leader: false,
            indexed_at: Utc::now(),
        }
    }

    /// Check if this is an image file
    pub fn is_image(&self) -> bool {
        matches!(
            self.extension.as_str(),
            "jpg" | "jpeg" | "png" | "gif" | "bmp" | "webp" | "heic" | "heif" | "tiff" | "tif"
        )
    }

    /// Check if this is a video file
    pub fn is_video(&self) -> bool {
        matches!(
            self.extension.as_str(),
            "mp4" | "mov" | "avi" | "mkv" | "wmv" | "flv" | "webm" | "m4v"
        )
    }
}