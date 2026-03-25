export type DateFolderFormat = 'yearMonthDay' | 'yearMonth' | 'yearOnly' | 'flatDate';
export type LocationGranularity = 'country' | 'city' | 'exact';

export interface OrganizeStrategy {
  type: 'byDate' | 'byCamera' | 'byType' | 'byLocation' | 'custom';
  dateFormat?: DateFolderFormat;
  locationGranularity?: LocationGranularity;
  customPattern?: string;
}

export interface FileOperation {
  source: string;
  destination: string;
  operationType: 'move' | 'copy';
}

export interface OrganizePreview {
  operations: FileOperation[];
  totalFiles: number;
  wouldOverwrite: string[];
}

export interface OrganizeResult {
  taskId: string;
  filesMoved: number;
  filesCopied: number;
  errors: string[];
  destinationPath: string;
}

export const DEFAULT_STRATEGIES: { name: string; strategy: OrganizeStrategy; description: string }[] = [
  {
    name: 'By Date (Year/Month/Day)',
    strategy: { type: 'byDate', dateFormat: 'yearMonthDay' },
    description: 'Organize photos into folders by year, then month, then day',
  },
  {
    name: 'By Date (Year/Month)',
    strategy: { type: 'byDate', dateFormat: 'yearMonth' },
    description: 'Organize photos into folders by year, then month',
  },
  {
    name: 'By Camera',
    strategy: { type: 'byCamera' },
    description: 'Group photos by camera model',
  },
  {
    name: 'By File Type',
    strategy: { type: 'byType' },
    description: 'Separate images and videos into different folders',
  },
];
