import React from 'react';
import { Activity, BarChart3, AlertTriangle } from 'lucide-react';

export const Performance: React.FC = () => {
  return (
    <div className="w-full h-full flex flex-col">
      <div className="flex items-center justify-between mb-8">
        <div>
          <h2 className="font-display text-3xl font-bold mb-1">Performance Telemetry</h2>
          <p className="text-gray-400 font-sans">Live rendering metrics from the VRModFramework injector.</p>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-6">
        <div className="glass-panel p-6">
          <div className="flex justify-between items-center mb-4">
            <h3 className="font-mono text-sm text-gray-400 uppercase tracking-widest">App Framerate</h3>
            <Activity size={18} className="text-cyan" />
          </div>
          <p className="font-display text-4xl font-bold text-white mb-1">118 <span className="text-xl text-gray-500">FPS</span></p>
          <p className="text-sm text-cyan font-mono">+12% from baseline</p>
        </div>

        <div className="glass-panel p-6">
          <div className="flex justify-between items-center mb-4">
            <h3 className="font-mono text-sm text-gray-400 uppercase tracking-widest">Frame Time</h3>
            <BarChart3 size={18} className="text-violet" />
          </div>
          <p className="font-display text-4xl font-bold text-white mb-1">8.4 <span className="text-xl text-gray-500">ms</span></p>
          <p className="text-sm text-violet font-mono">Stable rendering</p>
        </div>

        <div className="glass-panel p-6 border-amber-500/20 bg-amber-500/5">
          <div className="flex justify-between items-center mb-4">
            <h3 className="font-mono text-sm text-amber-500 uppercase tracking-widest">Reprojection</h3>
            <AlertTriangle size={18} className="text-amber-500" />
          </div>
          <p className="font-display text-4xl font-bold text-amber-400 mb-1">1.2 <span className="text-xl text-amber-500/50">%</span></p>
          <p className="text-sm text-amber-500/70 font-mono">Minor frame drops detected</p>
        </div>
      </div>

      <div className="glass-panel flex-1 p-6 relative overflow-hidden flex flex-col">
        <div className="flex justify-between items-center mb-6 z-10 relative">
          <h3 className="font-bold text-lg">Frame Time Graph (Last 60s)</h3>
          <div className="flex items-center space-x-2">
            <span className="w-3 h-3 rounded-full bg-cyan"></span>
            <span className="text-xs font-mono text-gray-400">Game Thread</span>
            <span className="w-3 h-3 rounded-full bg-violet ml-4"></span>
            <span className="text-xs font-mono text-gray-400">Render Thread</span>
          </div>
        </div>
        
        {/* Mock Graph using pure CSS gradients for the "Liquid Spatial" look */}
        <div className="flex-1 w-full relative z-10 flex items-end">
          <div className="absolute inset-0 bg-[linear-gradient(rgba(255,255,255,0.05)_1px,transparent_1px),linear-gradient(90deg,rgba(255,255,255,0.05)_1px,transparent_1px)] bg-[size:40px_40px] [mask-image:linear-gradient(to_bottom,transparent,black)] pointer-events-none" />
          
          <div className="w-full h-full relative">
            {/* The actual graph line mock */}
            <svg viewBox="0 0 1000 300" className="absolute inset-0 w-full h-full preserve-3d" preserveAspectRatio="none">
              <path 
                d="M0,250 C100,240 150,260 200,245 C300,230 350,280 400,220 C500,180 550,230 600,210 C700,200 750,150 800,190 C900,220 950,160 1000,180" 
                fill="none" stroke="#00F5D4" strokeWidth="4" 
                style={{ filter: 'drop-shadow(0px 10px 10px rgba(0,245,212,0.4))' }}
              />
              <path 
                d="M0,280 C100,270 150,290 200,275 C300,260 350,290 400,250 C500,220 550,270 600,240 C700,230 750,190 800,220 C900,250 950,200 1000,210" 
                fill="none" stroke="#7B2CBF" strokeWidth="4"
                style={{ filter: 'drop-shadow(0px 10px 10px rgba(123,44,191,0.4))' }}
              />
            </svg>
          </div>
        </div>
      </div>
    </div>
  );
};
