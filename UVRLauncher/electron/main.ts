import { app, BrowserWindow, ipcMain, dialog } from 'electron';
import path from 'node:path';
import fs from 'node:fs';
import { exec } from 'node:child_process';

// Suppress the CSP warning in development
process.env['ELECTRON_DISABLE_SECURITY_WARNINGS'] = 'true';

// The built directory structure
//
// ├─┬─┬ dist
// │ │ └── index.html
// │ │
// │ ├─┬ dist-electron
// │ │ ├── main.js
// │ │ └── preload.js
// │
process.env.DIST = path.join(__dirname, '../dist');
process.env.VITE_PUBLIC = app.isPackaged ? process.env.DIST : path.join(process.env.DIST, '../public');

let win: BrowserWindow | null;

function createWindow() {
  win = new BrowserWindow({
    width: 1280,
    height: 800,
    title: "UVR (Universal VR Mod) Desktop",
    backgroundColor: '#030712',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true,
    },
    // We want the premium feel without the default windows frame if possible, 
    // but for now we keep it standard for ease of development.
    // titleBarStyle: 'hidden',
  });

  // Test active push message to Renderer-process.
  win.webContents.on('did-finish-load', () => {
    win?.webContents.send('main-process-message', (new Date).toLocaleString());
  });

  if (app.isPackaged) {
    win.loadFile(path.join(process.env.DIST, 'index.html'));
    win.webContents.openDevTools(); // Force devtools to open in production
  } else {
    const devServerUrl = process.env.VITE_DEV_SERVER_URL || 'http://localhost:5173';
    win.loadURL(devServerUrl);
    // Open DevTools automatically to catch any future errors only in dev
    win.webContents.openDevTools();
  }
}

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
    win = null;
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

app.whenReady().then(() => {
  ipcMain.handle('select-game', async () => {
    const { canceled, filePaths } = await dialog.showOpenDialog({
      title: 'Select Game Executable',
      filters: [{ name: 'Executables', extensions: ['exe'] }],
      properties: ['openFile']
    });

    if (canceled || filePaths.length === 0) return null;
    
    const filePath = filePaths[0];
    const exeName = path.basename(filePath);
    const dirPath = path.dirname(filePath);
    // Remove .exe extension for title and format nicely
    const title = exeName.replace('.exe', '').replace(/[-_]/g, ' ');

    return { title, exeName, dirPath };
  });

  ipcMain.handle('launch-injector', async (event, exeName: string, dirPath?: string) => {
    return new Promise((resolve, reject) => {
      if (!dirPath) {
        reject(new Error("Cannot install mod: Game directory path is missing. Please re-add the game to your library."));
        return;
      }
      
      // 1. Update vr_config.ini
      const iniPath = path.join(__dirname, '../../VRModFramework/vr_config.ini');
      try {
        if (fs.existsSync(iniPath)) {
          let iniContent = fs.readFileSync(iniPath, 'utf8');
          const processNameNoExt = exeName.replace('.exe', '');
          iniContent = iniContent.replace(/^ProcessName=.*$/m, `ProcessName=${processNameNoExt}`);
          fs.writeFileSync(iniPath, iniContent, 'utf8');
        } else {
          console.warn('vr_config.ini not found, skipping config update.');
        }
      } catch (e) {
        console.error('Failed to update vr_config.ini', e);
      }

        // 3. Install Proxy DLL
        // We find dxgi.dll in our own build folder and copy it to the game directory
        // In production, the dxgi.dll should be packed next to the exe or in a resources folder.
        // For development, we look at the VRModFramework build folder.
        const sourceDll = app.isPackaged 
          ? path.join(process.resourcesPath, 'dxgi.dll') 
          : path.join(__dirname, '../../VRModFramework/build/Release/dxgi.dll');
        
        const targetDll = path.join(dirPath, 'dxgi.dll');

        try {
          if (!fs.existsSync(sourceDll)) {
             throw new Error(`Mod DLL not found at: ${sourceDll}. Please ensure you have downloaded and placed it there.`);
          }
          fs.copyFileSync(sourceDll, targetDll);
          resolve(`Successfully installed VR Mod to ${dirPath}! Start the game normally to play in VR.`);
        } catch (error: any) {
          console.error('Copy error:', error);
          reject(new Error(`Failed to copy VR mod to game folder: ${error.message}`));
        }
    });
  });

  createWindow();
});
