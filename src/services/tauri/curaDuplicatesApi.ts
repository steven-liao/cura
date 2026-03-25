import { invoke } from '@tauri-apps/api/core';
import { DuplicateResult } from '../../types/duplicates';
import { CuraMediaFile } from '../../types/curaMediaFile';

export async function findDuplicates(
  files: CuraMediaFile[],
  similarityThreshold?: number
): Promise<DuplicateResult> {
  return await invoke('find_duplicates', {
    files,
    similarityThreshold,
  });
}

export async function deleteFile(path: string): Promise<void> {
  return await invoke('delete_file', { path });
}

export async function moveFile(source: string, destination: string): Promise<void> {
  return await invoke('move_file', { source, destination });
}
