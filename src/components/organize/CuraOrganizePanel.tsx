import { Play, Eye, CheckCircle, AlertCircle, FolderOpen, RefreshCw, ArrowRight } from 'lucide-react';
import { useCuraMediaStore } from '../../stores/curaMediaStore';
import { useOrganizeStore } from '../../stores/organizeStore';
import { previewOrganize, executeOrganize, selectOrganizeDestination } from '../../services/tauri/curaOrganizeApi';
import { DEFAULT_STRATEGIES } from '../../types/organize';

function StrategyCard({
  name,
  description,
  selected,
  onClick,
}: {
  name: string;
  description: string;
  selected: boolean;
  onClick: () => void;
}) {
  return (
    <button
      onClick={onClick}
      className={`w-full p-4 rounded-lg border-2 text-left transition-all ${
        selected
          ? 'border-cura-500 bg-cura-500/10'
          : 'border-slate-700 bg-slate-800 hover:border-slate-600'
      }`}
    >
      <h4 className={`font-medium ${selected ? 'text-cura-400' : 'text-slate-200'}`}>
        {name}
      </h4>
      <p className="text-sm text-slate-500 mt-1">{description}</p>
    </button>
  );
}

function PreviewPanel({
  preview,
  onConfirm,
  onCancel,
  isOrganizing,
}: {
  preview: { operations: any[]; totalFiles: number; wouldOverwrite: string[] };
  onConfirm: () => void;
  onCancel: () => void;
  isOrganizing: boolean;
}) {
  const copyInsteadOfMove = useOrganizeStore((state) => state.copyInsteadOfMove);

  // Group operations by destination folder
  const folderGroups: Record<string, typeof preview.operations> = {};
  preview.operations.forEach((op) => {
    const folder = op.destination.substring(0, op.destination.lastIndexOf('/') || op.destination.lastIndexOf('\\'));
    if (!folderGroups[folder]) folderGroups[folder] = [];
    folderGroups[folder].push(op);
  });

  return (
    <div className="bg-slate-800 border border-slate-700 rounded-lg p-4">
      <h3 className="text-lg font-semibold text-white mb-4 flex items-center gap-2">
        <Eye className="w-5 h-5 text-cura-400" />
        Preview
      </h3>

      <div className="mb-4 p-3 bg-slate-700/50 rounded-lg">
        <div className="flex items-center justify-between text-sm">
          <span className="text-slate-400">Total files to {copyInsteadOfMove ? 'copy' : 'move'}:</span>
          <span className="text-white font-medium">{preview.totalFiles}</span>
        </div>
        <div className="flex items-center justify-between text-sm mt-2">
          <span className="text-slate-400">Destination folders:</span>
          <span className="text-white font-medium">{Object.keys(folderGroups).length}</span>
        </div>
      </div>

      {preview.wouldOverwrite.length > 0 && (
        <div className="mb-4 p-3 bg-yellow-500/10 border border-yellow-500/30 rounded-lg">
          <div className="flex items-center gap-2 text-yellow-400 mb-2">
            <AlertCircle className="w-4 h-4" />
            <span className="font-medium">Warning: {preview.wouldOverwrite.length} files would be overwritten</span>
          </div>
          <div className="text-xs text-slate-400 max-h-20 overflow-auto">
            {preview.wouldOverwrite.slice(0, 5).map((name, i) => (
              <div key={i}>{name}</div>
            ))}
            {preview.wouldOverwrite.length > 5 && (
              <div>...and {preview.wouldOverwrite.length - 5} more</div>
            )}
          </div>
        </div>
      )}

      <div className="max-h-64 overflow-auto space-y-2 mb-4">
        {Object.entries(folderGroups).slice(0, 10).map(([folder, ops]) => (
          <div key={folder} className="bg-slate-700/30 rounded-lg p-3">
            <div className="flex items-center gap-2 text-slate-300 text-sm font-medium mb-2">
              <FolderOpen className="w-4 h-4" />
              {folder}
            </div>
            <div className="space-y-1">
              {ops.slice(0, 3).map((op, i) => (
                <div key={i} className="flex items-center gap-2 text-xs text-slate-500">
                  <span className="truncate flex-1">{op.source.split(/[/\\]/).pop()}</span>
                  <ArrowRight className="w-3 h-3" />
                  <span className="truncate flex-1">{op.destination.split(/[/\\]/).pop()}</span>
                </div>
              ))}
              {ops.length > 3 && (
                <div className="text-xs text-slate-500 italic">...and {ops.length - 3} more</div>
              )}
            </div>
          </div>
        ))}
        {Object.keys(folderGroups).length > 10 && (
          <div className="text-center text-sm text-slate-500">
            ...and {Object.keys(folderGroups).length - 10} more folders
          </div>
        )}
      </div>

      <div className="flex items-center justify-end gap-3">
        <button
          onClick={onCancel}
          disabled={isOrganizing}
          className="px-4 py-2 text-slate-400 hover:text-white transition-colors"
        >
          Cancel
        </button>
        <button
          onClick={onConfirm}
          disabled={isOrganizing}
          className="flex items-center gap-2 px-4 py-2 bg-cura-600 hover:bg-cura-700 disabled:opacity-50 text-white rounded-lg font-medium transition-colors"
        >
          {isOrganizing ? (
            <RefreshCw className="w-4 h-4 animate-spin" />
          ) : (
            <Play className="w-4 h-4" />
          )}
          {isOrganizing ? 'Organizing...' : `Confirm ${copyInsteadOfMove ? 'Copy' : 'Move'}`}
        </button>
      </div>
    </div>
  );
}

