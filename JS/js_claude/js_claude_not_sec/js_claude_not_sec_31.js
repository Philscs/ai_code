// Abstract base class for file system providers
class FileSystemProvider {
    constructor() {
      if (this.constructor === FileSystemProvider) {
        throw new Error('Cannot instantiate abstract class');
      }
    }
  
    async read(path) { throw new Error('Not implemented'); }
    async write(path, data) { throw new Error('Not implemented'); }
    async delete(path) { throw new Error('Not implemented'); }
    async list(path) { throw new Error('Not implemented'); }
    async exists(path) { throw new Error('Not implemented'); }
    async mkdir(path) { throw new Error('Not implemented'); }
    async stat(path) { throw new Error('Not implemented'); }
    watch(path, callback) { throw new Error('Not implemented'); }
  }
  
  // Memory-based file system provider
  class MemoryProvider extends FileSystemProvider {
    constructor() {
      super();
      this.files = new Map();
      this.watchers = new Map();
    }
  
    async read(path) {
      const normalizedPath = this.normalizePath(path);
      if (!this.files.has(normalizedPath)) {
        throw new Error('File not found');
      }
      return this.files.get(normalizedPath);
    }
  
    async write(path, data) {
      const normalizedPath = this.normalizePath(path);
      this.files.set(normalizedPath, data);
      this.notifyWatchers(normalizedPath, 'write');
    }
  
    async delete(path) {
      const normalizedPath = this.normalizePath(path);
      if (!this.files.has(normalizedPath)) {
        throw new Error('File not found');
      }
      this.files.delete(normalizedPath);
      this.notifyWatchers(normalizedPath, 'delete');
    }
  
    async list(path) {
      const normalizedPath = this.normalizePath(path);
      const files = [];
      for (const filePath of this.files.keys()) {
        if (filePath.startsWith(normalizedPath)) {
          files.push(filePath);
        }
      }
      return files;
    }
  
    async exists(path) {
      const normalizedPath = this.normalizePath(path);
      return this.files.has(normalizedPath);
    }
  
    async mkdir(path) {
      // In memory provider, directories are implicit
      return true;
    }
  
    async stat(path) {
      const normalizedPath = this.normalizePath(path);
      if (!this.files.has(normalizedPath)) {
        throw new Error('File not found');
      }
      const content = this.files.get(normalizedPath);
      return {
        size: content.length,
        isDirectory: false,
        created: Date.now(),
        modified: Date.now()
      };
    }
  
    watch(path, callback) {
      const normalizedPath = this.normalizePath(path);
      if (!this.watchers.has(normalizedPath)) {
        this.watchers.set(normalizedPath, new Set());
      }
      this.watchers.get(normalizedPath).add(callback);
  
      return () => {
        this.watchers.get(normalizedPath).delete(callback);
        if (this.watchers.get(normalizedPath).size === 0) {
          this.watchers.delete(normalizedPath);
        }
      };
    }
  
    notifyWatchers(path, event) {
      for (const [watchPath, callbacks] of this.watchers) {
        if (path.startsWith(watchPath)) {
          for (const callback of callbacks) {
            callback(event, path);
          }
        }
      }
    }
  
    normalizePath(path) {
      return path.replace(/\\/g, '/').replace(/\/+/g, '/').replace(/^\//, '').replace(/\/$/, '');
    }
  }
  
  // Local file system provider using Node.js fs module
  class LocalProvider extends FileSystemProvider {
    constructor() {
      super();
      this.fs = require('fs/promises');
      this.fsSync = require('fs');
      this.path = require('path');
    }
  
    async read(path) {
      return await this.fs.readFile(path);
    }
  
    async write(path, data) {
      await this.fs.writeFile(path, data);
    }
  
    async delete(path) {
      await this.fs.unlink(path);
    }
  
    async list(path) {
      const files = await this.fs.readdir(path, { withFileTypes: true });
      return files.map(file => this.path.join(path, file.name));
    }
  
    async exists(path) {
      try {
        await this.fs.access(path);
        return true;
      } catch {
        return false;
      }
    }
  
    async mkdir(path) {
      await this.fs.mkdir(path, { recursive: true });
    }
  
    async stat(path) {
      const stats = await this.fs.stat(path);
      return {
        size: stats.size,
        isDirectory: stats.isDirectory(),
        created: stats.birthtime,
        modified: stats.mtime
      };
    }
  
    watch(path, callback) {
      const watcher = this.fsSync.watch(path, (event, filename) => {
        callback(event, this.path.join(path, filename));
      });
      return () => watcher.close();
    }
  }
  
  // Virtual File System class that manages providers and handles path resolution
  class VirtualFileSystem {
    constructor() {
      this.providers = new Map();
      this.mounts = new Map();
    }
  
    mount(path, provider) {
      const normalizedPath = this.normalizePath(path);
      this.providers.set(provider, normalizedPath);
      this.mounts.set(normalizedPath, provider);
    }
  
    unmount(path) {
      const normalizedPath = this.normalizePath(path);
      const provider = this.mounts.get(normalizedPath);
      if (provider) {
        this.providers.delete(provider);
        this.mounts.delete(normalizedPath);
      }
    }
  
    findProvider(path) {
      const normalizedPath = this.normalizePath(path);
      let longestPrefix = '';
      let selectedProvider = null;
  
      for (const [mountPath, provider] of this.mounts) {
        if (normalizedPath.startsWith(mountPath) && mountPath.length > longestPrefix.length) {
          longestPrefix = mountPath;
          selectedProvider = provider;
        }
      }
  
      if (!selectedProvider) {
        throw new Error('No provider found for path');
      }
  
      const relativePath = normalizedPath.slice(longestPrefix.length);
      return { provider: selectedProvider, path: relativePath };
    }
  
    async createReadStream(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      const chunks = [];
      
      return new ReadableStream({
        async start(controller) {
          try {
            const data = await provider.read(resolvedPath);
            controller.enqueue(data);
            controller.close();
          } catch (error) {
            controller.error(error);
          }
        }
      });
    }
  
    async createWriteStream(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      let buffer = Buffer.from([]);
  
      return new WritableStream({
        write(chunk) {
          buffer = Buffer.concat([buffer, chunk]);
        },
        async close() {
          await provider.write(resolvedPath, buffer);
        }
      });
    }
  
    normalizePath(path) {
      return path.replace(/\\/g, '/').replace(/\/+/g, '/').replace(/^\//, '').replace(/\/$/, '');
    }
  
    // Proxy all basic operations to the appropriate provider
    async read(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      return await provider.read(resolvedPath);
    }
  
    async write(path, data) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      await provider.write(resolvedPath, data);
    }
  
    async delete(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      await provider.delete(resolvedPath);
    }
  
    async list(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      const files = await provider.list(resolvedPath);
      return files.map(file => this.normalizePath(file));
    }
  
    async exists(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      return await provider.exists(resolvedPath);
    }
  
    async mkdir(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      await provider.mkdir(resolvedPath);
    }
  
    async stat(path) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      return await provider.stat(resolvedPath);
    }
  
    watch(path, callback) {
      const { provider, path: resolvedPath } = this.findProvider(path);
      return provider.watch(resolvedPath, callback);
    }
  }
  
  // Example usage:
  async function example() {
    const vfs = new VirtualFileSystem();
    
    // Mount different providers
    const memoryFs = new MemoryProvider();
    const localFs = new LocalProvider();
    
    vfs.mount('/memory', memoryFs);
    vfs.mount('/local', localFs);
    
    // Write and read from memory provider
    await vfs.write('/memory/test.txt', 'Hello, World!');
    const content = await vfs.read('/memory/test.txt');
    console.log(content.toString()); // Hello, World!
    
    // Stream operations
    const writeStream = await vfs.createWriteStream('/memory/stream.txt');
    const writer = writeStream.getWriter();
    await writer.write(new TextEncoder().encode('Streaming data'));
    await writer.close();
    
    const readStream = await vfs.createReadStream('/memory/stream.txt');
    const reader = readStream.getReader();
    const { value } = await reader.read();
    console.log(new TextDecoder().decode(value)); // Streaming data
    
    // Watch for changes
    const unwatch = vfs.watch('/memory', (event, path) => {
      console.log(`File ${path} ${event}`);
    });
    
    // Cleanup
    unwatch();
  }