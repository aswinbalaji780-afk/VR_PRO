import { contextBridge, ipcRenderer } from 'electron';

// Expose safe APIs to the React renderer process
contextBridge.exposeInMainWorld('electronAPI', {
  sendMessage: (message: string) => ipcRenderer.send('message', message),
  launchInjector: (exeName: string) => ipcRenderer.invoke('launch-injector', exeName),
  selectGame: () => ipcRenderer.invoke('select-game'),
});
