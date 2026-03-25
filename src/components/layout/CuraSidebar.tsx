import { Home, Search, Copy, FolderTree, Sparkles, Settings } from 'lucide-react';
import { useNavigationStore, ViewType } from '../../stores/navigationStore';

interface NavItem {
  id: ViewType;
  label: string;
  icon: React.ReactNode;
}

function CuraSidebar() {
  const { currentView, setView } = useNavigationStore();

  const navItems: NavItem[] = [
    { id: 'home', label: 'Dashboard', icon: <Home className="w-5 h-5" /> },
    { id: 'scan', label: 'Scan', icon: <Search className="w-5 h-5" /> },
    { id: 'dedupe', label: 'Duplicates', icon: <Copy className="w-5 h-5" /> },
    { id: 'organize', label: 'Organize', icon: <FolderTree className="w-5 h-5" /> },
    { id: 'smart', label: 'Smart Features', icon: <Sparkles className="w-5 h-5" /> },
  ];

  return (
    <aside className="w-56 bg-slate-900 border-r border-slate-700 flex flex-col">
      <nav className="flex-1 p-3">
        <ul className="space-y-1">
          {navItems.map((item) => (
            <li key={item.id}>
              <button
                onClick={() => setView(item.id)}
                className={`w-full flex items-center gap-3 px-3 py-2 rounded-md text-sm font-medium transition-colors ${
                  currentView === item.id
                    ? 'bg-cura-600 text-white'
                    : 'text-slate-300 hover:text-white hover:bg-slate-800'
                }`}
              >
                {item.icon}
                <span>{item.label}</span>
              </button>
            </li>
          ))}
        </ul>
      </nav>

      <div className="p-3 border-t border-slate-700">
        <button
          onClick={() => setView('settings')}
          className={`w-full flex items-center gap-3 px-3 py-2 rounded-md text-sm font-medium transition-colors ${
            currentView === 'settings'
              ? 'bg-cura-600 text-white'
              : 'text-slate-400 hover:text-white hover:bg-slate-800'
          }`}
        >
          <Settings className="w-5 h-5" />
          <span>Settings</span>
        </button>
      </div>

      <div className="p-3 border-t border-slate-700">
        <div className="bg-slate-800 rounded-lg p-3">
          <p className="text-xs text-slate-400 mb-2">Quick Stats</p>
          <div className="space-y-1">
            <div className="flex justify-between text-sm">
              <span className="text-slate-500">Files scanned</span>
              <span className="text-white font-medium">0</span>
            </div>
            <div className="flex justify-between text-sm">
              <span className="text-slate-500">Duplicates</span>
              <span className="text-white font-medium">0</span>
            </div>
          </div>
        </div>
      </div>
    </aside>
  );
}

export default CuraSidebar;