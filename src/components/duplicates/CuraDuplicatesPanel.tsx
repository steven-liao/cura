import { useState } from 'react';
import { Trash2, RefreshCw, CheckCircle, AlertCircle, Copy, Image, FileText } from 'lucide-react';
import { useCuraMediaStore } from '../../stores/curaMediaStore';
import { useDuplicatesStore } from '../../stores/duplicatesStore';
import { findDuplicates, deleteFile } from '../../services/tauri/curaDuplicatesApi';
import { DuplicateGroup, DuplicateType, formatWastedSpace } from '../../types/duplicates';
import { formatFileSize } from '../../types/curaMediaFile';

function DuplicateGroupCard({
  group,
  type,
  selected,
  filesToDelete,
  onToggleGroup,
  onToggleFile,
}: {
  group: DuplicateGroup;
  type: DuplicateType;
  selected: boolean;
  filesToDelete: Set<string>;
  onToggleGroup: (id: string) => void;
  onToggleFile: (id: string) => void;
}) {
  const typeConfig = {
    exact: { icon: Copy, color: 'text-red-400', bg: 'bg-red-500/10', label: 'Exact Duplicate' },
    similar: { icon: Image, color: 'text-yellow-400', bg: 'bg-yellow-500/10', label: 'Similar Image' },
    nameMatch: { icon: FileText, color: 'text-blue-400', bg: 'bg-blue-500/10', label: 'Same Name' },
  };

  const config = typeConfig[type];
  const Icon = config.icon;

  const bestFile = group.files.reduce((best, file) => {
    // Prefer larger files (higher quality), then by date taken
    if (file.fileSize > best.fileSize) return file;
    if (file.fileSize === best.fileSize) {
      if (file.dateTaken && best.dateTaken && file.dateTaken > best.dateTaken) return file;
    }
    return best;
  }, group.files[0]);

  return (
    <div className={`border rounded-lg p-4 transition-colors ${
      selected ? 'border-cura-500 bg-cura-500/10' : 'border-slate-700 bg-slate-800/50'
    }`}>
      <div className="flex items-start justify-between mb-3">
        <div className="flex items-center gap-2">
          <input
            type="checkbox"
            checked={selected}
            onChange={() => onToggleGroup(group.id)}
            className="w-4 h-4 rounded border-slate-600 bg-slate-700 text-cura-600 focus:ring-cura-500"
          />
          <div className={`p-1.5 rounded ${config.bg}`}>
            <Icon className={`w-4 h-4 ${config.color}`} />
          </div>
          <div>
            <span className={`text-xs font-medium ${config.color}`}>{config.label}</span>
            <h4 className="text-sm font-medium text-slate-200">
              {group.files.length} files • {formatWastedSpace(group.wastedSpace)} wasted
            </h4>
          </div>
        </div>
      </div>

      <div className="space-y-2">
        {group.files.map((file) => {
          const isBest = file.id === bestFile.id;
          const isSelectedForDelete = filesToDelete.has(file.id);

          return (
            <div
              key={file.id}
              className={`flex items-center gap-3 p-2 rounded-lg text-sm ${
                isBest ? 'bg-green-500/10 border border-green-500/30' : 'bg-slate-700/50'
              }`}
            >
              <input
                type="checkbox"
                checked={isSelectedForDelete}
                onChange={() => onToggleFile(file.id)}
                className="w-4 h-4 rounded border-slate-600 bg-slate-700 text-cura-600 focus:ring-cura-500"
              />
              <div className="flex-1 min-w-0">
                <p className="font-medium text-slate-200 truncate">{file.fileName}</p>
                <p className="text-xs text-slate-500 truncate">{file.path}</p>
                <div className="flex items-center gap-3 mt-1 text-xs text-slate-400">
                  <span>{formatFileSize(file.fileSize)}</span>
                  {file.width && file.height && (
                    <span>{file.width}×{file.height}</span>
                  )}
                  {file.dateTaken && (
                    <span>{new Date(file.dateTaken).toLocaleDateString()}</span>
                  )}
                </div>
              </div>
              {isBest && (
                <span className="text-xs text-green-400 font-medium">Best</span>
              )}
            </div>
          );
        })}
      </div>

      {type !== 'nameMatch' && (
        <div className="mt-3 flex justify-end">
          <button
            onClick={() => {
              // Select all except best for deletion
              const toDelete = group.files
                .filter(f => f.id !== bestFile.id)
                .map(f => f.id);
              toDelete.forEach(id => onToggleFile(id));
            }}
            className="text-xs text-slate-400 hover:text-cura-400 transition-colors"
          >
            Auto-select duplicates (keep best)
          </button>
        </div>
      )}
    </div>
  );
}

