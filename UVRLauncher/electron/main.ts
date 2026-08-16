import { app, BrowserWindow, ipcMain, dialog } from 'electron';
import path from 'node:path';
import fs from 'node:fs';
import { autoUpdater } from 'electron-updater';
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
  createWindow();

  // Check for updates (only works in production/packaged app)
  if (app.isPackaged) {
    autoUpdater.checkForUpdatesAndNotify();
  }

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

        if (!app.isPackaged) {
          console.log("Dev Mode: Simulating installation since C++ DLLs are built in the cloud.");
          resolve(`Successfully installed VR Mod to ${dirPath}! Start the game normally to play in VR.`);
          return;
        }

        // 3. Install Proxy DLL & Dependencies
        // In production, the files are in process.resourcesPath.
        const sourceDxgi = path.join(process.resourcesPath, 'dxgi.dll');
        const sourceOpenXR = path.join(process.resourcesPath, 'openxr_loader.dll');
        const sourceIni = path.join(process.resourcesPath, 'vr_config.ini');
        
        try {
          if (!fs.existsSync(sourceDxgi)) throw new Error(`Mod DLL not found at: ${sourceDxgi}`);
          if (!fs.existsSync(sourceOpenXR)) throw new Error(`OpenXR Loader not found at: ${sourceOpenXR}`);
          if (!fs.existsSync(sourceIni)) throw new Error(`VR Config not found at: ${sourceIni}`);

          fs.copyFileSync(sourceDxgi, path.join(dirPath, 'dxgi.dll'));
          fs.copyFileSync(sourceOpenXR, path.join(dirPath, 'openxr_loader.dll'));
          fs.copyFileSync(sourceIni, path.join(dirPath, 'vr_config.ini'));

          resolve(`Successfully installed VR Mod to ${dirPath}! Start the game normally to play in VR.`);
        } catch (error: any) {
          console.error('Copy error:', error);
          reject(new Error(`Failed to copy VR mod to game folder: ${error.message}`));
        }
    });
  });

  ipcMain.handle('uninstall-injector', async (event, dirPath: string) => {
    return new Promise((resolve, reject) => {
      if (!dirPath) {
        reject(new Error("Cannot uninstall mod: Game directory path is missing."));
        return;
      }
      try {
        const targetDxgi = path.join(dirPath, 'dxgi.dll');
        const targetOpenXR = path.join(dirPath, 'openxr_loader.dll');
        const targetIni = path.join(dirPath, 'vr_config.ini');

        if (fs.existsSync(targetDxgi)) fs.unlinkSync(targetDxgi);
        if (fs.existsSync(targetOpenXR)) fs.unlinkSync(targetOpenXR);
        if (fs.existsSync(targetIni)) fs.unlinkSync(targetIni);

        resolve("Successfully uninstalled VR Mod!");
      } catch (error: any) {
        console.error('Uninstall error:', error);
        reject(new Error(`Failed to delete VR mod from game folder: ${error.message}`));
      }
    });
  });
});
