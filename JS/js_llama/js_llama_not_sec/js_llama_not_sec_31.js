class VFS {
    constructor(backends) {
        this.backends = backends;
        this.currentBackend = null;
    }

    setBackend(backendName) {
        if (this.backends[backendName]) {
            this.currentBackend = this.backends[backendName];
        } else {
            throw new Error(`Backend ${backendName} nicht gefunden`);
        }
    }

    readFile(filePath) {
        return this.currentBackend.readFile(filePath);
    }

    writefile(filePath, data) {
        return this.currentBackend.writefile(filePath, data);
    }
}

class VirtualFileSystem extends VFS {
    constructor(backends) {
        super(backends);
        this.folders = {};
    }

    createFolder(folderName) {
        if (!this.folders[folderName]) {
            this.folders[folderName] = new Folder(this.backends);
        }
        return this.folders[folderName];
    }

    readFile(filePath) {
        const pathParts = filePath.split('/');
        let currentPath = this;
        for (const part of pathParts) {
            if (!currentPath.folders[part]) {
                break;
            }
            currentPath = currentPath.folders[part];
        }
        return currentPath.currentBackend.readFile(currentPath.path);
    }

    writefile(filePath, data) {
        const pathParts = filePath.split('/');
        let currentPath = this;
        for (const part of pathParts) {
            if (!currentPath.folders[part]) {
                throw new Error(`Folder ${part} nicht gefunden`);
            }
            currentPath = currentPath.folders[part];
        }
        return currentPath.currentBackend.writefile(currentPath.path, data);
    }

    deleteFile(filePath) {
        const pathParts = filePath.split('/');
        let currentPath = this;
        for (const part of pathParts) {
            if (!currentPath.folders[part]) {
                throw new Error(`Folder ${part} nicht gefunden`);
            }
            currentPath = currentPath.folders[part];
        }
        return currentPath.currentBackend.deleteFile(currentPath.path);
    }

    moveFile(source, destination) {
        const pathPartsSource = source.split('/');
        let currentPath = this;
        for (const part of pathPartsSource) {
            if (!currentPath.folders[part]) {
                throw new Error(`Folder ${part} nicht gefunden`);
            }
            currentPath = currentPath.folders[part];
        }
        const pathPartsDestination = destination.split('/');
        let destinationPath = '';
        for (let i = 0; i < pathPartsDestination.length - 1; i++) {
            destinationPath += pathPartsDestination[i] + '/';
        }
        if (currentPath.path !== destinationPath) {
            throw new Error('Destinationspfad ist falsch');
        }
        return this.writefile(destination, this.readFile(source));
    }

    onEvent(callback) {
        return () => {
            callback();
        };
    }
}

class Backend {
    constructor(pathToBackend) {
        this.path = pathToBackend;
    }

    readFile(filePath) {
        throw new Error('Lesen aus dem Dateisystem ist nicht implementiert');
    }

    writefile(filePath, data) {
        throw new Error('Schreiben in das Dateisystem ist nicht implementiert');
    }

    deleteFile(filePath) {
        throw new Error('Lösen des Dateisystems ist nicht implementiert');
    }
}

class LocalBackend extends Backend {
    constructor(pathToLocalFolder) {
        super(pathToLocalFolder);
    }

    readFile(filePath) {
        const fileContent = require(`${this.path}/${filePath}`);
        return fileContent;
    }

    writefile(filePath, data) {
        fs.writeFileSync(`${this.path}/${filePath}`, data);
    }
}

class RemoteBackend extends Backend {
    constructor(url) {
        super(null);
        this.url = url;
    }

    readFile(filePath) {
        const response = fetch(this.url + filePath);
        return response.text();
    }

    writefile(filePath, data) {
        throw new Error('Schreiben in ein externes Dateisystem ist nicht implementiert');
    }
}

const fs = new VirtualFileSystem({
    'local': new LocalBackend('/local'),
    'remote': new RemoteBackend('https://example.com/vfs')
});

fs.createFolder('test').then(folder => {
    return folder.writefile('test.txt', 'Hallo Welt!');
}).then(() => {
    console.log(fs.readFile('test.txt'));
    fs.deleteFile('test.txt');
}).catch(error => {
    console.error(error);
});