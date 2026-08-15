export interface IElectronAPI {
  sendMessage: (message: string) => void;
  launchInjector: (exeName: string) => Promise<string>;
  selectGame: () => Promise<{ title: string, exeName: string } | null>;
}

declare global {
  interface Window {
    electronAPI: IElectronAPI;
  }
}
