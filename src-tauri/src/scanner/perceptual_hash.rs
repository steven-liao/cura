use image::imageops::FilterType;
use std::path::Path;

/// Compute a simple perceptual hash for an image
/// Uses average hash (aHash) algorithm:
/// 1. Resize image to 8x8
/// 2. Convert to grayscale
/// 3. Compute average pixel value
/// 4. Generate 64-bit hash where each bit is 1 if pixel > average
pub fn compute_perceptual_hash(path: &Path) -> Option<u64> {
    let img = image::open(path).ok()?;

    // Resize to 8x8 grayscale
    let resized = img.resize_exact(8, 8, FilterType::Lanczos3).to_luma8();

    // Compute average
    let avg: u32 = resized.pixels().map(|p| p[0] as u32).sum::<u32>() / 64;

    // Generate hash
    let mut hash: u64 = 0;
    for (i, pixel) in resized.pixels().enumerate() {
        if pixel[0] as u32 > avg {
            hash |= 1u64 << i;
        }
    }

    Some(hash)
}

/// Calculate Hamming distance between two perceptual hashes
/// Lower distance = more similar images
pub fn hamming_distance(hash1: u64, hash2: u64) -> u32 {
    (hash1 ^ hash2).count_ones()
}

/// Check if two images are similar based on perceptual hash
/// threshold of 10 or less typically indicates similar images
pub fn are_similar(hash1: u64, hash2: u64, threshold: u32) -> bool {
    hamming_distance(hash1, hash2) <= threshold
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_hamming_distance() {
        assert_eq!(hamming_distance(0b0000, 0b0000), 0);
        assert_eq!(hamming_distance(0b0000, 0b1111), 4);
        assert_eq!(hamming_distance(0b1010, 0b0101), 4);
    }

    #[test]
    fn test_are_similar() {
        // Same hash
        assert!(are_similar(0b1010, 0b1010, 0));

        // 1 bit difference
        assert!(are_similar(0b1010, 0b1011, 1));
        assert!(!are_similar(0b1010, 0b1011, 0));
    }
}