import React, { useState, useEffect } from 'react';
import axios from 'axios';

const FileUploader = () => {
  const [file, setFile] = useState(null);
  const [chunks, setChunks] = useState([]);
  const [uploadProgress, setUploadProgress] = useState(0);
  const [error, setError] = useState(null);

  useEffect(() => {
    if (file && chunks.length > 0) {
      uploadFile();
    } else {
      setError('Keine Datei oder keine Dateien gefunden');
    }
  }, [file, chunks]);

  const handleDrop = acceptedFiles => {
    const file = acceptedFiles[0];
    const chunks = [];
    for (let i = 0; i < file.size; i += 1024 * 1024) {
      chunks.push(file.slice(i, i + 1024 * 1024));
    }
    setChunks(chunks);
    setFile(file);
  };

  const uploadFile = async () => {
    try {
      const response = await axios.post('/upload', file);
      console.log(response.data);

      // Lade die nächsten Chunk
      if (chunks.length > 0) {
        const chunk = chunks.shift();
        setChunks([...chunks, chunk]);
        const progress = Math.floor((chunk.size / file.size) * 100);
        setUploadProgress(progress);
        uploadFile();
      }
    } catch (error) {
      setError('Fehler beim Hochladen der Datei');
    }
  };

  return (
    <div>
      {file ? (
        <p> Datei hochgeladen: {Math.floor((uploadProgress / 100) * file.size)} KB</p>
      ) : (
        <p> oder wählen Sie eine Datei...</p>
      )}
      <input type="file" onChange={event => handleDrop(event.target.files)} />
      {error && <p style={{ color: 'red' }}>{error}</p>}
    </div>
  );
};

export default FileUploader;