function ResultPanel({ result, onReset }: { result: { filesMoved: number; filesCopied: number; errors: string[] }; onReset: () => void }) {
  const hasErrors = result.errors.length > 0;

  return (
    <div className="bg-slate-800 border border-slate-700 rounded-lg p-4">
      <h3 className="text-lg font-semibold text-white mb-4 flex items-center gap-2">
        <CheckCircle className="w-5 h-5 text-green-400" />
        Organization Complete
      </h3>

      <div className="space-y-3 mb-4">
        <div className="flex items-center justify-between p-3 bg-slate-700/50 rounded-lg">
          <span className="text-slate-400">Files moved:</span>
          <span className="text-white font-medium">{result.filesMoved}</span>
        </div>
        <div className="flex items-center justify-between p-3 bg-slate-700/50 rounded-lg">
          <span className="text-slate-400">Files copied:</span>
          <span className="text-white font-medium">{result.filesCopied}</span>
        </div>
        <div className="flex items-center justify-between p-3 bg-slate-700/50 rounded-lg">
          <span className="text-slate-400">Errors:</span>
          <span className={hasErrors ? 'text-red-400 font-medium' : 'text-slate-300'}>
            {result.errors.length}
          </span>
        </div>
      </div>

      {hasErrors && (
        <div className="mb-4 p-3 bg-red-500/10 border border-red-500/30 rounded-lg max-h-32 overflow-auto">
          {result.errors.map((error, i) => (
            <div key={i} className="text-xs text-red-400">{error}</div>
          ))}
        </div>
      )}

      <button
        onClick={onReset}
        className="w-full px-4 py-2 bg-slate-700 hover:bg-slate-600 text-white rounded-lg font-medium transition-colors"
      >
        Start New Organization
      </button>
    </div>
  );
}

