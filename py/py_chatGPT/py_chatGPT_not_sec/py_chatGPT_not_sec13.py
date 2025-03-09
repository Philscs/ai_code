import librosa
import numpy as np
from midiutil import MIDIFile

class MusicAnalyzer:
    def __init__(self, audio_file):
        self.audio_file = audio_file
        self.y = None
        self.sr = None
        self.notes = []
        self.chords = []

    def load_audio(self):
        print("Lade Audiodatei...")
        self.y, self.sr = librosa.load(self.audio_file, sr=None)
        print(f"Audiodatei geladen: {self.audio_file}")

    def analyze_notes(self):
        print("Analysiere Noten...")
        onset_frames = librosa.onset.onset_detect(y=self.y, sr=self.sr, backtrack=True)
        onset_times = librosa.frames_to_time(onset_frames, sr=self.sr)
        pitches, magnitudes = librosa.piptrack(y=self.y, sr=self.sr)

        for frame in onset_frames:
            pitch = pitches[:, frame].argmax()
            freq = librosa.hz_to_midi(librosa.midi_to_hz(pitch))
            if freq > 0:
                self.notes.append(freq)
        print(f"Gefundene Noten: {self.notes}")

    def identify_chords(self):
        print("Identifiziere Akkorde...")
        # Eine einfache Annäherung zur Akkorderkennung basierend auf Notenabständen
        for i in range(len(self.notes) - 2):
            chord = (self.notes[i], self.notes[i + 1], self.notes[i + 2])
            self.chords.append(chord)
        print(f"Gefundene Akkorde: {self.chords}")

    def create_midi(self, output_file):
        print("Erstelle MIDI-Datei...")
        midi = MIDIFile(1)
        midi.addTempo(0, 0, 120)

        for i, note in enumerate(self.notes):
            midi.addNote(0, 0, int(note), i * 0.5, 1, 100)

        with open(output_file, "wb") as f:
            midi.writeFile(f)
        print(f"MIDI-Datei erstellt: {output_file}")

if __name__ == "__main__":
    audio_path = input("Pfad zur Audiodatei eingeben: ")
    output_midi_path = "output.mid"

    analyzer = MusicAnalyzer(audio_path)
    analyzer.load_audio()
    analyzer.analyze_notes()
    analyzer.identify_chords()
    analyzer.create_midi(output_midi_path)
    print("Analyse abgeschlossen.")
