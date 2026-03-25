/**
 * Progress information for scanning operations
 */
export interface CuraProgress {
  /** Current phase of the operation */
  phase: CuraProgressPhase;
  /** Number of files processed so far */
  current: number;
  /** Total number of files to process */
  total: number;
  /** Current file being processed */
  currentFile?: string;
  /** Processing speed (files per second) */
  speed?: number;
  /** Estimated time remaining in seconds */
  etaSeconds?: number;
}

/**
 * Phase of a scan operation
 */
export type CuraProgressPhase =
  | 'Initializing'
  | 'Scanning'
  | 'Hashing'
  | 'MetadataExtraction'
  | 'Deduplication'
  | 'Organizing'
  | 'Complete'
  | 'Cancelled'
  | { Error: string };

/**
 * Create a CuraProgress with default values
 */
export function createCuraProgress(overrides: Partial<CuraProgress> = {}): CuraProgress {
  return {
    phase: 'Initializing',
    current: 0,
    total: 0,
    ...overrides,
  };
}

/**
 * Format progress as a percentage
 */
export function formatProgressPercent(progress: CuraProgress): number {
  if (progress.total === 0) return 0;
  return Math.round((progress.current / progress.total) * 100);
}

/**
 * Format ETA as a human-readable string
 */
export function formatEta(seconds?: number): string {
  if (!seconds) return '--:--';

  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;

  if (hours > 0) {
    return `${hours}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
  }
  return `${minutes}:${secs.toString().padStart(2, '0')}`;
}