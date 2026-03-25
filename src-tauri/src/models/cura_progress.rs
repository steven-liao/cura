use serde::{Deserialize, Serialize};

/// Progress information for scanning operations
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CuraProgress {
    /// Current phase of the operation
    pub phase: CuraProgressPhase,
    /// Number of files processed so far
    pub current: usize,
    /// Total number of files to process
    pub total: usize,
    /// Current file being processed
    pub current_file: Option<String>,
    /// Processing speed (files per second)
    pub speed: Option<f64>,
    /// Estimated time remaining in seconds
    pub eta_seconds: Option<u64>,
}

/// Phase of a scan operation
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub enum CuraProgressPhase {
    /// Initializing the scan
    Initializing,
    /// Scanning directories for files
    Scanning,
    /// Computing file hashes
    Hashing,
    /// Extracting metadata
    MetadataExtraction,
    /// Detecting duplicates
    Deduplication,
    /// Organizing files
    Organizing,
    /// Operation complete
    Complete,
    /// Operation cancelled
    Cancelled,
    /// Error occurred
    Error(String),
}

impl Default for CuraProgress {
    fn default() -> Self {
        Self {
            phase: CuraProgressPhase::Initializing,
            current: 0,
            total: 0,
            current_file: None,
            speed: None,
            eta_seconds: None,
        }
    }
}