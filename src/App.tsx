import { useState, useEffect } from 'react';
import CuraMainLayout from './components/layout/CuraMainLayout';

function App() {
  const [isReady, setIsReady] = useState(false);

  useEffect(() => {
    // Initialize app
    setIsReady(true);
  }, []);

  if (!isReady) {
    return (
      <div className="flex items-center justify-center h-screen bg-slate-900">
        <div className="text-center">
          <div className="w-12 h-12 border-4 border-cura-500 border-t-transparent rounded-full animate-spin mx-auto mb-4"></div>
          <p className="text-slate-400">Loading Cura...</p>
        </div>
      </div>
    );
  }

  return <CuraMainLayout />;
}

export default App;