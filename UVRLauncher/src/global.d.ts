export interface IElectronAPI {
  sendMessage: (message: string) => void;
  launchInjector: (exeName: string, dirPath?: string) => Promise<string>;
  uninstallInjector: (dirPath: string) => Promise<string>;
  selectGame: () => Promise<{ title: string, exeName: string, dirPath: string } | null>;
  getAppVersion: () => Promise<string>;
  checkForUpdates: () => Promise<string>;
  onUpdateStatus: (callback: (status: string) => void) => void;
}

declare global {
  interface Window {
    electronAPI: IElectronAPI;
  }
}
