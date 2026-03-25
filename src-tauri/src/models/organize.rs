use serde::{Deserialize, Serialize};
use chrono::{DateTime, Utc};

/// Represents an organization task
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct OrganizeTask {
    pub id: String,
    pub strategy: OrganizeStrategy,
    pub source_paths: Vec<String>,
    pub destination_path: String,
    pub status: OrganizeStatus,
    pub files_processed: usize,
    pub files_total: usize,
    pub created_at: DateTime<Utc>,
    pub completed_at: Option<DateTime<Utc>>,
}

/// Strategy for organizing photos
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", tag = "type")]
pub enum OrganizeStrategy {
    /// Organize by date taken (year/month/day folder structure)
    ByDate { format: DateFolderFormat },
    /// Organize by camera model
    ByCamera,
    /// Organize by file type
    ByType,
    /// Organize by location (if GPS data available)
    ByLocation { granularity: LocationGranularity },
    /// Custom folder naming pattern
    Custom { pattern: String },
}

/// Date folder format options
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum DateFolderFormat {
    YearMonthDay,  // 2024/03/24
    YearMonth,     // 2024/03
    YearOnly,      // 2024
    FlatDate,      // 2024-03-24
}

/// Location granularity for organization
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum LocationGranularity {
    Country,
    City,
    Exact, // GPS coordinates
}

/// Status of an organize task
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum OrganizeStatus {
    Pending,
    Running,
    Completed,
    Failed(String),
    Cancelled,
}

/// Preview of what organizing will do
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct OrganizePreview {
    pub operations: Vec<FileOperation>,
    pub total_files: usize,
    pub would_overwrite: Vec<String>,
}

/// A single file operation
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct FileOperation {
    pub source: String,
    pub destination: String,
    pub operation_type: OperationType,
}

/// Type of file operation
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum OperationType {
    Move,
    Copy,
}

/// Result of organizing
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct OrganizeResult {
    pub task_id: String,
    pub files_moved: usize,
    pub files_copied: usize,
    pub errors: Vec<String>,
    pub destination_path: String,
}

impl OrganizeTask {
    pub fn new(
        strategy: OrganizeStrategy,
        source_paths: Vec<String>,
        destination_path: String,
    ) -> Self {
        Self {
            id: uuid::Uuid::new_v4().to_string(),
            strategy,
            source_paths,
            destination_path,
            status: OrganizeStatus::Pending,
            files_processed: 0,
            files_total: 0,
            created_at: Utc::now(),
            completed_at: None,
        }
    }
}