function CuraDuplicatesPanel() {
  const files = useCuraMediaStore((state) => state.files);
  const {
    duplicateResult,
    selectedGroups,
    filesToDelete,
    isScanning,
    error,
    setDuplicateResult,
    toggleGroupSelection,
    clearGroupSelection,
    toggleFileForDeletion,
    setIsScanning,
    setError,
    clearResults,
  } = useDuplicatesStore();

  const [activeTab, setActiveTab] = useState<DuplicateType | 'all'>('all');

  const handleScan = async () => {
    if (files.length === 0) {
      setError('No files to scan. Please scan some folders first.');
      return;
    }

    // Debug logging
    console.log('[CuraDuplicatesPanel] Files count:', files.length);
    console.log('[CuraDuplicatesPanel] First file:', files[0]);
    console.log('[CuraDuplicatesPanel] Files with sha256Hash:', files.filter(f => f.sha256Hash).length);

    setIsScanning(true);
    setError(null);
    clearResults();

    try {
      const result = await findDuplicates(files, 10);
      setDuplicateResult(result);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to find duplicates');
    } finally {
      setIsScanning(false);
    }
  };

  const handleDeleteSelected = async () => {
    if (filesToDelete.size === 0) return;

    if (!confirm(`Delete ${filesToDelete.size} selected files? This cannot be undone.`)) {
      return;
    }

    // Get the files to delete
    const pathsToDelete: string[] = [];
    files.forEach(file => {
      if (filesToDelete.has(file.id)) {
        pathsToDelete.push(file.path);
      }
    });

    let successCount = 0;
    for (const path of pathsToDelete) {
      try {
        await deleteFile(path);
        successCount++;
      } catch (err) {
        console.error('Failed to delete:', path, err);
      }
    }

    // Refresh results
    alert(`Deleted ${successCount} files`);
    handleScan();
  };

  const getFilteredGroups = (): { group: DuplicateGroup; type: DuplicateType }[] => {
    if (!duplicateResult) return [];

    const groups: { group: DuplicateGroup; type: DuplicateType }[] = [];

    if (activeTab === 'all' || activeTab === 'exact') {
      duplicateResult.exactDuplicates.forEach(g => groups.push({ group: g, type: 'exact' }));
    }
    if (activeTab === 'all' || activeTab === 'similar') {
      duplicateResult.similarImages.forEach(g => groups.push({ group: g, type: 'similar' }));
    }
    if (activeTab === 'all' || activeTab === 'nameMatch') {
      duplicateResult.nameMatches.forEach(g => groups.push({ group: g, type: 'nameMatch' }));
    }

    return groups;
  };

  const filteredGroups = getFilteredGroups();
  const hasResults = duplicateResult && duplicateResult.totalGroups > 0;

  return (
    <div className="h-full flex flex-col">
      {/* Header */}
      <div className="flex items-center justify-between mb-6">
        <div>
          <h2 className="text-2xl font-semibold text-white">Duplicates</h2>
          <p className="text-slate-400 text-sm">
            {files.length > 0
              ? `${files.length} files indexed`
              : 'Scan folders first to find duplicates'}
          </p>
        </div>
        <div className="flex items-center gap-3">
          {hasResults && (
            <div className="text-right mr-4">
              <p className="text-sm text-slate-300">
                {duplicateResult.totalGroups} groups found
              </p>
              <p className="text-sm text-red-400">
                {formatWastedSpace(duplicateResult.totalWastedSpace)} wasted
              </p>
            </div>
          )}
          <button
            onClick={handleScan}
            disabled={isScanning || files.length === 0}
            className="flex items-center gap-2 px-4 py-2 bg-cura-600 hover:bg-cura-700 disabled:opacity-50 disabled:cursor-not-allowed text-white rounded-lg font-medium transition-colors"
          >
            {isScanning ? (
              <RefreshCw className="w-4 h-4 animate-spin" />
            ) : (
              <Copy className="w-4 h-4" />
            )}
            {isScanning ? 'Scanning...' : 'Find Duplicates'}
          </button>
        </div>
      </div>

      {/* Error */}
      {error && (
        <div className="mb-4 p-4 bg-red-500/10 border border-red-500/30 rounded-lg flex items-center gap-3">
          <AlertCircle className="w-5 h-5 text-red-400" />
          <p className="text-red-400">{error}</p>
        </div>
      )}

      {/* Tabs */}
      {hasResults && (
        <div className="flex items-center gap-1 mb-4 border-b border-slate-700">
          {(['all', 'exact', 'similar', 'nameMatch'] as const).map((tab) => (
            <button
              key={tab}
              onClick={() => setActiveTab(tab)}
              className={`px-4 py-2 text-sm font-medium transition-colors relative ${
                activeTab === tab
                  ? 'text-cura-400'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              {tab === 'all' && 'All Groups'}
              {tab === 'exact' && `Exact (${duplicateResult.exactDuplicates.length})`}
              {tab === 'similar' && `Similar (${duplicateResult.similarImages.length})`}
              {tab === 'nameMatch' && `Name Match (${duplicateResult.nameMatches.length})`}
              {activeTab === tab && (
                <div className="absolute bottom-0 left-0 right-0 h-0.5 bg-cura-500" />
              )}
            </button>
          ))}
        </div>
      )}

      {/* Results */}
      <div className="flex-1 overflow-auto">
        {!hasResults && !isScanning && (
          <div className="h-full flex flex-col items-center justify-center text-slate-500">
            <Copy className="w-16 h-16 mb-4 opacity-30" />
            <p className="text-lg font-medium">No duplicates found</p>
            <p className="text-sm">Click "Find Duplicates" to scan your files</p>
          </div>
        )}

        {hasResults && filteredGroups.length === 0 && (
          <div className="h-full flex flex-col items-center justify-center text-slate-500">
            <p className="text-lg font-medium">No groups in this category</p>
          </div>
        )}

        {hasResults && (
          <div className="space-y-4">
            {filteredGroups.map(({ group, type }) => (
              <DuplicateGroupCard
                key={group.id}
                group={group}
                type={type}
                selected={selectedGroups.has(group.id)}
                filesToDelete={filesToDelete}
                onToggleGroup={toggleGroupSelection}
                onToggleFile={toggleFileForDeletion}
              />
            ))}
          </div>
        )}
      </div>

      {/* Actions Footer */}
      {filesToDelete.size > 0 && (
        <div className="mt-4 p-4 bg-slate-800 border border-slate-700 rounded-lg flex items-center justify-between">
          <div className="flex items-center gap-3">
            <CheckCircle className="w-5 h-5 text-cura-400" />
            <span className="text-slate-200">
              {filesToDelete.size} files selected for deletion
            </span>
          </div>
          <div className="flex items-center gap-2">
            <button
              onClick={() => clearGroupSelection()}
              className="px-4 py-2 text-slate-400 hover:text-white transition-colors"
            >
              Clear
            </button>
            <button
              onClick={handleDeleteSelected}
              className="flex items-center gap-2 px-4 py-2 bg-red-600 hover:bg-red-700 text-white rounded-lg font-medium transition-colors"
            >
              <Trash2 className="w-4 h-4" />
              Delete Selected
            </button>
          </div>
        </div>
      )}
    </div>
  );
}

export default CuraDuplicatesPanel;
