import { contextBridge, ipcRenderer } from 'electron';

// Expose safe APIs to the React renderer process
contextBridge.exposeInMainWorld('electronAPI', {
  sendMessage: (message: string) => ipcRenderer.send('message', message),
  launchInjector: (exeName: string, dirPath?: string) => ipcRenderer.invoke('launch-injector', exeName, dirPath),
  uninstallInjector: (dirPath: string) => ipcRenderer.invoke('uninstall-injector', dirPath),
  selectGame: () => ipcRenderer.invoke('select-game'),
});
