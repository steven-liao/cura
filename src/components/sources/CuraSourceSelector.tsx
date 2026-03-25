import { useState } from 'react';
import { FolderOpen, Plus, X, ChevronRight } from 'lucide-react';
import { curaScanApi } from '../../services/tauri';
import { useCuraMediaStore } from '../../stores';

function CuraSourceSelector() {
  const { sourceFolders, setSourceFolders } = useCuraMediaStore();
  const [isLoading, setIsLoading] = useState(false);

  const handleAddFolder = async () => {
    setIsLoading(true);
    try {
      const result = await curaScanApi.selectFolders();
      if (result.folders.length > 0) {
        // Add only new folders
        const newFolders = result.folders.filter(
          (f) => !sourceFolders.includes(f)
        );
        setSourceFolders([...sourceFolders, ...newFolders]);
      }
    } catch (error) {
      console.error('Failed to select folders:', error);
    } finally {
      setIsLoading(false);
    }
  };

  const handleRemoveFolder = (folder: string) => {
    setSourceFolders(sourceFolders.filter((f) => f !== folder));
  };

  const handleClearAll = () => {
    setSourceFolders([]);
  };

  return (
    <div className="bg-slate-800 rounded-lg p-4">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-medium text-white">Source Folders</h3>
        <div className="flex gap-2">
          <button
            onClick={handleAddFolder}
            disabled={isLoading}
            className="flex items-center gap-2 px-3 py-1.5 bg-cura-600 hover:bg-cura-700 disabled:bg-slate-600 text-white text-sm rounded-md transition-colors"
          >
            <Plus className="w-4 h-4" />
            Add Folder
          </button>
          {sourceFolders.length > 0 && (
            <button
              onClick={handleClearAll}
              className="flex items-center gap-2 px-3 py-1.5 bg-slate-700 hover:bg-slate-600 text-slate-300 text-sm rounded-md transition-colors"
            >
              <X className="w-4 h-4" />
              Clear
            </button>
          )}
        </div>
      </div>

      {sourceFolders.length === 0 ? (
        <div className="flex flex-col items-center justify-center py-8 text-center">
          <FolderOpen className="w-12 h-12 text-slate-500 mb-3" />
          <p className="text-slate-400 mb-2">No folders selected</p>
          <p className="text-slate-500 text-sm">
            Click "Add Folder" to select the folders you want to scan for photos and videos
          </p>
        </div>
      ) : (
        <ul className="space-y-2">
          {sourceFolders.map((folder) => (
            <li
              key={folder}
              className="flex items-center justify-between bg-slate-700 rounded-md px-3 py-2"
            >
              <div className="flex items-center gap-2 flex-1 min-w-0">
                <ChevronRight className="w-4 h-4 text-cura-500 flex-shrink-0" />
                <span className="text-sm text-slate-200 truncate" title={folder}>
                  {folder}
                </span>
              </div>
              <button
                onClick={() => handleRemoveFolder(folder)}
                className="p-1 text-slate-400 hover:text-red-400 transition-colors"
                title="Remove folder"
              >
                <X className="w-4 h-4" />
              </button>
            </li>
          ))}
        </ul>
      )}

      {sourceFolders.length > 0 && (
        <div className="mt-4 pt-4 border-t border-slate-700">
          <p className="text-sm text-slate-400">
            {sourceFolders.length} folder{sourceFolders.length !== 1 ? 's' : ''} selected
          </p>
        </div>
      )}
    </div>
  );
}

export default CuraSourceSelector;