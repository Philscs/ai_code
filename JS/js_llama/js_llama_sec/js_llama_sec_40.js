// main.js
import { IPC } from '@tauri-apps/api/ipc';
import { NativeAPIBridge } from './native-api-bridge';
import { Security } from '@tauri-apps/api/security';
import { Resource } from '@tauri-apps/api/resource';
import { Update } from '@tauri-apps/api/update';

const ipc = new IPC();
const bridge = new NativeAPIBridge();
const security = new Security();
const resource = new Resource();
const update = new Update();

// Sicherheits-Einstellungen
security.setAllowFileAccess(false);

// Ressourcen-Einstellungen
resource.setAllowFileAccess(false);

// Aktualisierungs-Einstellungen
update.setAllowFileAccess(false);

// IPC-Listening
ipc.on('hello', () => {
  console.log('Hallo aus dem JS!');
});

// Native API Bridge
bridge.nativeAPI = {
  helloFunction: () => {
    console.log('Hallo aus der native C-Bibliothek!');
  }
};

// Funktion, die von JS ausgeführt wird und eine native Funktionaufruf über den IPC-Tunnel 
startet
function js_hello_function() {
  ipc.send('hello');
}

// Funktion, die in C geschrieben wird und als native API bereitgestellt wird
js_hello_function();

// Einfacher Update-Mechanismus
function updateFunction() {
  console.log('Update aus dem JS!');
}
