import { invoke } from '@tauri-apps/api/core';
import { listen, UnlistenFn } from '@tauri-apps/api/event';
import { CuraMediaFile } from '../../types/curaMediaFile';
import { CuraProgress } from '../../types/curaProgress';

export interface SelectFoldersResult {
  folders: string[];
  count: number;
}

export interface ScanResult {
  scanId: string;
  files: CuraMediaFile[];
  totalFiles: number;
}

export type ScanProgressListener = (progress: CuraProgress) => void;

/**
 * Get the application version
 */
export async function getVersion(): Promise<string> {
  return invoke<string>('get_version');
}

/**
 * Open a folder selection dialog
 */
export async function selectFolders(): Promise<SelectFoldersResult> {
  return invoke<SelectFoldersResult>('select_folders');
}

/**
 * Start scanning folders for media files
 */
export async function startScan(folders: string[]): Promise<ScanResult> {
  return invoke<ScanResult>('start_scan', { folders });
}

/**
 * Cancel an active scan
 */
export async function cancelScan(scanId: string): Promise<void> {
  return invoke('cancel_scan', { scanId });
}

/**
 * Subscribe to scan progress events
 */
export async function onScanProgress(
  callback: ScanProgressListener
): Promise<UnlistenFn> {
  return listen<CuraProgress>('scan-progress', (event) => {
    callback(event.payload);
  });
}

export const curaScanApi = {
  getVersion,
  selectFolders,
  startScan,
  cancelScan,
  onScanProgress,
};