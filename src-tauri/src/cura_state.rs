use std::collections::HashMap;
use tokio::sync::RwLock;
use tokio_util::sync::CancellationToken;
use uuid::Uuid;

/// Application state for Cura
pub struct CuraState {
    /// Active scan operations
    pub active_scans: RwLock<HashMap<Uuid, ScanHandle>>,
    /// Cancellation tokens for active scans
    pub cancel_tokens: RwLock<HashMap<Uuid, CancellationToken>>,
}

/// Handle for an active scan operation
#[derive(Debug, Clone)]
pub struct ScanHandle {
    pub id: Uuid,
    pub source_paths: Vec<String>,
    pub files_found: usize,
    pub status: ScanStatus,
}

/// Status of a scan operation
#[derive(Debug, Clone, serde::Serialize)]
pub enum ScanStatus {
    Pending,
    Scanning,
    Hashing,
    MetadataExtraction,
    Complete,
    Cancelled,
    Error(String),
}

impl Default for CuraState {
    fn default() -> Self {
        Self::new()
    }
}

impl CuraState {
    /// Create a new CuraState instance
    pub fn new() -> Self {
        Self {
            active_scans: RwLock::new(HashMap::new()),
            cancel_tokens: RwLock::new(HashMap::new()),
        }
    }

    /// Create a cancellation token for a scan
    pub async fn create_cancel_token(&self, scan_id: Uuid) -> CancellationToken {
        let token = CancellationToken::new();
        self.cancel_tokens.write().await.insert(scan_id, token.clone());
        token
    }

    /// Remove a cancellation token (cleanup after scan completes)
    pub async fn remove_cancel_token(&self, scan_id: &Uuid) {
        self.cancel_tokens.write().await.remove(scan_id);
    }

    /// Cancel a scan by its ID
    pub async fn cancel_scan(&self, scan_id: &Uuid) -> bool {
        if let Some(token) = self.cancel_tokens.read().await.get(scan_id) {
            token.cancel();
            true
        } else {
            false
        }
    }

    /// Check if a scan has been cancelled
    pub async fn is_cancelled(&self, scan_id: &Uuid) -> bool {
        self.cancel_tokens
            .read()
            .await
            .get(scan_id)
            .map(|t| t.is_cancelled())
            .unwrap_or(true)
    }
}