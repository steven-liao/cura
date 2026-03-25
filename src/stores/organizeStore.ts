import { create } from 'zustand';
import { OrganizeStrategy, OrganizePreview, OrganizeResult } from '../types/organize';

interface OrganizeState {
  // Settings
  selectedStrategy: OrganizeStrategy | null;
  destinationPath: string | null;
  copyInsteadOfMove: boolean;

  // Preview
  preview: OrganizePreview | null;

  // Execution
  isPreviewing: boolean;
  isOrganizing: boolean;
  result: OrganizeResult | null;
  error: string | null;

  // Actions
  setSelectedStrategy: (strategy: OrganizeStrategy | null) => void;
  setDestinationPath: (path: string | null) => void;
  setCopyInsteadOfMove: (copy: boolean) => void;
  setPreview: (preview: OrganizePreview | null) => void;
  setIsPreviewing: (previewing: boolean) => void;
  setIsOrganizing: (organizing: boolean) => void;
  setResult: (result: OrganizeResult | null) => void;
  setError: (error: string | null) => void;
  reset: () => void;
}

const initialState = {
  selectedStrategy: null,
  destinationPath: null,
  copyInsteadOfMove: false,
  preview: null,
  isPreviewing: false,
  isOrganizing: false,
  result: null,
  error: null,
};

export const useOrganizeStore = create<OrganizeState>((set) => ({
  ...initialState,

  setSelectedStrategy: (strategy) => set({ selectedStrategy: strategy }),
  setDestinationPath: (path) => set({ destinationPath: path }),
  setCopyInsteadOfMove: (copy) => set({ copyInsteadOfMove: copy }),
  setPreview: (preview) => set({ preview }),
  setIsPreviewing: (isPreviewing) => set({ isPreviewing }),
  setIsOrganizing: (isOrganizing) => set({ isOrganizing }),
  setResult: (result) => set({ result }),
  setError: (error) => set({ error }),
  reset: () => set(initialState),
}));
