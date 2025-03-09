const { app, BrowserWindow, ipcMain, contextBridge } = require('electron');
const { autoUpdater } = require('electron-updater');
const os = require('os');
const fs = require('fs');

// === Hauptprozess ===
let mainWindow;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      contextIsolation: true,
      preload: `${__dirname}/preload.js`, // Preload für Sicherheits-Sandbox
    },
  });

  mainWindow.loadFile('index.html');
  mainWindow.webContents.on('did-finish-load', () => {
    console.log('Window loaded successfully');
  });
}

app.on('ready', () => {
  createWindow();

  // Automatische Updates prüfen
  autoUpdater.checkForUpdatesAndNotify();
});

// === IPC und Native API Bridge ===
ipcMain.handle('get-os-info', () => ({
  platform: os.platform(),
  version: os.version(),
}));

ipcMain.handle('open-file', (event, path) => {
  try {
    return fs.readFileSync(path, 'utf8');
  } catch (err) {
    return { error: 'Unable to read file', details: err.message };
  }
});

// === Updates ===
autoUpdater.on('update-available', () => {
  console.log('Update available');
});

autoUpdater.on('update-downloaded', () => {
  console.log('Update downloaded; installing...');
  autoUpdater.quitAndInstall();
});

// === Speicherverwaltung ===
app.on('browser-window-created', (event, window) => {
  window.webContents.on('did-finish-load', () => {
    console.log('Memory Usage:', process.memoryUsage());
  });
});

// === Preload (Sicherheits-Sandbox) ===
// Speichere dies in einer Datei namens `preload.js`:
contextBridge.exposeInMainWorld('nativeAPI', {
  getOSInfo: () => ipcRenderer.invoke('get-os-info'),
  openFile: (path) => ipcRenderer.invoke('open-file', path),
});

// === HTML-Datei für UI ===
// Speichere diese in `index.html`:
/*
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <meta http-equiv="Content-Security-Policy" content="default-src 'self';">
    <title>Desktop Integration System</title>
  </head>
  <body>
    <h1>Desktop Integration System</h1>
    <button id="os-info">Get OS Info</button>
    <button id="open-file">Open File</button>
    <pre id="output"></pre>
    <script>
      const output = document.getElementById('output');

      document.getElementById('os-info').addEventListener('click', () => {
        window.nativeAPI.getOSInfo().then((info) => {
          output.textContent = JSON.stringify(info, null, 2);
        });
      });

      document.getElementById('open-file').addEventListener('click', () => {
        const path = prompt('Enter file path:');
        if (path) {
          window.nativeAPI.openFile(path).then((content) => {
            output.textContent = content.error
              ? `Error: ${content.details}`
              : content;
          });
        }
      });
    </script>
  </body>
</html>
*/

// === Startskript ===
// Füge folgendes in der `package.json` hinzu:
// "main": "main.js",
// "scripts": { "start": "electron ." }

// Installiere die benötigten Pakete:
// npm install electron electron-updater

// Starte die Anwendung:
// npm run start
