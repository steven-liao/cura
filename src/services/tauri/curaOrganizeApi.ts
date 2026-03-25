import { invoke } from '@tauri-apps/api/core';
import { CuraMediaFile } from '../../types/curaMediaFile';
import { OrganizeStrategy, OrganizePreview, OrganizeResult } from '../../types/organize';

export async function previewOrganize(
  files: CuraMediaFile[],
  strategy: OrganizeStrategy,
  destination: string
): Promise<OrganizePreview> {
  return await invoke('preview_organize', {
    files,
    strategy,
    destination,
  });
}

export async function executeOrganize(
  files: CuraMediaFile[],
  strategy: OrganizeStrategy,
  destination: string,
  copyInsteadOfMove: boolean = false
): Promise<OrganizeResult> {
  return await invoke('execute_organize', {
    files,
    strategy,
    destination,
    copyInsteadOfMove,
  });
}

export async function selectOrganizeDestination(): Promise<string | null> {
  return await invoke('select_organize_destination');
}
