import React, { useState } from "react";

const CHUNK_SIZE = 5 * 1024 * 1024; // 5 MB
const MAX_FILE_SIZE = 50 * 1024 * 1024; // 50 MB
const ALLOWED_FILE_TYPES = ["image/jpeg", "image/png", "application/pdf"];

const FileUpload = () => {
  const [uploadProgress, setUploadProgress] = useState(0);
  const [errorMessage, setErrorMessage] = useState("");
  const [uploading, setUploading] = useState(false);

  const handleFileDrop = async (event) => {
    event.preventDefault();
    const file = event.dataTransfer.files[0];
    if (!file) return;

    // Validate file size
    if (file.size > MAX_FILE_SIZE) {
      setErrorMessage("Die Datei überschreitet die maximale Größe von 50 MB.");
      return;
    }

    // Validate file type
    if (!ALLOWED_FILE_TYPES.includes(file.type)) {
      setErrorMessage("Ungültiger Dateityp. Erlaubt sind JPEG, PNG und PDF.");
      return;
    }

    setErrorMessage("");
    setUploading(true);
    await uploadFileInChunks(file);
    setUploading(false);
  };

  const uploadFileInChunks = async (file) => {
    const totalChunks = Math.ceil(file.size / CHUNK_SIZE);
    let uploadedChunks = 0;

    for (let start = 0; start < file.size; start += CHUNK_SIZE) {
      const chunk = file.slice(start, start + CHUNK_SIZE);

      // Simulate virus scanning and server upload
      const isSafe = await scanForViruses(chunk);
      if (!isSafe) {
        setErrorMessage("Die Datei hat den Virenscan nicht bestanden.");
        setUploading(false);
        return;
      }

      await uploadChunk(chunk, file.name);
      uploadedChunks++;
      setUploadProgress(Math.round((uploadedChunks / totalChunks) * 100));
    }
  };

  const scanForViruses = async (chunk) => {
    // Simulate a virus scan with a server call
    return new Promise((resolve) => {
      setTimeout(() => resolve(true), 500); // Mock virus scan result
    });
  };

  const uploadChunk = async (chunk, fileName) => {
    const formData = new FormData();
    formData.append("chunk", chunk);
    formData.append("fileName", fileName);

    // Mock API call to upload the chunk
    return new Promise((resolve) => {
      setTimeout(() => resolve(true), 500);
    });
  };

  return (
    <div
      onDrop={handleFileDrop}
      onDragOver={(e) => e.preventDefault()}
      style={{ border: "2px dashed #ccc", padding: "20px", textAlign: "center" }}
    >
      {uploading ? (
        <div>
          <p>Hochladen: {uploadProgress}%</p>
          <progress value={uploadProgress} max="100" />
        </div>
      ) : (
        <p>Ziehen Sie eine Datei hierher oder klicken Sie zum Hochladen.</p>
      )}
      {errorMessage && <p style={{ color: "red" }}>{errorMessage}</p>}
    </div>
  );
};

export default FileUpload;
