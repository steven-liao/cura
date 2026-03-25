import { useEffect, useState, useRef } from 'react';
import CuraSourceSelector from '../sources/CuraSourceSelector';
import { CuraMediaGrid } from '../grid/CuraMediaGrid';
import { CuraScanProgress } from '../progress/CuraScanProgress';
import { useCuraMediaStore } from '../../stores';
import { curaScanApi } from '../../services/tauri';
import { UnlistenFn } from '@tauri-apps/api/event';
import { Play, RotateCcw } from 'lucide-react';
import { formatFileSize } from '../../types/curaMediaFile';

export function CuraScanPanel() {
  const {
    sourceFolders,
    isScanning,
    files,
    setFiles,
    setIsScanning,
    setProgress,
    setCurrentScanId,
    setError,
    resetScanState,
  } = useCuraMediaStore();

  const [isStarting, setIsStarting] = useState(false);
  const unlistenRef = useRef<UnlistenFn | null>(null);

  // Subscribe to scan progress events - only once on mount
  useEffect(() => {
    const setupListener = async () => {
      console.log('[CuraScanPanel] Setting up progress listener');
      unlistenRef.current = await curaScanApi.onScanProgress((progressUpdate) => {
        console.log('[CuraScanPanel] Received progress:', progressUpdate);
        setProgress(progressUpdate);

        if (progressUpdate.phase === 'Complete' || progressUpdate.phase === 'Cancelled') {
          setIsScanning(false);
        }

        if (typeof progressUpdate.phase === 'object' && 'Error' in progressUpdate.phase) {
          setError(progressUpdate.phase.Error);
          setIsScanning(false);
        }
      });
      console.log('[CuraScanPanel] Listener set up');
    };

    setupListener();

    return () => {
      if (unlistenRef.current) {
        console.log('[CuraScanPanel] Cleaning up listener');
        unlistenRef.current();
      }
    };
  }, []); // Empty deps - only run once on mount

  const handleStartScan = async () => {
    if (sourceFolders.length === 0) return;

    setIsStarting(true);
    setIsScanning(true);
    setProgress({ phase: 'Initializing', current: 0, total: 0, currentFile: undefined, speed: undefined, etaSeconds: undefined });

    try {
      console.log('[CuraScanPanel] Starting scan for:', sourceFolders);
      const result = await curaScanApi.startScan(sourceFolders);
      console.log('[CuraScanPanel] Scan result:', result);
      setCurrentScanId(result.scanId);
      setFiles(result.files);
      // Don't set isScanning false here - let the Complete event do it
    } catch (error) {
      const errorMsg = error instanceof Error ? error.message : 'Scan failed';
      console.error('[CuraScanPanel] Scan error:', errorMsg);
      if (errorMsg !== 'Scan cancelled') {
        setError(errorMsg);
      }
      setIsScanning(false);
    } finally {
      setIsStarting(false);
    }
  };

  const handleReset = () => {
    resetScanState();
    setFiles([]);
  };

  const totalSize = files.reduce((acc, f) => acc + f.fileSize, 0);
  const hashedCount = files.filter(f => f.md5Hash).length;

  return (
    <div className="flex flex-col h-full">
      {/* Header with actions */}
      <div className="flex items-center justify-between mb-4">
        <h2 className="text-xl font-semibold text-white">Media Library</h2>
        <div className="flex gap-2">
          {files.length > 0 && (
            <button
              onClick={handleReset}
              disabled={isScanning}
              className="flex items-center gap-2 px-4 py-2 bg-slate-700 hover:bg-slate-600 disabled:bg-slate-800 disabled:text-slate-500 text-white rounded-lg transition-colors"
            >
              <RotateCcw className="w-4 h-4" />
              Reset
            </button>
          )}
          <button
            onClick={handleStartScan}
            disabled={isScanning || sourceFolders.length === 0 || isStarting}
            className="flex items-center gap-2 px-4 py-2 bg-cura-600 hover:bg-cura-700 disabled:bg-slate-600 disabled:cursor-not-allowed text-white rounded-lg transition-colors"
          >
            <Play className="w-4 h-4" />
            {isStarting ? 'Starting...' : isScanning ? 'Scanning...' : 'Start Scan'}
          </button>
        </div>
      </div>

      {/* Source folders selector */}
      <div className="mb-4">
        <CuraSourceSelector />
      </div>

      {/* Stats bar */}
      {files.length > 0 && (
        <div className="flex gap-6 mb-4 text-sm text-slate-400 bg-slate-700/50 rounded-lg px-4 py-2">
          <span><strong className="text-white">{files.length.toLocaleString()}</strong> files</span>
          <span><strong className="text-white">{formatFileSize(totalSize)}</strong> total</span>
          <span><strong className="text-white">{hashedCount.toLocaleString()}</strong> hashed</span>
        </div>
      )}

      {/* Media grid */}
      <div className="flex-1 min-h-0 bg-slate-800/50 rounded-lg">
        <CuraMediaGrid />
      </div>

      {/* Progress overlay */}
      <CuraScanProgress />
    </div>
  );
}