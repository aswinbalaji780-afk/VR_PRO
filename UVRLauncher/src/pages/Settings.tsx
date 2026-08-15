import React, { useState } from 'react';
import { Shield, HardDrive, Zap, Monitor, Save } from 'lucide-react';
import { NXButton } from '../components/NXButton';

export const Settings: React.FC = () => {
  const [autoInject, setAutoInject] = useState(false);
  const [hardwareAccel, setHardwareAccel] = useState(true);
  const [isSaved, setIsSaved] = useState(false);

  const handleSave = () => {
    setIsSaved(true);
    setTimeout(() => setIsSaved(false), 2000);
  };

  return (
    <div className="w-full h-full flex flex-col">
      <div className="flex items-center justify-between mb-8">
        <div>
          <h2 className="font-display text-3xl font-bold mb-1">System Settings</h2>
          <p className="text-gray-400 font-sans">Global preferences for the UVR runtime.</p>
        </div>
        <NXButton 
          icon={<Save size={18} />} 
          onClick={handleSave}
          disabled={isSaved}
        >
          {isSaved ? 'APPLIED!' : 'APPLY CHANGES'}
        </NXButton>
      </div>

      <div className="max-w-3xl space-y-4">
        {/* General Settings */}
        <div className="glass-panel p-6 flex items-center justify-between">
          <div className="flex items-center">
            <div className="w-12 h-12 rounded-lg bg-white/5 flex items-center justify-center mr-6 border border-white/10">
              <Zap size={24} className="text-cyan" />
            </div>
            <div>
              <h3 className="font-bold text-lg">Auto-Inject on Game Launch</h3>
              <p className="text-sm text-gray-400">Automatically detect compatible games and inject the VRModFramework DLL.</p>
            </div>
          </div>
          
          <button 
            onClick={() => setAutoInject(!autoInject)}
            className={`w-14 h-8 rounded-full transition-colors relative ${autoInject ? 'bg-cyan' : 'bg-gray-600'}`}
          >
            <div className={`absolute top-1 left-1 w-6 h-6 rounded-full bg-white transition-transform ${autoInject ? 'translate-x-6' : ''}`} />
          </button>
        </div>

        <div className="glass-panel p-6 flex items-center justify-between">
          <div className="flex items-center">
            <div className="w-12 h-12 rounded-lg bg-white/5 flex items-center justify-center mr-6 border border-white/10">
              <Monitor size={24} className="text-violet" />
            </div>
            <div>
              <h3 className="font-bold text-lg">Hardware Acceleration</h3>
              <p className="text-sm text-gray-400">Use GPU to render the Desktop Shell UI for maximum performance.</p>
            </div>
          </div>
          
          <button 
            onClick={() => setHardwareAccel(!hardwareAccel)}
            className={`w-14 h-8 rounded-full transition-colors relative ${hardwareAccel ? 'bg-violet shadow-[0_0_15px_rgba(123,44,191,0.5)]' : 'bg-gray-600'}`}
          >
            <div className={`absolute top-1 left-1 w-6 h-6 rounded-full bg-white transition-transform ${hardwareAccel ? 'translate-x-6' : ''}`} />
          </button>
        </div>

        <div className="glass-panel p-6 flex items-center justify-between">
          <div className="flex items-center">
            <div className="w-12 h-12 rounded-lg bg-white/5 flex items-center justify-center mr-6 border border-white/10">
              <HardDrive size={24} className="text-gray-300" />
            </div>
            <div>
              <h3 className="font-bold text-lg">OpenXR Runtime</h3>
              <p className="text-sm text-gray-400">Select the default API used for rendering to the headset.</p>
            </div>
          </div>
          
          <select className="bg-obsidian border border-white/20 text-white text-sm rounded-lg focus:ring-cyan focus:border-cyan block p-2.5 outline-none font-mono">
            <option>System Default</option>
            <option>SteamVR (OpenVR)</option>
            <option>Oculus Runtime</option>
            <option>Windows Mixed Reality</option>
          </select>
        </div>

        <div className="glass-panel p-6 flex items-center justify-between opacity-50">
          <div className="flex items-center">
            <div className="w-12 h-12 rounded-lg bg-white/5 flex items-center justify-center mr-6 border border-white/10">
              <Shield size={24} className="text-red-400" />
            </div>
            <div>
              <h3 className="font-bold text-lg text-red-400">Anti-Cheat Bypass (Experimental)</h3>
              <p className="text-sm text-gray-400">Attempt to mask injection signatures. High risk of account bans.</p>
            </div>
          </div>
          
          <button disabled className="w-14 h-8 rounded-full bg-gray-700 relative cursor-not-allowed">
            <div className="absolute top-1 left-1 w-6 h-6 rounded-full bg-gray-500" />
          </button>
        </div>
        
      </div>
    </div>
  );
};
