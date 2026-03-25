import { create } from 'zustand';
import { DuplicateResult } from '../types/duplicates';

interface DuplicatesState {
  // Results
  duplicateResult: DuplicateResult | null;
  selectedGroups: Set<string>;
  filesToDelete: Set<string>;
  isScanning: boolean;
  error: string | null;

  // Actions
  setDuplicateResult: (result: DuplicateResult | null) => void;
  toggleGroupSelection: (groupId: string) => void;
  selectAllGroups: () => void;
  clearGroupSelection: () => void;
  toggleFileForDeletion: (fileId: string) => void;
  selectFilesInGroup: (_groupId: string, fileIds: string[]) => void;
  clearFilesForDeletion: () => void;
  setIsScanning: (scanning: boolean) => void;
  setError: (error: string | null) => void;
  clearResults: () => void;
}

export const useDuplicatesStore = create<DuplicatesState>((set) => ({
  // Initial state
  duplicateResult: null,
  selectedGroups: new Set(),
  filesToDelete: new Set(),
  isScanning: false,
  error: null,

  // Actions
  setDuplicateResult: (result) => set({ duplicateResult: result }),

  toggleGroupSelection: (groupId) => set((state) => {
    const newSelection = new Set(state.selectedGroups);
    if (newSelection.has(groupId)) {
      newSelection.delete(groupId);
    } else {
      newSelection.add(groupId);
    }
    return { selectedGroups: newSelection };
  }),

  selectAllGroups: () => set((state) => {
    if (!state.duplicateResult) return state;
    const allIds = [
      ...state.duplicateResult.exactDuplicates.map(g => g.id),
      ...state.duplicateResult.similarImages.map(g => g.id),
      ...state.duplicateResult.nameMatches.map(g => g.id),
    ];
    return { selectedGroups: new Set(allIds) };
  }),

  clearGroupSelection: () => set({ selectedGroups: new Set() }),

  toggleFileForDeletion: (fileId) => set((state) => {
    const newSelection = new Set(state.filesToDelete);
    if (newSelection.has(fileId)) {
      newSelection.delete(fileId);
    } else {
      newSelection.add(fileId);
    }
    return { filesToDelete: newSelection };
  }),

  selectFilesInGroup: (_groupId, fileIds) => set((state) => {
    const newSelection = new Set(state.filesToDelete);
    fileIds.forEach(id => newSelection.add(id));
    return { filesToDelete: newSelection };
  }),

  clearFilesForDeletion: () => set({ filesToDelete: new Set() }),

  setIsScanning: (scanning) => set({ isScanning: scanning }),

  setError: (error) => set({ error }),

  clearResults: () => set({
    duplicateResult: null,
    selectedGroups: new Set(),
    filesToDelete: new Set(),
    error: null,
  }),
}));