function CuraOrganizePanel() {
  const files = useCuraMediaStore((state) => state.files);
  const {
    selectedStrategy,
    destinationPath,
    copyInsteadOfMove,
    preview,
    isPreviewing,
    isOrganizing,
    result,
    error,
    setSelectedStrategy,
    setDestinationPath,
    setCopyInsteadOfMove,
    setPreview,
    setIsPreviewing,
    setIsOrganizing,
    setResult,
    setError,
    reset,
  } = useOrganizeStore();

  const handleSelectDestination = async () => {
    try {
      const path = await selectOrganizeDestination();
      setDestinationPath(path);
    } catch (err) {
      setError('Failed to select destination folder');
    }
  };

  const handlePreview = async () => {
    if (!selectedStrategy || !destinationPath) return;
    if (files.length === 0) return;

    setIsPreviewing(true);
    setError(null);
    setPreview(null);

    try {
      console.log('[Organize] Preview request:', { filesCount: files.length, strategy: selectedStrategy, destination: destinationPath });
      const previewResult = await previewOrganize(files, selectedStrategy, destinationPath);
      console.log('[Organize] Preview result:', previewResult);
      setPreview(previewResult);
    } catch (err) {
      console.error('[Organize] Preview error:', err);
      setError(err instanceof Error ? err.message : 'Failed to generate preview');
    } finally {
      setIsPreviewing(false);
    }
  };

  const handleExecute = async () => {
    if (!selectedStrategy || !destinationPath) return;
    if (files.length === 0) return;

    setIsOrganizing(true);
    setError(null);

    try {
      const result = await executeOrganize(files, selectedStrategy, destinationPath, copyInsteadOfMove);
      setResult(result);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Organization failed');
    } finally {
      setIsOrganizing(false);
    }
  };

  const handleReset = () => {
    reset();
  };

  const canPreview = selectedStrategy && destinationPath && files.length > 0;

  return (
    <div className="h-full flex flex-col">
      {/* Header */}
      <div className="flex items-center justify-between mb-6">
        <div>
          <h2 className="text-2xl font-semibold text-white">Organize</h2>
          <p className="text-slate-400 text-sm">
            {files.length > 0
              ? `${files.length} files ready to organize`
              : 'Scan folders first to organize photos'}
          </p>
        </div>
      </div>

      {/* Error */}
      {error && (
        <div className="mb-4 p-4 bg-red-500/10 border border-red-500/30 rounded-lg flex items-center gap-3">
          <AlertCircle className="w-5 h-5 text-red-400" />
          <p className="text-red-400">{error}</p>
        </div>
      )}

      {/* Main content */}
      <div className="flex-1 overflow-auto">
        {result ? (
          <ResultPanel result={result} onReset={handleReset} />
        ) : preview ? (
          <PreviewPanel
            preview={preview}
            onConfirm={handleExecute}
            onCancel={() => setPreview(null)}
            isOrganizing={isOrganizing}
          />
        ) : (
          <div className="space-y-6">
            {/* Strategy Selection */}
            <div>
              <h3 className="text-sm font-medium text-slate-300 mb-3">Select Organization Strategy</h3>
              <div className="grid grid-cols-2 gap-3">
                {DEFAULT_STRATEGIES.map((s) => (
                  <StrategyCard
                    key={s.name}
                    name={s.name}
                    description={s.description}
                    selected={selectedStrategy?.type === s.strategy.type &&
                      selectedStrategy?.dateFormat === s.strategy.dateFormat}
                    onClick={() => {
                      setSelectedStrategy(s.strategy);
                    }}
                  />
                ))}
              </div>
            </div>

            {/* Destination Selection */}
            <div className="bg-slate-800 border border-slate-700 rounded-lg p-4">
              <h3 className="text-sm font-medium text-slate-300 mb-3">Destination Folder</h3>
              <div className="flex items-center gap-3">
                <button
                  onClick={handleSelectDestination}
                  className="flex items-center gap-2 px-4 py-2 bg-slate-700 hover:bg-slate-600 text-white rounded-lg transition-colors"
                >
                  <FolderOpen className="w-4 h-4" />
                  {destinationPath ? 'Change' : 'Select Folder'}
                </button>
                {destinationPath && (
                  <span className="text-sm text-slate-400 truncate flex-1">{destinationPath}</span>
                )}
              </div>
            </div>

            {/* Options */}
            <div className="bg-slate-800 border border-slate-700 rounded-lg p-4">
              <h3 className="text-sm font-medium text-slate-300 mb-3">Options</h3>
              <label className="flex items-center gap-3 cursor-pointer">
                <input
                  type="checkbox"
                  checked={copyInsteadOfMove}
                  onChange={(e) => setCopyInsteadOfMove(e.target.checked)}
                  className="w-4 h-4 rounded border-slate-600 bg-slate-700 text-cura-600 focus:ring-cura-500"
                />
                <span className="text-sm text-slate-300">
                  Copy files instead of moving (keep originals)
                </span>
              </label>
            </div>

            {/* Preview Button */}
            <button
              onClick={handlePreview}
              disabled={!canPreview || isPreviewing}
              className="w-full flex items-center justify-center gap-2 px-4 py-3 bg-cura-600 hover:bg-cura-700 disabled:bg-slate-700 disabled:text-slate-500 disabled:cursor-not-allowed text-white rounded-lg font-medium transition-colors"
            >
              {isPreviewing ? (
                <RefreshCw className="w-4 h-4 animate-spin" />
              ) : (
                <Eye className="w-4 h-4" />
              )}
              {isPreviewing ? 'Generating Preview...' : 'Preview Changes'}
            </button>
          </div>
        )}
      </div>
    </div>
  );
}

export default CuraOrganizePanel;
