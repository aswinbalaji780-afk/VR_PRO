export interface IElectronAPI {
  sendMessage: (message: string) => void;
  launchInjector: (exeName: string, dirPath?: string) => Promise<string>;
  uninstallInjector: (dirPath: string) => Promise<string>;
  selectGame: () => Promise<{ title: string, exeName: string, dirPath: string } | null>;
}

declare global {
  interface Window {
    electronAPI: IElectronAPI;
  }
}
