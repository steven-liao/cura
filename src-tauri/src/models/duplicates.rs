use serde::{Deserialize, Serialize};
use crate::models::CuraMediaFile;

/// Represents a group of duplicate files
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DuplicateGroup {
    /// Unique identifier for this group
    pub id: String,
    /// Type of duplicate detection
    pub duplicate_type: DuplicateType,
    /// The hash value that groups these files (if exact match)
    pub hash: Option<String>,
    /// Files in this duplicate group
    pub files: Vec<CuraMediaFile>,
    /// Total size of all duplicates (wasted space)
    pub wasted_space: u64,
    /// Number of files that can be safely deleted (total - 1)
    pub removable_count: usize,
}

/// Type of duplicate detection
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum DuplicateType {
    /// Exact duplicates by content hash
    Exact,
    /// Similar images by perceptual hash
    Similar,
    /// Files with same name but different content
    NameMatch,
}

/// Result from duplicate detection
#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DuplicateResult {
    /// Groups of exact duplicates
    pub exact_duplicates: Vec<DuplicateGroup>,
    /// Groups of similar images
    pub similar_images: Vec<DuplicateGroup>,
    /// Groups with same name
    pub name_matches: Vec<DuplicateGroup>,
    /// Total wasted space in bytes
    pub total_wasted_space: u64,
    /// Total number of duplicate groups
    pub total_groups: usize,
}

impl DuplicateResult {
    pub fn new() -> Self {
        Self {
            exact_duplicates: Vec::new(),
            similar_images: Vec::new(),
            name_matches: Vec::new(),
            total_wasted_space: 0,
            total_groups: 0,
        }
    }
}
