import { useState, useEffect } from 'react';
import { Sidebar } from './components/Sidebar';
import { NXButton } from './components/NXButton';
import { MyGames } from './pages/MyGames';
import { VRProfiles } from './pages/VRProfiles';
import { Settings } from './pages/Settings';
import { Devices } from './pages/Devices';
import { Performance } from './pages/Performance';
import { Plugins } from './pages/Plugins';
import { Play, Activity, Cpu, Headphones } from 'lucide-react';

type XRState = 'IDLE' | 'DETECTING' | 'READY' | 'INITIALIZING' | 'TRACKING_LOCKED' | 'VR_READY';

export default function App() {
  const [xrState, setXrState] = useState<XRState>('IDLE');
  const [currentView, setCurrentView] = useState('home');

  // Hoist games state and load from localStorage
  const [games, setGames] = useState(() => {
    const savedGames = localStorage.getItem('uvr_games');
    if (savedGames) {
      try {
        return JSON.parse(savedGames);
      } catch (e) {
        console.error('Failed to parse games from memory', e);
      }
    }
    // Default fallback if no memory exists
    return [
      { id: 1, title: 'Cyberpunk 2077', exeName: 'Cyberpunk2077.exe', status: 'Ready', image: 'https://images.unsplash.com/photo-1605806616949-1e87b487cb2a?q=80&w=600&auto=format&fit=crop' },
      { id: 2, title: 'Elden Ring', exeName: 'eldenring.exe', status: 'Needs Profile', image: 'https://images.unsplash.com/photo-1600329065609-b7fb3f28dcf3?q=80&w=600&auto=format&fit=crop' },
      { id: 3, title: 'Hogwarts Legacy', exeName: 'HogwartsLegacy.exe', status: 'Injected', image: 'https://images.unsplash.com/photo-1518709268805-4e9042af9f23?q=80&w=600&auto=format&fit=crop' },
      { id: 4, title: 'Red Dead Redemption 2', exeName: 'RDR2.exe', status: 'Ready', image: 'https://images.unsplash.com/photo-1542385262-cdf06b2bbe72?q=80&w=600&auto=format&fit=crop' },
    ];
  });

  // Sync to localStorage whenever games change
  useEffect(() => {
    localStorage.setItem('uvr_games', JSON.stringify(games));
  }, [games]);

  const handleLaunchVR = async () => {
    if (xrState === 'READY' || xrState === 'IDLE') {
      setXrState('INITIALIZING');
      
      try {
        // Execute real backend injection via Electron
        const activeGame = games.length > 0 ? games[0] : null;
        const targetExe = activeGame ? activeGame.exeName : 'Cyberpunk2077.exe';
        const response = await window.electronAPI.launchInjector(targetExe);
        console.log('Backend response:', response);
        setXrState('TRACKING_LOCKED');
        setTimeout(() => setXrState('VR_READY'), 1000); // Small transition
      } catch (e: any) {
        console.error('Failed to launch VR:', e);
        alert(e.message || 'Injection failed');
        setXrState('READY'); // Reset on failure
      }
    }
  };

  return (
    <div className="flex w-full h-full bg-obsidian overflow-hidden text-gray-100">
      <Sidebar currentView={currentView} setCurrentView={setCurrentView} />
      
      <main className="flex-1 flex flex-col relative overflow-y-auto">
        {/* Topbar */}
        <header className="h-16 flex items-center justify-between px-8 border-b border-white/5 bg-surface/50 backdrop-blur-md sticky top-0 z-20">
          <div className="font-mono text-xs text-gray-400 tracking-wider">
            NEXUS XR // CORE v1.0.0
          </div>
          <div className="flex items-center space-x-4">
            <div className={`px-3 py-1 rounded-full text-xs font-mono border flex items-center ${
              xrState === 'IDLE' ? 'border-gray-600 text-gray-400' :
              xrState === 'DETECTING' ? 'border-amber-500/50 text-amber-400 bg-amber-500/10' :
              'border-cyan/50 text-cyan bg-cyan/10'
            }`}>
              <div className={`w-2 h-2 rounded-full mr-2 ${
                xrState === 'IDLE' ? 'bg-gray-500' :
                xrState === 'DETECTING' ? 'bg-amber-400 animate-pulse' :
                'bg-cyan glow-cyan'
              }`} />
              {xrState === 'IDLE' ? 'NO HEADSET' :
               xrState === 'DETECTING' ? 'DETECTING...' :
               xrState === 'VR_READY' ? 'TRACKING ACTIVE' : 'OPENXR READY'}
            </div>
          </div>
        </header>

        {/* Dashboard Content */}
        <div className="flex-1 p-8">
          {currentView === 'home' && (
            <>
          {/* Hero Section */}
          <div className="relative glass-panel overflow-hidden mb-8 p-10 flex items-center justify-between min-h-[320px]">
            {/* Background Effects */}
            <div className="absolute -right-20 -top-20 w-96 h-96 bg-cyan/20 rounded-full blur-[100px] pointer-events-none" />
            <div className="absolute right-40 -bottom-20 w-80 h-80 bg-violet/20 rounded-full blur-[100px] pointer-events-none" />
            
            <div className="relative z-10 max-w-xl">
              <h2 className="font-display font-extrabold text-5xl mb-2 tracking-tight">READY FOR IMMERSION</h2>
              <p className="text-gray-400 mb-8 font-sans text-lg">Universal conversion engine for immersive PC gaming. Elevate your PC library to Spatial Computing.</p>
              
              <div className="flex items-center space-x-4">
                <NXButton 
                  size="lg" 
                  icon={xrState === 'VR_READY' ? <Activity size={24}/> : <Play size={24} className={xrState === 'INITIALIZING' ? 'animate-pulse' : ''} />}
                  onClick={handleLaunchVR}
                  disabled={xrState === 'INITIALIZING' || xrState === 'TRACKING_LOCKED'}
                >
                  {xrState === 'IDLE' ? 'DETECT HEADSET' : 
                   xrState === 'DETECTING' ? 'SEARCHING...' :
                   xrState === 'INITIALIZING' ? 'INITIALIZING XR...' :
                   xrState === 'TRACKING_LOCKED' ? 'LOCKING TRACKING...' :
                   xrState === 'VR_READY' ? 'SESSION ACTIVE' :
                   games.length > 0 ? `LAUNCH ${games[0].title.toUpperCase()}` : 'LAUNCH IN VR'}
                </NXButton>
                <NXButton variant="secondary" onClick={() => setCurrentView('profiles')}>CONFIGURE PROFILE</NXButton>
              </div>
            </div>

            {/* 3D Mock Headset Visualization */}
            <div className="relative z-10 w-64 h-64 flex items-center justify-center mr-10">
              <div className={`absolute inset-0 border-2 rounded-full transition-all duration-1000 ${
                xrState === 'READY' || xrState === 'VR_READY' ? 'border-cyan/30 scale-110' : 'border-white/5 scale-100'
              }`} />
              <div className={`absolute inset-4 border rounded-full transition-all duration-1000 delay-100 ${
                xrState === 'READY' || xrState === 'VR_READY' ? 'border-cyan/20 scale-110' : 'border-white/5 scale-100'
              }`} />
              
              <div className={`relative z-20 transition-all duration-500 transform ${
                xrState === 'DETECTING' ? 'animate-bounce' : 
                xrState === 'TRACKING_LOCKED' || xrState === 'VR_READY' ? 'animate-[spin_4s_linear_infinite]' : ''
              }`}>
                <Headphones size={80} className={`${
                  xrState === 'IDLE' ? 'text-gray-600' :
                  xrState === 'DETECTING' ? 'text-amber-400' :
                  'text-cyan drop-shadow-[0_0_15px_rgba(0,245,212,0.8)]'
                } transition-colors duration-500`} />
              </div>
            </div>
          </div>

          {/* Quick Stats Grid */}
          <div className="grid grid-cols-3 gap-6">
            <div className="glass-panel p-6">
              <div className="flex items-center justify-between mb-4">
                <h3 className="font-mono text-sm text-gray-400 uppercase tracking-widest">Active Runtime</h3>
                <Cpu size={18} className="text-violet" />
              </div>
              <p className="font-display text-2xl font-semibold">
                {xrState === 'IDLE' ? 'Waiting...' : 'OpenXR'}
              </p>
              <p className="text-sm text-cyan mt-1 font-mono">Windows Mixed Reality</p>
            </div>

            <div className="glass-panel p-6">
              <div className="flex items-center justify-between mb-4">
                <h3 className="font-mono text-sm text-gray-400 uppercase tracking-widest">Tracking Status</h3>
                <Activity size={18} className="text-cyan" />
              </div>
              <p className="font-display text-2xl font-semibold">
                {xrState === 'VR_READY' || xrState === 'TRACKING_LOCKED' ? '6DOF Active' : 'Standby'}
              </p>
              <p className="text-sm text-gray-500 mt-1 font-mono">2 Controllers Detected</p>
            </div>

            <div className="glass-panel p-6 border-cyan/20">
              <div className="flex items-center justify-between mb-4">
                <h3 className="font-mono text-sm text-gray-400 uppercase tracking-widest">System Status</h3>
                <div className={`w-2 h-2 rounded-full ${xrState === 'IDLE' ? 'bg-amber-500' : 'bg-cyan glow-cyan'}`} />
              </div>
              <p className="font-display text-2xl font-semibold text-cyan text-glow">
                {xrState === 'IDLE' ? 'ACTION REQUIRED' : 'READY'}
              </p>
              <p className="text-sm text-gray-400 mt-1 font-sans">All subsystems nominal</p>
            </div>
          </div>
          </>
          )}

          {currentView === 'library' && <MyGames setCurrentView={setCurrentView} games={games} setGames={setGames} />}
          {currentView === 'profiles' && <VRProfiles />}
          {currentView === 'settings' && <Settings />}
          {currentView === 'devices' && <Devices />}
          {currentView === 'performance' && <Performance />}
          {currentView === 'plugins' && <Plugins />}
        </div>
      </main>
    </div>
  );
}
