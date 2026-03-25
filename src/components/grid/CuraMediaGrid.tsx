import { useRef, useCallback, useEffect, useState } from 'react';
import { useVirtualizer } from '@tanstack/react-virtual';
import { CuraMediaFile, formatFileSize, isImage } from '../../types/curaMediaFile';
import { useCuraMediaStore } from '../../stores';
import { Check, Film, Image } from 'lucide-react';

interface CuraMediaGridProps {
  onFileClick?: (file: CuraMediaFile) => void;
  columnWidth?: number;
  rowHeight?: number;
  gap?: number;
}

export function CuraMediaGrid({
  onFileClick,
  columnWidth = 180,
  rowHeight = 180,
  gap = 12,
}: CuraMediaGridProps) {
  const { files, selectedFiles, toggleSelection } = useCuraMediaStore();
  const parentRef = useRef<HTMLDivElement>(null);
  const [columns, setColumns] = useState(5);

  // Calculate columns based on container width
  useEffect(() => {
    const updateColumns = () => {
      if (parentRef.current) {
        const containerWidth = parentRef.current.clientWidth;
        const newColumns = Math.max(1, Math.floor((containerWidth + gap) / (columnWidth + gap)));
        setColumns(newColumns);
      }
    };

    updateColumns();
    window.addEventListener('resize', updateColumns);
    return () => window.removeEventListener('resize', updateColumns);
  }, [columnWidth, gap]);

  const rowCount = Math.ceil(files.length / columns);

  const rowVirtualizer = useVirtualizer({
    count: rowCount,
    getScrollElement: () => parentRef.current,
    estimateSize: () => rowHeight + gap,
    overscan: 3,
  });

  const handleFileClick = useCallback((file: CuraMediaFile, event: React.MouseEvent) => {
    if (event.ctrlKey || event.metaKey) {
      toggleSelection(file.id);
    } else if (event.shiftKey && selectedFiles.size > 0) {
      // Range selection
      const lastSelectedId = Array.from(selectedFiles).pop();
      const lastIndex = files.findIndex(f => f.id === lastSelectedId);
      const currentIndex = files.findIndex(f => f.id === file.id);
      const start = Math.min(lastIndex, currentIndex);
      const end = Math.max(lastIndex, currentIndex);
      for (let i = start; i <= end; i++) {
        if (!selectedFiles.has(files[i].id)) {
          toggleSelection(files[i].id);
        }
      }
    } else {
      onFileClick?.(file);
    }
  }, [toggleSelection, selectedFiles, files, onFileClick]);

  if (files.length === 0) {
    return (
      <div className="flex flex-col items-center justify-center h-full text-slate-500">
        <Image className="w-16 h-16 mb-4 opacity-50" />
        <p className="text-lg">No files scanned yet</p>
        <p className="text-sm mt-2">Select folders and click "Start Scan"</p>
      </div>
    );
  }

  return (
    <div
      ref={parentRef}
      className="h-full overflow-auto"
      style={{ contain: 'strict' }}
    >
      <div
        className="relative w-full p-3"
        style={{ height: `${rowVirtualizer.getTotalSize()}px` }}
      >
        {rowVirtualizer.getVirtualItems().map((virtualRow) => {
          const startIndex = virtualRow.index * columns;
          const rowFiles = files.slice(startIndex, startIndex + columns);

          return (
            <div
              key={virtualRow.key}
              className="absolute flex"
              style={{
                transform: `translateY(${virtualRow.start}px)`,
                height: `${rowHeight}px`,
                paddingLeft: `${gap / 2}px`,
              }}
            >
              {rowFiles.map((file) => {
                const isSelected = selectedFiles.has(file.id);
                return (
                  <div
                    key={file.id}
                    className={`relative group cursor-pointer rounded-lg overflow-hidden border-2 transition-all flex-shrink-0`}
                    style={{
                      width: `${columnWidth}px`,
                      height: `${rowHeight}px`,
                      marginRight: `${gap}px`,
                      borderColor: isSelected ? 'rgb(14, 165, 233)' : 'transparent',
                    }}
                    onClick={(e) => handleFileClick(file, e)}
                  >
                    {/* Thumbnail placeholder */}
                    <div className="w-full h-full bg-slate-700 flex items-center justify-center">
                      {isImage(file.extension) ? (
                        <Image className="w-12 h-12 text-slate-500" />
                      ) : (
                        <Film className="w-12 h-12 text-slate-500" />
                      )}
                    </div>

                    {/* Selection indicator */}
                    <div
                      className={`absolute top-2 right-2 w-6 h-6 rounded-full flex items-center justify-center transition-opacity ${
                        isSelected ? 'bg-cura-500 opacity-100' : 'bg-slate-900/70 opacity-0 group-hover:opacity-100'
                      }`}
                    >
                      {isSelected && <Check className="w-4 h-4 text-white" />}
                    </div>

                    {/* File info overlay */}
                    <div className="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-black/80 to-transparent p-2">
                      <p className="text-white text-sm truncate" title={file.fileName}>
                        {file.fileName}
                      </p>
                      <p className="text-slate-400 text-xs">{formatFileSize(file.fileSize)}</p>
                    </div>
                  </div>
                );
              })}
            </div>
          );
        })}
      </div>
    </div>
  );
}