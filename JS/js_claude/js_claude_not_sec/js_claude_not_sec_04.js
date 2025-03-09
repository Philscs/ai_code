// JSON Stream Processor mit Support für Transformationen und HTTP Streaming
class JsonStreamProcessor {
    constructor(options = {}) {
      this.transformFns = [];
      this.filterFns = [];
      this.chunkSize = options.chunkSize || 1024 * 64; // 64KB default chunk size
      this.delimiter = options.delimiter || '\n';
    }
  
    // Fügt eine Transformationsfunktion hinzu
    addTransform(fn) {
      this.transformFns.push(fn);
      return this;
    }
  
    // Fügt eine Filterfunktion hinzu
    addFilter(fn) {
      this.filterFns.push(fn);
      return this;
    }
  
    // Verarbeitet einen Stream von JSON-Daten
    async *process(readableStream) {
      const decoder = new TextDecoder();
      const reader = readableStream.getReader();
      let buffer = '';
  
      try {
        while (true) {
          const { done, value } = await reader.read();
          
          if (done) {
            if (buffer.trim()) {
              yield* this.processChunk(buffer);
            }
            break;
          }
  
          buffer += decoder.decode(value, { stream: true });
          
          const chunks = buffer.split(this.delimiter);
          buffer = chunks.pop(); // Behalte unvollständigen chunk
  
          for (const chunk of chunks) {
            if (chunk.trim()) {
              yield* this.processChunk(chunk);
            }
          }
        }
      } finally {
        reader.releaseLock();
      }
    }
  
    // Verarbeitet einen einzelnen JSON-Chunk
    async *processChunk(chunk) {
      try {
        let data = JSON.parse(chunk);
  
        // Wende Filter an
        for (const filterFn of this.filterFns) {
          if (!await filterFn(data)) {
            return;
          }
        }
  
        // Wende Transformationen an
        for (const transformFn of this.transformFns) {
          data = await transformFn(data);
        }
  
        yield data;
      } catch (err) {
        console.error('Error processing chunk:', err);
      }
    }
  
    // Streamt JSON-Daten über HTTP
    static async streamHttpJson(url, options = {}) {
      const response = await fetch(url);
      
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      
      if (!response.body) {
        throw new Error('ReadableStream not supported');
      }
  
      return response.body;
    }
  }
  
  // Beispiel für die Verwendung
  async function example() {
    const processor = new JsonStreamProcessor({
      chunkSize: 1024 * 32 // 32KB chunks
    });
  
    // Füge Transformationen hinzu
    processor.addTransform(async data => {
      return {
        ...data,
        timestamp: new Date().toISOString()
      };
    });
  
    // Füge Filter hinzu
    processor.addFilter(async data => {
      return data.type === 'important';
    });
  
    try {
      // Stream JSON von URL
      const jsonStream = await JsonStreamProcessor.streamHttpJson(
        'https://api.example.com/large-dataset.json'
      );
  
      // Verarbeite den Stream
      for await (const item of processor.process(jsonStream)) {
        console.log('Processed item:', item);
      }
    } catch (err) {
      console.error('Error:', err);
    }
  }
  
  // Beispiel für progressive HTTP-Streaming
  class JsonStreamServer {
    static async streamResponse(req, res) {
      res.setHeader('Content-Type', 'application/json');
      res.setHeader('Transfer-Encoding', 'chunked');
  
      const processor = new JsonStreamProcessor();
      
      processor.addTransform(async data => {
        // Beispiel-Transformation
        return {
          ...data,
          processed: true
        };
      });
  
      const jsonStream = await JsonStreamProcessor.streamHttpJson('https://source-api.example.com/data');
  
      for await (const item of processor.process(jsonStream)) {
        // Sende jedes verarbeitete Item als separaten Chunk
        res.write(JSON.stringify(item) + '\n');
      }
  
      res.end();
    }
  }
  
  export { JsonStreamProcessor, JsonStreamServer };