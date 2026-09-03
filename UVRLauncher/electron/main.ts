import { app, BrowserWindow, ipcMain, dialog } from 'electron';
import path from 'node:path';
import fs from 'node:fs';
import { autoUpdater } from 'electron-updater';
import { exec } from 'node:child_process';



// Suppress the CSP warning in development
process.env['ELECTRON_DISABLE_SECURITY_WARNINGS'] = 'true';

// Global error guards to prevent silent crashes across any machine
process.on('uncaughtException', (err) => {
  console.error('Unhandled Exception in Main Process:', err);
});

process.on('unhandledRejection', (reason) => {
  console.error('Unhandled Rejection in Main Process:', reason);
});

// Prevent GPU driver resets on laptops from killing the app
app.commandLine.appendSwitch('disable-gpu-process-crash-limit');

// CLI version check (like java --version)
if (process.argv.includes('--version') || process.argv.includes('-v')) {
  process.stdout.write(`UVR Launcher v${app.getVersion()}\n`);
  console.log(`UVR Launcher v${app.getVersion()}`);
  app.exit(0);
}

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

app.setAppUserModelId('com.nexusxr.uvrlauncher');

app.whenReady().then(() => {
  createWindow();

  // Check for updates (only works in production/packaged app)
  if (app.isPackaged) {
    autoUpdater.autoDownload = false; // Safe: don't freeze or saturate network silently
    autoUpdater.autoInstallOnAppQuit = false;

    autoUpdater.on('checking-for-update', () => {
      win?.webContents.send('update-status', 'Checking GitHub for updates...');
    });

    autoUpdater.on('update-available', (info) => {
      console.log('Update available:', info.version);
      win?.webContents.send('update-status', `Update v${info.version} available!`);
      dialog.showMessageBox({
        type: 'info',
        title: 'New Update Available',
        message: `Version ${info.version} of UVR Launcher is available!`,
        detail: 'Would you like to download this update in the background now?',
        buttons: ['Download Update', 'Later'],
        defaultId: 0,
        cancelId: 1
      }).then((result) => {
        if (result.response === 0) {
          win?.webContents.send('update-status', 'Downloading update in background...');
          autoUpdater.downloadUpdate().catch((dlErr) => {
            console.error('Download error:', dlErr);
            win?.webContents.send('update-status', `Download failed: ${dlErr.message}`);
          });
        }
      }).catch(() => {});
    });

    autoUpdater.on('update-not-available', (info) => {
      console.log('Update not available');
      win?.webContents.send('update-status', `You are using the latest version (v${info.version}).`);
    });

    autoUpdater.on('download-progress', (progress) => {
      win?.webContents.send('update-status', `Downloading update: ${Math.round(progress.percent)}%`);
    });

    autoUpdater.on('update-downloaded', (info) => {
      console.log('Update downloaded:', info.version);
      win?.webContents.send('update-status', `Update v${info.version} ready to install!`);
      dialog.showMessageBox({
        type: 'info',
        title: 'Update Ready to Install',
        message: `Version ${info.version} of UVR Launcher has been downloaded!`,
        detail: 'Click Restart to install the update now.',
        buttons: ['Restart Now', 'Install on Exit'],
        defaultId: 0,
        cancelId: 1
      }).then((returnValue) => {
        if (returnValue.response === 0) {
          autoUpdater.quitAndInstall();
        } else {
          autoUpdater.autoInstallOnAppQuit = true;
        }
      }).catch(() => {});
    });

    autoUpdater.on('error', (err) => {
      console.error('Error checking for updates:', err);
      win?.webContents.send('update-status', `Update status: ${err.message || 'Check complete'}`);
    });

    // Check for updates 5 seconds after launch (non-blocking)
    setTimeout(() => {
      try {
        autoUpdater.checkForUpdates().catch(e => console.warn('Background update check notice:', e.message));
      } catch (e) {
        console.warn('Silent update catch:', e);
      }
    }, 5000);
  }

  ipcMain.handle('get-app-version', async () => {
    return app.getVersion();
  });

  ipcMain.handle('check-for-updates', async () => {
    if (!app.isPackaged) {
      return "Dev Mode: Auto-update only runs in packaged app.";
    }
    try {
      win?.webContents.send('update-status', 'Checking GitHub for updates...');
      const checkResult = await autoUpdater.checkForUpdates();
      if (!checkResult || !checkResult.updateInfo) {
        return "No updates found.";
      }
      return `Found v${checkResult.updateInfo.version}`;
    } catch (e: any) {
      win?.webContents.send('update-status', `Update check: ${e.message}`);
      return `Status: ${e.message}`;
    }
  });


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

      // Stateless resolution: look in app resources first (production), then fallback to relative dev tree
      const prodDxgi = path.join(process.resourcesPath, 'dxgi.dll');
      const prodIni = path.join(process.resourcesPath, 'vr_config.ini');
      const devDxgi = path.join(__dirname, '../../VRModFramework/build/Release/dxgi.dll');
      const devIni = path.join(__dirname, '../../VRModFramework/vr_config.ini');

      const sourceDxgi = fs.existsSync(prodDxgi) ? prodDxgi : devDxgi;
      const sourceIni = fs.existsSync(prodIni) ? prodIni : devIni;

      if (!fs.existsSync(sourceDxgi)) {
        if (!app.isPackaged) {
          resolve(`Dev Mode: Simulated installation for ${exeName} to ${dirPath}.`);
          return;
        }
        reject(new Error(`VR Mod DLL (dxgi.dll) not found in launcher resources at: ${sourceDxgi}`));
        return;
      }

      try {
        const processNameNoExt = exeName.replace('.exe', '');

        // Copy dxgi.dll to game folder
        fs.copyFileSync(sourceDxgi, path.join(dirPath, 'dxgi.dll'));

        // Prepare vr_config.ini dynamically in memory (100% stateless across all machines)
        let iniContent = "";
        if (fs.existsSync(sourceIni)) {
          iniContent = fs.readFileSync(sourceIni, 'utf8');
        } else {
          iniContent = "[Target]\nProcessName=game\n\n[HeadTracking]\nMode=Mouse\nMouseSensitivityX=1000.0\nMouseSensitivityY=1000.0\nInvertY=false\n\n[Debug]\nForceSplitScreen=true\n";
        }

        // Dynamically set game process name and ensure desktop split screen is enabled
        iniContent = iniContent.replace(/^ProcessName=.*$/m, `ProcessName=${processNameNoExt}`);
        if (!iniContent.includes("ForceSplitScreen=")) {
          iniContent += "\n[Debug]\nForceSplitScreen=true\n";
        } else {
          iniContent = iniContent.replace(/^ForceSplitScreen=.*$/m, `ForceSplitScreen=true`);
        }

        fs.writeFileSync(path.join(dirPath, 'vr_config.ini'), iniContent, 'utf8');

        resolve(`Successfully installed VR Mod to ${dirPath}! Start ${exeName} normally to play in VR.`);
      } catch (error: any) {
        console.error('Copy error:', error);
        if (error.code === 'EBUSY' || (error.message && error.message.includes('EBUSY'))) {
          reject(new Error(`The game is currently running! Please completely close ${exeName} (check Task Manager to ensure it is not running in the background), then try again.`));
        } else {
          reject(new Error(`Failed to copy VR mod to game folder: ${error.message}`));
        }
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
        const targetIni = path.join(dirPath, 'vr_config.ini');

        if (fs.existsSync(targetDxgi)) fs.unlinkSync(targetDxgi);
        if (fs.existsSync(targetIni)) fs.unlinkSync(targetIni);

        resolve("Successfully uninstalled VR Mod!");
      } catch (error: any) {
        console.error('Uninstall error:', error);
        reject(new Error(`Failed to delete VR mod from game folder: ${error.message}`));
      }
    });
  });
});
