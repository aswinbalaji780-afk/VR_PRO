import React, { useState } from 'react';
import { Package, Download, RefreshCw, Power } from 'lucide-react';
import { NXButton } from '../components/NXButton';

export const Plugins: React.FC = () => {
  const [plugins, setPlugins] = useState([
    { id: 1, name: 'OpenXR Toolkit', author: 'mbucchia', version: '1.3.2', status: 'Active', description: 'Advanced rendering options like Fixed Foveated Rendering (FFR) and NIS scaling.' },
    { id: 2, name: 'ReShade VR', author: 'crosire', version: '5.9.2', status: 'Inactive', description: 'Post-processing injector for advanced color correction and depth-of-field in VR.' },
    { id: 3, name: 'Cyberpunk Native UI', author: 'NEXUS_Community', version: '0.8.4', status: 'Active', description: 'Projects the 2D Cyberpunk UI into 3D world space attached to the player\'s wrist.' },
  ]);

  const togglePlugin = (id: number) => {
    setPlugins(prev => prev.map(p => 
      p.id === id ? { ...p, status: p.status === 'Active' ? 'Inactive' : 'Active' } : p
    ));
  };

  const installPlugin = () => {
    const newId = plugins.length + 1;
    setPlugins([...plugins, {
      id: newId,
      name: 'Skyrim VR Engine Fixes',
      author: 'aers',
      version: '1.2.1',
      status: 'Active',
      description: 'Critical engine-level bug fixes and performance patches for Skyrim VR.'
    }]);
  };

  return (
    <div className="w-full h-full flex flex-col">
      <div className="flex items-center justify-between mb-8">
        <div>
          <h2 className="font-display text-3xl font-bold mb-1">Plugin Manager</h2>
          <p className="text-gray-400 font-sans">Extend the UVR runtime with community mods and utilities.</p>
        </div>
        <NXButton icon={<Download size={18} />} onClick={installPlugin}>INSTALL PLUGIN</NXButton>
      </div>

      <div className="flex-1 space-y-4">
        {plugins.map((plugin) => (
          <div key={plugin.id} className="glass-panel p-6 flex flex-col md:flex-row md:items-center justify-between transition-colors hover:border-white/20">
            <div className="flex items-start md:items-center mb-4 md:mb-0">
              <div className={`w-12 h-12 rounded-xl flex items-center justify-center mr-6 border ${
                plugin.status === 'Active' ? 'bg-cyan/10 border-cyan/30' : 'bg-white/5 border-white/10'
              }`}>
                <Package size={24} className={plugin.status === 'Active' ? 'text-cyan' : 'text-gray-500'} />
              </div>
              
              <div>
                <div className="flex items-center mb-1">
                  <h3 className="font-bold text-lg text-white mr-3">{plugin.name}</h3>
                  <span className={`px-2 py-0.5 rounded text-[10px] font-mono font-bold border ${
                    plugin.status === 'Active' ? 'bg-cyan/20 border-cyan/50 text-cyan' : 'bg-gray-800 border-gray-600 text-gray-400'
                  }`}>
                    {plugin.status.toUpperCase()}
                  </span>
                </div>
                <p className="text-sm text-gray-400 mb-1">{plugin.description}</p>
                <div className="flex items-center text-xs font-mono text-gray-500">
                  <span className="mr-4">Author: <span className="text-gray-300">{plugin.author}</span></span>
                  <span>Version: <span className="text-gray-300">v{plugin.version}</span></span>
                </div>
              </div>
            </div>

            <div className="flex items-center space-x-3">
              <button className="p-2 rounded bg-white/5 hover:bg-white/10 border border-white/10 transition-colors text-gray-300 hover:text-white">
                <RefreshCw size={18} />
              </button>
              <button 
                onClick={() => togglePlugin(plugin.id)}
                className={`flex items-center px-4 py-2 rounded font-bold font-sans text-sm transition-colors ${
                  plugin.status === 'Active' 
                    ? 'bg-white/10 hover:bg-red-500/20 text-white hover:text-red-400 border border-white/10 hover:border-red-500/50' 
                    : 'bg-cyan/20 hover:bg-cyan text-cyan hover:text-obsidian border border-cyan/30 hover:border-cyan shadow-[0_0_10px_rgba(0,245,212,0.2)] hover:shadow-[0_0_15px_rgba(0,245,212,0.6)]'
                }`}
              >
                <Power size={16} className="mr-2" />
                {plugin.status === 'Active' ? 'DISABLE' : 'ENABLE'}
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};
