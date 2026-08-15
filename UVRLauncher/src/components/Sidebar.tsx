import React from 'react';
import { Home, Library, Settings, Activity, Cpu, MonitorPlay, Plug } from 'lucide-react';

interface SidebarProps {
  currentView: string;
  setCurrentView: (view: string) => void;
}

export const Sidebar: React.FC<SidebarProps> = ({ currentView, setCurrentView }) => {
  const navItems = [
    { icon: <Home size={20} />, label: 'Home', id: 'home' },
    { icon: <Library size={20} />, label: 'My Games', id: 'library' },
    { icon: <MonitorPlay size={20} />, label: 'VR Profiles', id: 'profiles' },
    { icon: <Cpu size={20} />, label: 'Devices', id: 'devices' },
    { icon: <Activity size={20} />, label: 'Performance', id: 'performance' },
    { icon: <Plug size={20} />, label: 'Plugins', id: 'plugins' },
  ];

  return (
    <div className="w-64 h-full bg-surface border-r border-white/5 flex flex-col pt-8 pb-4 backdrop-blur-md">
      <div className="px-6 mb-10 flex items-center">
        <div className="w-8 h-8 rounded bg-gradient-to-tr from-cyan to-violet flex items-center justify-center mr-3 shadow-[0_0_15px_rgba(0,245,212,0.4)]">
          <span className="font-display font-bold text-obsidian text-lg">U</span>
        </div>
        <div>
          <h1 className="font-display font-bold text-xl tracking-tight text-white">UVR</h1>
          <p className="text-[10px] uppercase font-mono text-cyan tracking-widest opacity-80">Universal VR Mod</p>
        </div>
      </div>

      <nav className="flex-1 px-4 space-y-2">
        {navItems.map((item, i) => (
          <button
            key={i}
            onClick={() => setCurrentView(item.id)}
            className={`w-full flex items-center px-4 py-3 rounded-lg transition-all duration-200 ${
              currentView === item.id 
                ? 'bg-gradient-to-r from-cyan/20 to-transparent border-l-2 border-cyan text-cyan font-medium' 
                : 'text-gray-400 hover:bg-white/5 hover:text-white'
            }`}
          >
            <span className={`mr-3 ${currentView === item.id ? 'text-cyan' : ''}`}>{item.icon}</span>
            <span className="font-sans text-sm">{item.label}</span>
          </button>
        ))}
      </nav>

      <div className="px-4 mt-auto pt-4 border-t border-white/5">
        <button 
          onClick={() => setCurrentView('settings')}
          className={`w-full flex items-center px-4 py-3 rounded-lg transition-all duration-200 ${
            currentView === 'settings' 
              ? 'bg-gradient-to-r from-cyan/20 to-transparent border-l-2 border-cyan text-cyan font-medium' 
              : 'text-gray-400 hover:bg-white/5 hover:text-white'
          }`}
        >
          <Settings size={20} className="mr-3" />
          <span className="font-sans text-sm">Settings</span>
        </button>
      </div>
    </div>
  );
};
