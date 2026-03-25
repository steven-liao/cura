/**
 * Represents a media file (photo or video) in the Cura system
 */
export interface CuraMediaFile {
  /** Unique identifier for this file */
  id: string;
  /** Full path to the file */
  path: string;
  /** File name without path */
  fileName: string;
  /** File size in bytes */
  fileSize: number;
  /** File extension (lowercase, without dot) */
  extension: string;
  /** MD5 hash of the file content */
  md5Hash?: string;
  /** SHA-256 hash of the file content */
  sha256Hash?: string;
  /** Perceptual hash for similarity detection */
  perceptualHash?: number;
  /** Date the photo/video was taken (from EXIF or file metadata) */
  dateTaken?: string;
  /** Width in pixels (for images/videos) */
  width?: number;
  /** Height in pixels (for images/videos) */
  height?: number;
  /** Camera model (from EXIF) */
  cameraModel?: string;
  /** Whether this file is flagged as blurry */
  isBlurry: boolean;
  /** Whether this file is the "leader" of a burst group */
  isBurstLeader: boolean;
  /** When this file was indexed */
  indexedAt: string;
}

/**
 * Create a CuraMediaFile with default values
 */
export function createCuraMediaFile(overrides: Partial<CuraMediaFile> = {}): CuraMediaFile {
  return {
    id: '',
    path: '',
    fileName: '',
    fileSize: 0,
    extension: '',
    isBlurry: false,
    isBurstLeader: false,
    indexedAt: new Date().toISOString(),
    ...overrides,
  };
}

/**
 * Check if a file is an image based on extension
 */
export function isImage(extension: string): boolean {
  const imageExtensions = ['jpg', 'jpeg', 'png', 'gif', 'bmp', 'webp', 'heic', 'heif', 'tiff', 'tif'];
  return imageExtensions.includes(extension.toLowerCase());
}

/**
 * Check if a file is a video based on extension
 */
export function isVideo(extension: string): boolean {
  const videoExtensions = ['mp4', 'mov', 'avi', 'mkv', 'wmv', 'flv', 'webm', 'm4v'];
  return videoExtensions.includes(extension.toLowerCase());
}

/**
 * Format file size for display
 */
export function formatFileSize(bytes: number): string {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(1))} ${sizes[i]}`;
}