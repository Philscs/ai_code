import librosa
import librosa.display
import numpy as np
import pretty_midi
from typing import List, Tuple, Dict
import scipy.signal

class MusicAnalyzer:
    def __init__(self, sample_rate: int = 22050):
        self.sample_rate = sample_rate
        self.note_frequencies = {
            'C': 261.63, 'C#': 277.18, 'D': 293.66,
            'D#': 311.13, 'E': 329.63, 'F': 349.23,
            'F#': 369.99, 'G': 392.00, 'G#': 415.30,
            'A': 440.00, 'A#': 466.16, 'B': 493.88
        }
        
    def load_audio(self, file_path: str) -> Tuple[np.ndarray, float]:
        """
        Lädt und verarbeitet eine Audiodatei.
        """
        y, sr = librosa.load(file_path, sr=self.sample_rate)
        return y, sr

    def detect_pitch(self, y: np.ndarray) -> np.ndarray:
        """
        Erkennt die Tonhöhen im Audiosignal.
        """
        pitches, magnitudes = librosa.piptrack(
            y=y,
            sr=self.sample_rate,
            fmin=librosa.note_to_hz('C2'),
            fmax=librosa.note_to_hz('C7')
        )
        return pitches[magnitudes > np.max(magnitudes) * 0.1]

    def frequency_to_note(self, frequency: float) -> str:
        """
        Konvertiert eine Frequenz in einen Notennamen.
        """
        if frequency <= 0:
            return None
            
        note_names = list(self.note_frequencies.keys())
        note_freqs = list(self.note_frequencies.values())
        
        # Oktavenanpassung
        while frequency < note_freqs[0]:
            frequency *= 2
        while frequency > note_freqs[-1]:
            frequency /= 2
            
        # Finde die nächstgelegene Note
        distances = [abs(frequency - f) for f in note_freqs]
        index = distances.index(min(distances))
        return note_names[index]

    def detect_chords(self, notes: List[str]) -> List[str]:
        """
        Identifiziert Akkorde aus einer Liste von Noten.
        """
        chord_patterns = {
            'major': [0, 4, 7],      # Dur
            'minor': [0, 3, 7],      # Moll
            'dim': [0, 3, 6],        # Vermindert
            'aug': [0, 4, 8],        # Übermäßig
            '7': [0, 4, 7, 10],      # Dominantseptakkord
            'maj7': [0, 4, 7, 11],   # Großer Septakkord
            'min7': [0, 3, 7, 10]    # Kleiner Septakkord
        }
        
        # Konvertiere Noten in numerische Werte (0-11)
        note_values = []
        for note in notes:
            base_note = note.replace('#', '').upper()
            value = list(self.note_frequencies.keys()).index(base_note)
            if '#' in note:
                value += 1
            note_values.append(value % 12)
            
        detected_chords = []
        for i in range(len(note_values)):
            for pattern_name, pattern in chord_patterns.items():
                chord_found = True
                root = note_values[i]
                for interval in pattern:
                    if (root + interval) % 12 not in note_values:
                        chord_found = False
                        break
                if chord_found:
                    root_note = list(self.note_frequencies.keys())[root]
                    detected_chords.append(f"{root_note}{pattern_name}")
                    
        return list(set(detected_chords))  # Entferne Duplikate

    def create_midi(self, notes: List[Tuple[str, float, float]], output_file: str):
        """
        Erstellt eine MIDI-Datei aus erkannten Noten.
        
        Args:
            notes: Liste von Tupeln (Notenname, Startzeit, Dauer)
            output_file: Ausgabepfad für die MIDI-Datei
        """
        pm = pretty_midi.PrettyMIDI()
        piano = pretty_midi.Instrument(program=0)  # Piano
        
        for note_name, start_time, duration in notes:
            # Konvertiere Notenname in MIDI-Notennummer
            note_number = pretty_midi.note_name_to_number(note_name)
            note = pretty_midi.Note(
                velocity=100,
                pitch=note_number,
                start=start_time,
                end=start_time + duration
            )
            piano.notes.append(note)
            
        pm.instruments.append(piano)
        pm.write(output_file)

    def analyze_file(self, input_file: str, output_midi: str) -> Dict:
        """
        Hauptfunktion zur Analyse einer Audiodatei.
        """
        # Audio laden
        y, sr = self.load_audio(input_file)
        
        # Tonhöhen erkennen
        pitches = self.detect_pitch(y)
        
        # Konvertiere Frequenzen in Noten
        notes = [self.frequency_to_note(f) for f in pitches if f > 0]
        notes = [n for n in notes if n is not None]
        
        # Erkenne Akkorde
        chords = self.detect_chords(notes)
        
        # Erstelle MIDI-Datei
        # Vereinfache die Notendauer für dieses Beispiel
        note_events = [(note, idx * 0.5, 0.5) for idx, note in enumerate(notes)]
        self.create_midi(note_events, output_midi)
        
        # Rückgabe der Analyseergebnisse
        return {
            'detected_notes': notes,
            'detected_chords': chords,
            'midi_file': output_midi
        }

# Beispielverwendung
if __name__ == "__main__":
    analyzer = MusicAnalyzer()
    results = analyzer.analyze_file("input_audio.wav", "output.mid")
    print("Erkannte Noten:", results['detected_notes'])
    print("Erkannte Akkorde:", results['detected_chords'])
    print("MIDI-Datei erstellt:", results['midi_file'])