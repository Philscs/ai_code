import React, { useState, useCallback, useRef } from 'react';
import { Alert, AlertDescription } from '@/components/ui/alert';
import { Progress } from '@/components/ui/progress';
import { Upload, AlertCircle, CheckCircle2, XCircle } from 'lucide-react';

const CHUNK_SIZE = 1024 * 1024; // 1MB chunks
const MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB
const ALLOWED_TYPES = [
  'application/pdf',
  'image/jpeg',
  'image/png',
  'text/plain'
];

const FileUpload = () => {
  const [isDragging, setIsDragging] = useState(false);
  const [files, setFiles] = useState([]);
  const [error, setError] = useState('');
  const fileInputRef = useRef(null);

  const validateFile = (file) => {
    // Check file size
    if (file.size > MAX_FILE_SIZE) {
      throw new Error('Datei ist zu groß (Max: 100MB)');
    }

    // Check file type
    if (!ALLOWED_TYPES.includes(file.type)) {
      throw new Error('Dateityp nicht erlaubt');
    }

    // Validate filename for path traversal
    const sanitizedName = file.name.replace(/[^a-zA-Z0-9.-]/g, '_');
    if (sanitizedName !== file.name) {
      throw new Error('Dateiname enthält ungültige Zeichen');
    }

    return true;
  };

  const uploadChunk = async (chunk, filename, chunkIndex, totalChunks) => {
    const formData = new FormData();
    formData.append('chunk', chunk);
    formData.append('filename', filename);
    formData.append('chunkIndex', chunkIndex);
    formData.append('totalChunks', totalChunks);

    try {
      // Simulate upload delay for demonstration
      await new Promise(resolve => setTimeout(resolve, 500));
      return true;
    } catch (error) {
      console.error('Chunk upload failed:', error);
      throw error;
    }
  };

  const uploadFile = async (file) => {
    try {
      validateFile(file);
      
      const totalChunks = Math.ceil(file.size / CHUNK_SIZE);
      const fileEntry = {
        id: Date.now(),
        name: file.name,
        progress: 0,
        status: 'uploading'
      };
      
      setFiles(prev => [...prev, fileEntry]);

      for (let i = 0; i < totalChunks; i++) {
        const chunk = file.slice(i * CHUNK_SIZE, (i + 1) * CHUNK_SIZE);
        await uploadChunk(chunk, file.name, i, totalChunks);
        
        setFiles(prev => prev.map(f => 
          f.id === fileEntry.id 
            ? { ...f, progress: Math.round(((i + 1) / totalChunks) * 100) }
            : f
        ));
      }

      // Simulate virus scanning
      await new Promise(resolve => setTimeout(resolve, 1000));
      
      setFiles(prev => prev.map(f => 
        f.id === fileEntry.id 
          ? { ...f, status: 'completed' }
          : f
      ));

    } catch (error) {
      setFiles(prev => prev.map(f => 
        f.id === fileEntry.id 
          ? { ...f, status: 'error', error: error.message }
          : f
      ));
      setError(error.message);
    }
  };

  const handleDrop = useCallback((e) => {
    e.preventDefault();
    setIsDragging(false);
    setError('');

    const droppedFiles = Array.from(e.dataTransfer.files);
    droppedFiles.forEach(uploadFile);
  }, []);

  const handleDragOver = useCallback((e) => {
    e.preventDefault();
    setIsDragging(true);
  }, []);

  const handleDragLeave = useCallback((e) => {
    e.preventDefault();
    setIsDragging(false);
  }, []);

  const handleFileInput = useCallback((e) => {
    const selectedFiles = Array.from(e.target.files);
    selectedFiles.forEach(uploadFile);
    e.target.value = null;
  }, []);

  return (
    <div className="w-full max-w-2xl mx-auto p-6">
      <div
        className={`border-2 border-dashed rounded-lg p-8 text-center cursor-pointer transition-colors
          ${isDragging ? 'border-blue-500 bg-blue-50' : 'border-gray-300 hover:border-gray-400'}`}
        onDrop={handleDrop}
        onDragOver={handleDragOver}
        onDragLeave={handleDragLeave}
        onClick={() => fileInputRef.current?.click()}
      >
        <Upload className="mx-auto h-12 w-12 text-gray-400" />
        <p className="mt-2 text-gray-600">
          Dateien hierher ziehen oder klicken zum Auswählen
        </p>
        <p className="text-sm text-gray-500 mt-1">
          PDF, JPG, PNG oder TXT bis 100MB
        </p>
        <input
          ref={fileInputRef}
          type="file"
          className="hidden"
          multiple
          onChange={handleFileInput}
          accept={ALLOWED_TYPES.join(',')}
        />
      </div>

      {error && (
        <Alert variant="destructive" className="mt-4">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      <div className="mt-6 space-y-4">
        {files.map(file => (
          <div key={file.id} className="border rounded-lg p-4">
            <div className="flex items-center justify-between mb-2">
              <span className="font-medium">{file.name}</span>
              {file.status === 'completed' && (
                <CheckCircle2 className="h-5 w-5 text-green-500" />
              )}
              {file.status === 'error' && (
                <XCircle className="h-5 w-5 text-red-500" />
              )}
            </div>
            
            {file.status === 'uploading' && (
              <div className="space-y-2">
                <Progress value={file.progress} className="w-full" />
                <span className="text-sm text-gray-500">
                  {file.progress}% hochgeladen
                </span>
              </div>
            )}
            
            {file.status === 'error' && (
              <span className="text-sm text-red-500">{file.error}</span>
            )}
          </div>
        ))}
      </div>
    </div>
  );
};

export default FileUpload;