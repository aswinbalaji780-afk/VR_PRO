import React from 'react';
import { Play, Settings2, Plus, Trash2 } from 'lucide-react';
import { NXButton } from '../components/NXButton';

interface MyGamesProps {
  setCurrentView: (view: string) => void;
  games: any[];
  setGames: React.Dispatch<React.SetStateAction<any[]>>;
}

export const MyGames: React.FC<MyGamesProps> = ({ setCurrentView, games, setGames }) => {
  const handleInject = async (id: number, exeName: string, dirPath: string) => {
    // If a game is injecting, set all others to 'Ready' and this one to 'Installing...'
    setGames(prev => prev.map(g => {
      if (g.id === id) return { ...g, status: 'Installing...' };
      if (g.status === 'Installed' || g.status === 'Installing...') return { ...g, status: 'Ready' };
      return g;
    }));
    
    try {
      // Actually call the C++ injector via Electron Main Process
      const response = await window.electronAPI.launchInjector(exeName, dirPath);
      console.log('Installation response:', response);
      setGames(prev => prev.map(g => g.id === id ? { ...g, status: 'Installed' } : g));
    } catch (e: any) {
      console.error('Installation failed:', e);
      alert(e.message || 'Installation failed');
      setGames(prev => prev.map(g => g.id === id ? { ...g, status: 'Ready' } : g));
    }
  };

  const handleStop = (id: number) => {
    const isConfirmed = window.confirm(
      "WARNING: Uninstalling only resets the launcher's state.\n\n" +
      "To fully uninstall the VR Mod, you must manually delete dxgi.dll from the game folder!"
    );
    
    if (isConfirmed) {
      setGames(prev => prev.map(g => g.id === id ? { ...g, status: 'Ready' } : g));
    }
  };

  const handleDeleteGame = (id: number) => {
    if (window.confirm('Are you sure you want to remove this game from your library?')) {
      setGames(prev => prev.filter(g => g.id !== id));
    }
  };

  const handleAddGame = async () => {
    try {
      const selectedGame = await window.electronAPI.selectGame();
      if (selectedGame) {
        const newId = games.length > 0 ? Math.max(...games.map(g => g.id)) + 1 : 1;
        setGames([...games, {
          id: newId,
          title: selectedGame.title,
          exeName: selectedGame.exeName,
          dirPath: selectedGame.dirPath,
          status: 'Ready',
          image: 'https://images.unsplash.com/photo-1552820728-8b83bb6b773f?q=80&w=600&auto=format&fit=crop'
        }]);
      }
    } catch (e) {
      console.error('Failed to select game:', e);
    }
  };

  return (
    <div className="w-full h-full flex flex-col">
      <div className="flex items-center justify-between mb-8">
        <div>
          <h2 className="font-display text-3xl font-bold mb-1">My Games</h2>
          <p className="text-gray-400 font-sans">Manage your VR-injected PC titles.</p>
        </div>
        <NXButton icon={<Plus size={18} />} onClick={handleAddGame}>ADD GAME</NXButton>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
        {games.map((game) => (
          <div key={game.id} className="glass-panel group relative overflow-hidden flex flex-col h-72 cursor-pointer transition-transform hover:scale-[1.02]">
            {/* Background Image */}
            <div 
              className="absolute inset-0 bg-cover bg-center transition-transform duration-700 group-hover:scale-110 opacity-40 group-hover:opacity-60"
              style={{ backgroundImage: `url(${game.image})` }}
            />
            {/* Gradient Overlay */}
            <div className="absolute inset-0 bg-gradient-to-t from-obsidian via-obsidian/80 to-transparent" />
            
            {/* Content */}
            <div className="relative z-10 flex flex-col h-full p-5 justify-end">
              <div className="absolute top-4 right-4 flex items-center space-x-2">
                <div className="bg-black/50 backdrop-blur px-2 py-1 rounded text-xs font-mono font-bold tracking-wider border border-white/10">
                  <span className={game.status === 'Ready' || game.status === 'Installed' ? 'text-cyan' : 'text-amber-400'}>
                    {game.status}
                  </span>
                </div>
                <button 
                  onClick={(e) => { e.stopPropagation(); handleDeleteGame(game.id); }}
                  className="bg-black/50 backdrop-blur p-1.5 rounded text-gray-400 hover:text-red-400 border border-white/10 hover:border-red-500/50 transition-colors"
                  title="Remove Game"
                >
                  <Trash2 size={14} />
                </button>
              </div>
              
              <h3 className="font-display text-xl font-bold mb-3">{game.title}</h3>
              
              {/* Action Buttons (Reveal on Hover) */}
              <div className="flex items-center justify-between opacity-0 translate-y-4 group-hover:opacity-100 group-hover:translate-y-0 transition-all duration-300">
                <button 
                  onClick={(e) => { 
                    e.stopPropagation(); 
                    if (game.status === 'Installed') handleStop(game.id);
                    else handleInject(game.id, game.exeName, game.dirPath); 
                  }}
                  disabled={game.status === 'Installing...'}
                  className={`flex-1 font-bold py-2 px-4 rounded font-sans text-sm mr-2 flex items-center justify-center transition-colors group/btn ${
                    game.status === 'Installed' 
                      ? 'bg-green-500/20 text-green-400 border border-green-500/50 hover:bg-red-500/20 hover:text-red-400 hover:border-red-500/50'
                      : game.status === 'Installing...'
                        ? 'bg-amber-500/20 text-amber-400 border border-amber-500/50 cursor-wait'
                        : 'bg-cyan text-obsidian hover:bg-cyan/90 shadow-[0_0_15px_rgba(0,245,212,0.4)]'
                  }`}
                >
                  <Play size={16} className={`mr-2 ${game.status === 'Installing...' ? 'animate-pulse' : ''} ${game.status === 'Installed' ? 'group-hover/btn:hidden' : ''}`} /> 
                  <span className={game.status === 'Installed' ? 'group-hover/btn:hidden' : ''}>
                    {game.status === 'Installed' ? 'Ready to Play' : game.status === 'Installing...' ? 'Installing...' : 'Install Mod'}
                  </span>
                  <span className={`hidden font-bold ${game.status === 'Installed' ? 'group-hover/btn:inline' : ''}`}>UNINSTALL</span>
                </button>
                <button 
                  onClick={(e) => { e.stopPropagation(); setCurrentView('profiles'); }}
                  className="bg-white/10 p-2 rounded hover:bg-white/20 transition-colors border border-white/10"
                >
                  <Settings2 size={20} className="text-gray-300" />
                </button>
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};
