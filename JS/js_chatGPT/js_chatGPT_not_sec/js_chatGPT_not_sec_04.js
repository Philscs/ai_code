const http = require('http');
const { Transform, pipeline } = require('stream');
const JSONStream = require('JSONStream');

/**
 * StreamProcessor: Transform-Stream für komplexe JSON-Transformationen.
 */
class StreamProcessor extends Transform {
  constructor(transformFn, filterFn) {
    super({ objectMode: true });
    this.transformFn = transformFn;
    this.filterFn = filterFn;
  }

  _transform(chunk, encoding, callback) {
    try {
      // Filterung anwenden
      if (this.filterFn && !this.filterFn(chunk)) {
        return callback(); // überspringen
      }

      // Transformation anwenden
      const transformed = this.transformFn ? this.transformFn(chunk) : chunk;

      this.push(transformed);
      callback();
    } catch (err) {
      callback(err);
    }
  }
}

// Beispiel für eine Transformation
const transformFn = (data) => {
  // z.B. bestimmte Felder extrahieren oder modifizieren
  return {
    id: data.id,
    name: data.name.toUpperCase(),
    timestamp: new Date().toISOString(),
  };
};

// Beispiel für eine Filterfunktion
const filterFn = (data) => {
  // z.B. nur Einträge mit einer bestimmten Bedingung behalten
  return data.isActive;
};

// HTTP-Server für JSON-Streaming
http.createServer((req, res) => {
  if (req.method === 'POST') {
    res.writeHead(200, { 'Content-Type': 'application/json' });

    pipeline(
      req,
      JSONStream.parse('*'), // JSON-Daten parsen
      new StreamProcessor(transformFn, filterFn), // Transformation und Filterung anwenden
      JSONStream.stringify(), // Daten zurück in JSON umwandeln
      res,
      (err) => {
        if (err) {
          console.error('Pipeline error:', err);
          res.statusCode = 500;
          res.end(JSON.stringify({ error: 'Internal Server Error' }));
        }
      }
    );
  } else {
    res.writeHead(405, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ error: 'Method Not Allowed' }));
  }
}).listen(3000, () => {
  console.log('JSON Stream Processor läuft auf http://localhost:3000');
});
