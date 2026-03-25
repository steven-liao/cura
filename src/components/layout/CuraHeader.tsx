import { FolderOpen, Settings, HelpCircle } from 'lucide-react';

function CuraHeader() {
  return (
    <header className="h-14 bg-slate-900 border-b border-slate-700 flex items-center justify-between px-4">
      <div className="flex items-center gap-3">
        <div className="w-8 h-8 text-cura-500">
          <svg viewBox="0 0 100 100" fill="currentColor">
            <circle cx="50" cy="50" r="45" fill="none" stroke="currentColor" strokeWidth="4" />
            <circle cx="35" cy="40" r="8" />
            <circle cx="65" cy="40" r="8" />
            <path d="M 30 65 Q 50 80 70 65" fill="none" stroke="currentColor" strokeWidth="4" strokeLinecap="round" />
          </svg>
        </div>
        <h1 className="text-lg font-semibold text-white">Cura</h1>
        <span className="text-xs text-slate-500 bg-slate-800 px-2 py-0.5 rounded">Photo Manager</span>
      </div>

      <div className="flex items-center gap-2">
        <button
          className="flex items-center gap-2 px-3 py-1.5 text-sm text-slate-300 hover:text-white hover:bg-slate-800 rounded-md transition-colors"
          title="Open Folder"
        >
          <FolderOpen className="w-4 h-4" />
          <span>Open</span>
        </button>
        <button
          className="p-2 text-slate-400 hover:text-white hover:bg-slate-800 rounded-md transition-colors"
          title="Settings"
        >
          <Settings className="w-4 h-4" />
        </button>
        <button
          className="p-2 text-slate-400 hover:text-white hover:bg-slate-800 rounded-md transition-colors"
          title="Help"
        >
          <HelpCircle className="w-4 h-4" />
        </button>
      </div>
    </header>
  );
}

export default CuraHeader;