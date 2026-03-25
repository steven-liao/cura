import { CuraMediaFile } from './curaMediaFile';

export type DuplicateType = 'exact' | 'similar' | 'nameMatch';

export interface DuplicateGroup {
  id: string;
  duplicateType: DuplicateType;
  hash?: string;
  files: CuraMediaFile[];
  wastedSpace: number;
  removableCount: number;
}

export interface DuplicateResult {
  exactDuplicates: DuplicateGroup[];
  similarImages: DuplicateGroup[];
  nameMatches: DuplicateGroup[];
  totalWastedSpace: number;
  totalGroups: number;
}

export function formatWastedSpace(bytes: number): string {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(1))} ${sizes[i]}`;
}
