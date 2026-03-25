import { ReactNode } from 'react';
import CuraHeader from './CuraHeader';
import CuraSidebar from './CuraSidebar';
import { CuraScanPanel } from '../scan';
import CuraDuplicatesPanel from '../duplicates/CuraDuplicatesPanel';
import CuraOrganizePanel from '../organize/CuraOrganizePanel';
import { useNavigationStore } from '../../stores/navigationStore';

interface CuraMainLayoutProps {
  children?: ReactNode;
}

function ViewContent() {
  const { currentView } = useNavigationStore();

  switch (currentView) {
    case 'home':
      return (
        <div className="flex flex-col items-center justify-center h-full text-slate-400">
          <h2 className="text-2xl font-semibold text-slate-200 mb-4">Dashboard</h2>
          <p>Welcome to Cura Photo Manager</p>
        </div>
      );
    case 'scan':
      return <CuraScanPanel />;
    case 'dedupe':
      return <CuraDuplicatesPanel />;
    case 'organize':
      return <CuraOrganizePanel />;
    case 'smart':
      return (
        <div className="flex flex-col items-center justify-center h-full text-slate-400">
          <h2 className="text-2xl font-semibold text-slate-200 mb-4">Smart Features</h2>
          <p>AI-powered features coming soon</p>
        </div>
      );
    case 'settings':
      return (
        <div className="flex flex-col items-center justify-center h-full text-slate-400">
          <h2 className="text-2xl font-semibold text-slate-200 mb-4">Settings</h2>
          <p>Application settings coming soon</p>
        </div>
      );
    default:
      return <CuraScanPanel />;
  }
}

function CuraMainLayout({ children }: CuraMainLayoutProps) {
  return (
    <div className="flex flex-col h-screen bg-slate-900 text-slate-100">
      <CuraHeader />
      <div className="flex flex-1 overflow-hidden">
        <CuraSidebar />
        <main className="flex-1 overflow-auto p-6 bg-slate-800">
          {children || <ViewContent />}
        </main>
      </div>
    </div>
  );
}

export default CuraMainLayout;