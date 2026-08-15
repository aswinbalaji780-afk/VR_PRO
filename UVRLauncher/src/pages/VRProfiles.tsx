import React, { useState } from 'react';
import { Save, RotateCcw, Box, Eye, Focus } from 'lucide-react';
import { NXButton } from '../components/NXButton';

export const VRProfiles: React.FC = () => {
  const [ipd, setIpd] = useState(63);
  const [resolutionScale, setResolutionScale] = useState(1.2);
  const [trackingMode, setTrackingMode] = useState('6DOF');
  const [projectionMode, setProjectionMode] = useState('native');
  const [isSaved, setIsSaved] = useState(false);

  const handleReset = () => {
    setIpd(63);
    setResolutionScale(1.2);
    setTrackingMode('6DOF');
    setProjectionMode('native');
  };

  const handleSave = () => {
    setIsSaved(true);
    setTimeout(() => setIsSaved(false), 2000);
  };

  return (
    <div className="w-full h-full flex flex-col">
      <div className="flex items-center justify-between mb-8">
        <div>
          <h2 className="font-display text-3xl font-bold mb-1">VR Profiles</h2>
          <p className="text-gray-400 font-sans">Configure universal injection parameters.</p>
        </div>
        <div className="flex space-x-3">
          <NXButton variant="secondary" icon={<RotateCcw size={18} />} onClick={handleReset}>RESET</NXButton>
          <NXButton 
            icon={<Save size={18} />} 
            onClick={handleSave}
            disabled={isSaved}
          >
            {isSaved ? 'SAVED!' : 'SAVE PROFILE'}
          </NXButton>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        {/* Left Column: Visual Settings */}
        <div className="space-y-6">
          <div className="glass-panel p-6">
            <div className="flex items-center mb-6">
              <div className="w-10 h-10 rounded-full bg-cyan/10 flex items-center justify-center mr-4">
                <Eye size={20} className="text-cyan" />
              </div>
              <div>
                <h3 className="font-display text-xl font-bold">Optical Parameters</h3>
                <p className="text-sm text-gray-400">Tweak rendering for your specific headset lenses.</p>
              </div>
            </div>

            <div className="space-y-6">
              <div>
                <div className="flex justify-between mb-2">
                  <label className="text-sm font-mono text-gray-300">Interpupillary Distance (IPD)</label>
                  <span className="text-cyan font-mono text-sm">{ipd} mm</span>
                </div>
                <input 
                  type="range" 
                  min="55" max="75" 
                  value={ipd} 
                  onChange={(e) => setIpd(Number(e.target.value))}
                  className="w-full h-1 bg-white/10 rounded-lg appearance-none cursor-pointer accent-cyan" 
                />
              </div>

              <div>
                <div className="flex justify-between mb-2">
                  <label className="text-sm font-mono text-gray-300">Resolution Scale</label>
                  <span className="text-violet font-mono text-sm">{resolutionScale.toFixed(1)}x</span>
                </div>
                <input 
                  type="range" 
                  min="0.5" max="2.0" step="0.1"
                  value={resolutionScale} 
                  onChange={(e) => setResolutionScale(Number(e.target.value))}
                  className="w-full h-1 bg-white/10 rounded-lg appearance-none cursor-pointer accent-violet" 
                />
              </div>
            </div>
          </div>

          <div className="glass-panel p-6">
            <div className="flex items-center mb-6">
              <div className="w-10 h-10 rounded-full bg-amber-500/10 flex items-center justify-center mr-4">
                <Focus size={20} className="text-amber-400" />
              </div>
              <div>
                <h3 className="font-display text-xl font-bold">Depth Projection</h3>
                <p className="text-sm text-gray-400">Configure Z-Buffer 3D reconstruction.</p>
              </div>
            </div>

            <div className="grid grid-cols-2 gap-4">
              <button 
                onClick={() => setProjectionMode('native')}
                className={`p-4 rounded border flex flex-col items-center justify-center transition-colors ${
                  projectionMode === 'native' 
                    ? 'border-cyan bg-cyan/10 text-cyan shadow-[0_0_10px_rgba(0,245,212,0.2)]' 
                    : 'border-white/10 hover:border-white/30 text-gray-400'
                }`}
              >
                <span className="font-bold mb-1">Native Stereo</span>
                <span className="text-xs opacity-80">True Geometry</span>
              </button>
              <button 
                onClick={() => setProjectionMode('zbuffer')}
                className={`p-4 rounded border flex flex-col items-center justify-center transition-colors ${
                  projectionMode === 'zbuffer' 
                    ? 'border-amber-400 bg-amber-500/10 text-amber-400 shadow-[0_0_10px_rgba(251,191,36,0.2)]' 
                    : 'border-white/10 hover:border-white/30 text-gray-400'
                }`}
              >
                <span className="font-bold mb-1">Z-Buffer 3D</span>
                <span className="text-xs opacity-80">Screen-space Fake</span>
              </button>
            </div>
          </div>
        </div>

        {/* Right Column: Tracking & Input */}
        <div className="space-y-6">
          <div className="glass-panel p-6 border-violet/20">
            <div className="flex items-center mb-6">
              <div className="w-10 h-10 rounded-full bg-violet/10 flex items-center justify-center mr-4">
                <Box size={20} className="text-violet" />
              </div>
              <div>
                <h3 className="font-display text-xl font-bold text-white">Tracking Subsystem</h3>
                <p className="text-sm text-gray-400">Map headset movement to game cameras.</p>
              </div>
            </div>

            <div className="space-y-4">
              <div 
                className={`p-4 rounded border cursor-pointer transition-all ${trackingMode === '6DOF' ? 'border-violet bg-violet/10 text-white' : 'border-white/10 text-gray-400 hover:border-white/30'}`}
                onClick={() => setTrackingMode('6DOF')}
              >
                <div className="flex justify-between items-center mb-1">
                  <span className="font-bold">6 Degrees of Freedom (6DOF)</span>
                  {trackingMode === '6DOF' && <div className="w-2 h-2 rounded-full bg-violet shadow-[0_0_10px_rgba(123,44,191,0.8)]" />}
                </div>
                <p className="text-xs opacity-70">Positional and rotational tracking. Requires memory hooking.</p>
              </div>

              <div 
                className={`p-4 rounded border cursor-pointer transition-all ${trackingMode === '3DOF' ? 'border-violet bg-violet/10 text-white' : 'border-white/10 text-gray-400 hover:border-white/30'}`}
                onClick={() => setTrackingMode('3DOF')}
              >
                <div className="flex justify-between items-center mb-1">
                  <span className="font-bold">3 Degrees of Freedom (3DOF)</span>
                  {trackingMode === '3DOF' && <div className="w-2 h-2 rounded-full bg-violet shadow-[0_0_10px_rgba(123,44,191,0.8)]" />}
                </div>
                <p className="text-xs opacity-70">Rotational tracking only (mouse emulation). High compatibility.</p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
