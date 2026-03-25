use md5::Md5;
use sha2::{Sha256, Digest};
use std::path::Path;

/// Holds both MD5 and SHA-256 hashes for a file
#[derive(Debug, Clone)]
pub struct FileHashes {
    pub md5: String,
    pub sha256: String,
}

/// Compute MD5 and SHA-256 hashes for a file
pub fn compute_hashes(path: &Path) -> Result<FileHashes, std::io::Error> {
    let content = std::fs::read(path)?;

    let md5 = format!("{:x}", Md5::digest(&content));
    let sha256 = format!("{:x}", Sha256::digest(&content));

    Ok(FileHashes { md5, sha256 })
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;
    use tempfile::NamedTempFile;

    #[test]
    fn test_compute_hashes() {
        let mut temp_file = NamedTempFile::new().unwrap();
        temp_file.write_all(b"test content for hashing").unwrap();
        temp_file.flush().unwrap();

        let hashes = compute_hashes(temp_file.path()).unwrap();

        assert!(!hashes.md5.is_empty());
        assert!(!hashes.sha256.is_empty());
        assert_eq!(hashes.md5.len(), 32); // MD5 is 32 hex chars
        assert_eq!(hashes.sha256.len(), 64); // SHA-256 is 64 hex chars
    }
}