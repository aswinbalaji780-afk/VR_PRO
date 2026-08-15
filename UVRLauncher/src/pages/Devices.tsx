import React, { useState } from 'react';
import { Headphones, Gamepad2, Battery, BatteryMedium, Cpu, Wifi } from 'lucide-react';
import { NXButton } from '../components/NXButton';

export const Devices: React.FC = () => {
  const [isPairing, setIsPairing] = useState(false);

  const handlePair = () => {
    setIsPairing(true);
    setTimeout(() => setIsPairing(false), 3000);
  };
  return (
    <div className="w-full h-full flex flex-col">
      <div className="flex items-center justify-between mb-8">
        <div>
          <h2 className="font-display text-3xl font-bold mb-1">Hardware Devices</h2>
          <p className="text-gray-400 font-sans">Manage connected OpenXR headsets and spatial controllers.</p>
        </div>
        <NXButton 
          icon={<Wifi size={18} className={isPairing ? 'animate-pulse' : ''} />}
          onClick={handlePair}
          disabled={isPairing}
        >
          {isPairing ? 'SEARCHING FOR DEVICES...' : 'PAIR NEW DEVICE'}
        </NXButton>
      </div>

      <div className="grid grid-cols-1 xl:grid-cols-2 gap-8">
        {/* Headset Card */}
        <div className="glass-panel p-8 relative overflow-hidden group">
          <div className="absolute -right-20 -top-20 w-64 h-64 bg-cyan/10 rounded-full blur-[80px] pointer-events-none group-hover:bg-cyan/20 transition-colors duration-500" />
          
          <div className="flex justify-between items-start mb-8 relative z-10">
            <div className="flex items-center">
              <div className="w-16 h-16 rounded-2xl bg-cyan/10 border border-cyan/30 flex items-center justify-center mr-6 shadow-[0_0_15px_rgba(0,245,212,0.2)]">
                <Headphones size={32} className="text-cyan" />
              </div>
              <div>
                <h3 className="font-display text-2xl font-bold text-white">Meta Quest 3</h3>
                <div className="flex items-center mt-1">
                  <div className="w-2 h-2 rounded-full bg-cyan glow-cyan mr-2" />
                  <span className="text-sm font-mono text-cyan">CONNECTED (LINK CABLE)</span>
                </div>
              </div>
            </div>
            <div className="flex items-center bg-white/5 px-3 py-1.5 rounded-lg border border-white/10">
              <Battery className="text-green-400 mr-2" size={16} />
              <span className="font-mono text-sm text-green-400">100%</span>
            </div>
          </div>

          <div className="grid grid-cols-2 gap-4 relative z-10">
            <div className="bg-white/5 p-4 rounded-xl border border-white/5">
              <p className="text-xs text-gray-400 font-mono mb-1 uppercase">Refresh Rate</p>
              <p className="text-lg font-bold text-white">120 Hz</p>
            </div>
            <div className="bg-white/5 p-4 rounded-xl border border-white/5">
              <p className="text-xs text-gray-400 font-mono mb-1 uppercase">Render Target</p>
              <p className="text-lg font-bold text-white">4128 x 2208</p>
            </div>
            <div className="bg-white/5 p-4 rounded-xl border border-white/5">
              <p className="text-xs text-gray-400 font-mono mb-1 uppercase">Firmware</p>
              <p className="text-lg font-bold text-white">v62.0.0</p>
            </div>
            <div className="bg-white/5 p-4 rounded-xl border border-white/5">
              <p className="text-xs text-gray-400 font-mono mb-1 uppercase">Tracking API</p>
              <p className="text-lg font-bold text-white">Oculus OVR</p>
            </div>
          </div>
        </div>

        {/* Controllers */}
        <div className="space-y-6">
          <div className="glass-panel p-6 flex items-center justify-between border border-transparent hover:border-violet/30 transition-colors">
            <div className="flex items-center">
              <div className="w-12 h-12 rounded-xl bg-violet/10 flex items-center justify-center mr-4">
                <Gamepad2 size={24} className="text-violet" />
              </div>
              <div>
                <h4 className="font-bold text-lg text-white">Left Controller</h4>
                <p className="text-xs text-gray-400 font-mono">Touch Plus (Active)</p>
              </div>
            </div>
            <div className="flex items-center">
              <span className="font-mono text-sm mr-3">82%</span>
              <BatteryMedium size={18} className="text-white" />
            </div>
          </div>

          <div className="glass-panel p-6 flex items-center justify-between border border-transparent hover:border-violet/30 transition-colors">
            <div className="flex items-center">
              <div className="w-12 h-12 rounded-xl bg-violet/10 flex items-center justify-center mr-4">
                <Gamepad2 size={24} className="text-violet" />
              </div>
              <div>
                <h4 className="font-bold text-lg text-white">Right Controller</h4>
                <p className="text-xs text-gray-400 font-mono">Touch Plus (Active)</p>
              </div>
            </div>
            <div className="flex items-center">
              <span className="font-mono text-sm mr-3">78%</span>
              <BatteryMedium size={18} className="text-white" />
            </div>
          </div>
          
          <div className="glass-panel p-6 flex items-center justify-between opacity-50 border border-transparent border-dashed">
            <div className="flex items-center">
              <div className="w-12 h-12 rounded-xl bg-white/5 flex items-center justify-center mr-4">
                <Cpu size={24} className="text-gray-500" />
              </div>
              <div>
                <h4 className="font-bold text-lg text-gray-300">Body Trackers</h4>
                <p className="text-xs text-gray-500 font-mono">No Vive Trackers Detected</p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
