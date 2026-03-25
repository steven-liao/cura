import { useCuraMediaStore } from '../../stores';
import { formatEta, formatProgressPercent } from '../../types/curaProgress';
import { curaScanApi } from '../../services/tauri';
import { Loader2, X } from 'lucide-react';

export function CuraScanProgress() {
  const { progress, currentScanId, resetScanState, isScanning } = useCuraMediaStore();

  if (!isScanning) return null;

  const percent = formatProgressPercent(progress);
  const phaseLabel = typeof progress.phase === 'string' ? progress.phase : 'Error';

  const handleCancel = async () => {
    if (currentScanId) {
      try {
        await curaScanApi.cancelScan(currentScanId);
      } catch (e) {
        console.error('Failed to cancel scan:', e);
      }
    }
    resetScanState();
  };

  return (
    <div className="fixed bottom-4 right-4 bg-slate-800 rounded-lg shadow-xl border border-slate-700 p-4 min-w-[320px] z-50">
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2">
          <Loader2 className="w-5 h-5 text-cura-500 animate-spin" />
          <span className="text-white font-medium">{phaseLabel}</span>
        </div>
        <button
          onClick={handleCancel}
          className="p-1 text-slate-400 hover:text-white transition-colors rounded hover:bg-slate-700"
          title="Cancel scan"
        >
          <X className="w-5 h-5" />
        </button>
      </div>

      <div className="space-y-2">
        {/* Progress bar */}
        <div className="w-full bg-slate-700 rounded-full h-2">
          <div
            className="bg-cura-500 h-2 rounded-full transition-all duration-300"
            style={{ width: `${percent}%` }}
          />
        </div>

        {/* Stats */}
        <div className="flex justify-between text-sm">
          <span className="text-slate-400">
            {progress.current} / {progress.total} files
          </span>
          <span className="text-slate-400">
            {percent}%
          </span>
        </div>

        {/* Current file and ETA */}
        {progress.currentFile && (
          <div className="flex justify-between text-xs text-slate-500">
            <span className="truncate max-w-[200px]" title={progress.currentFile}>
              {progress.currentFile}
            </span>
            {progress.etaSeconds && progress.etaSeconds > 0 && (
              <span>ETA: {formatEta(progress.etaSeconds)}</span>
            )}
          </div>
        )}

        {/* Speed */}
        {progress.speed && progress.speed > 0 && (
          <div className="text-xs text-slate-500">
            {progress.speed.toFixed(1)} files/sec
          </div>
        )}
      </div>
    </div>
  );
}