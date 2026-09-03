import { contextBridge, ipcRenderer } from 'electron';



// Expose safe APIs to the React renderer process
contextBridge.exposeInMainWorld('electronAPI', {
  sendMessage: (message: string) => ipcRenderer.send('message', message),
  launchInjector: (exeName: string, dirPath?: string) => ipcRenderer.invoke('launch-injector', exeName, dirPath),
  uninstallInjector: (dirPath: string) => ipcRenderer.invoke('uninstall-injector', dirPath),
  selectGame: () => ipcRenderer.invoke('select-game'),
  getAppVersion: () => ipcRenderer.invoke('get-app-version'),
  checkForUpdates: () => ipcRenderer.invoke('check-for-updates'),
  onUpdateStatus: (callback: (status: string) => void) => {
    ipcRenderer.on('update-status', (_, status) => callback(status));
  },
});
