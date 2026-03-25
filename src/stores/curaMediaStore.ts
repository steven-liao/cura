import { create } from 'zustand';
import { CuraMediaFile } from '../types/curaMediaFile';
import { CuraProgress, createCuraProgress } from '../types/curaProgress';

interface CuraMediaState {
  // State
  files: CuraMediaFile[];
  selectedFiles: Set<string>;
  sourceFolders: string[];
  isScanning: boolean;
  progress: CuraProgress;
  currentScanId: string | null;
  error: string | null;

  // Actions
  setFiles: (files: CuraMediaFile[]) => void;
  addFiles: (files: CuraMediaFile[]) => void;
  clearFiles: () => void;
  toggleSelection: (id: string) => void;
  selectAll: () => void;
  clearSelection: () => void;
  setSourceFolders: (folders: string[]) => void;
  addSourceFolder: (folder: string) => void;
  removeSourceFolder: (folder: string) => void;
  setIsScanning: (scanning: boolean) => void;
  setProgress: (progress: CuraProgress) => void;
  setCurrentScanId: (id: string | null) => void;
  setError: (error: string | null) => void;
  resetScanState: () => void;
}

export const useCuraMediaStore = create<CuraMediaState>((set) => ({
  // Initial state
  files: [],
  selectedFiles: new Set(),
  sourceFolders: [],
  isScanning: false,
  progress: createCuraProgress(),
  currentScanId: null,
  error: null,

  // Actions
  setFiles: (files) => set({ files }),

  addFiles: (files) => set((state) => ({
    files: [...state.files, ...files]
  })),

  clearFiles: () => set({ files: [], selectedFiles: new Set() }),

  toggleSelection: (id) => set((state) => {
    const newSelection = new Set(state.selectedFiles);
    if (newSelection.has(id)) {
      newSelection.delete(id);
    } else {
      newSelection.add(id);
    }
    return { selectedFiles: newSelection };
  }),

  selectAll: () => set((state) => ({
    selectedFiles: new Set(state.files.map(f => f.id))
  })),

  clearSelection: () => set({ selectedFiles: new Set() }),

  setSourceFolders: (folders) => set({ sourceFolders: folders }),

  addSourceFolder: (folder) => set((state) => {
    if (state.sourceFolders.includes(folder)) return state;
    return { sourceFolders: [...state.sourceFolders, folder] };
  }),

  removeSourceFolder: (folder) => set((state) => ({
    sourceFolders: state.sourceFolders.filter(f => f !== folder)
  })),

  setIsScanning: (scanning) => set({ isScanning: scanning }),

  setProgress: (progress) => {
    console.log('[Store] Setting progress:', progress);
    return set({ progress: progress as CuraProgress });
  },

  setCurrentScanId: (id) => set({ currentScanId: id }),

  setError: (error) => set({ error }),

  resetScanState: () => set({
    isScanning: false,
    progress: createCuraProgress(),
    currentScanId: null,
    error: null,
  }),
}));