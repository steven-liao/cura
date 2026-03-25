import { create } from 'zustand';

export type ViewType = 'home' | 'scan' | 'dedupe' | 'organize' | 'smart' | 'settings';

interface NavigationState {
  currentView: ViewType;
  setView: (view: ViewType) => void;
}

export const useNavigationStore = create<NavigationState>((set) => ({
  currentView: 'scan',
  setView: (view) => set({ currentView: view }),
}));
