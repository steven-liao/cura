use exif::{Reader, In};
use std::path::Path;
use chrono::{DateTime, Utc, TimeZone, NaiveDateTime};

/// Extracted EXIF metadata
#[derive(Debug, Clone, Default)]
pub struct ExifData {
    pub date_taken: Option<DateTime<Utc>>,
    pub width: Option<u32>,
    pub height: Option<u32>,
    pub camera_model: Option<String>,
}

/// Extract EXIF metadata from an image file
pub fn extract_exif(path: &Path) -> Option<ExifData> {
    let file = std::fs::File::open(path).ok()?;
    let mut bufreader = std::io::BufReader::new(&file);
    let exifreader = Reader::new();
    let exif = exifreader.read_from_container(&mut bufreader).ok()?;

    let date_taken = exif.get_field(exif::Tag::DateTimeOriginal, In::PRIMARY)
        .and_then(|field| parse_exif_datetime(&field.display_value().to_string()));

    let width = exif.get_field(exif::Tag::PixelXDimension, In::PRIMARY)
        .and_then(|f| f.value.get_uint(0))
        .map(|v| v as u32);

    let height = exif.get_field(exif::Tag::PixelYDimension, In::PRIMARY)
        .and_then(|f| f.value.get_uint(0))
        .map(|v| v as u32);

    let camera_model = exif.get_field(exif::Tag::Model, In::PRIMARY)
        .map(|f| f.display_value().to_string());

    // If dimensions not in EXIF, try to get from image crate
    let (width, height) = if width.is_none() || height.is_none() {
        if let Ok(img) = image::open(path) {
            (width.or(Some(img.width())), height.or(Some(img.height())))
        } else {
            (width, height)
        }
    } else {
        (width, height)
    };

    Some(ExifData {
        date_taken,
        width,
        height,
        camera_model,
    })
}

/// Parse EXIF datetime string to DateTime<Utc>
/// EXIF format: "YYYY:MM:DD HH:MM:SS"
fn parse_exif_datetime(s: &str) -> Option<DateTime<Utc>> {
    let s = s.trim();

    // Try parsing EXIF format: "YYYY:MM:DD HH:MM:SS"
    if s.len() >= 19 {
        let year: i32 = s[0..4].parse().ok()?;
        let month: u32 = s[5..7].parse().ok()?;
        let day: u32 = s[8..10].parse().ok()?;
        let hour: u32 = s[11..13].parse().ok()?;
        let minute: u32 = s[14..16].parse().ok()?;
        let second: u32 = s[17..19].parse().ok()?;

        let naive = NaiveDateTime::new(
            chrono::NaiveDate::from_ymd_opt(year, month, day)?,
            chrono::NaiveTime::from_hms_opt(hour, minute, second)?,
        );

        return Some(Utc.from_utc_datetime(&naive));
    }

    None
}

#[cfg(test)]
mod tests {
    use super::*;
    use chrono::Datelike;

    #[test]
    fn test_parse_exif_datetime() {
        let result = parse_exif_datetime("2024:03:22 14:30:45");
        assert!(result.is_some());

        let dt = result.unwrap();
        assert_eq!(dt.year(), 2024);
        assert_eq!(dt.month(), 3);
        assert_eq!(dt.day(), 22);
    }

    #[test]
    fn test_parse_exif_datetime_invalid() {
        assert!(parse_exif_datetime("invalid").is_none());
        assert!(parse_exif_datetime("").is_none());
    }
}