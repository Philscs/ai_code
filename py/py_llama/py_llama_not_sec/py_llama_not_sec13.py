import librosa
import numpy as np

def noten_erkennen(audio_datei):
    # Lade die Audio-Datei
    audio, sr = librosa.load(audio_datei)

    # Durchführen der Frequenzanalyse
    spectrogramme = np.abs(librosa.stft(audio))

    # Definieren der Noten und ihrer entsprechenden Frequenzen
    noten = {
        'C': 261.63,
        'D': 293.66,
        'E': 329.63,
        'F': 349.23,
        'G': 392.00,
        'A': 440.00,
        'B': 493.88
    }

    # Suche nach den Noten in der Frequenzanalyse
    noten_in_spektrogramm = []
    for freq in spectrogramme[0]:
        if freq > 200:  # Beachte, dass die Frequenzanalyse mit einer Schwellung von 200 beginnt
            # Suche nach der nächsten Note, die größer als die aktuelle ist
            next_note = None
            for note in noten:
                if spectrogramme[0][note] > freq:
                    next_note = note
                    break

            if next_note is not None:
                noten_in_spektrogramm.append((freq, noten[next_note]))

    return noten_in_spektrogramm
import music21

def akkorde_identifizieren(noten_in_spektrogramm):
    # Erstellen eines Musikstücks aus den Noten
    song = music21.stream.Stream()
    for freq, note in noten_in_spektrogramm:
        pitch = music21.pitch.Pitch(freq)
        duration = 1.0  # Definiere die Dauer des Akkords
        note = music21.note.Note(pitch=pitch, quarterLength=duration)
        song.append(note)

    # Analyse des Musikstücks
    akkord = music21.algorithms.correlation_correlator(song)
    return akkord
import mido

def midi_ausgabe(noten_in_spektrogramm, akkorde_identifiziert):
    # Erstellen einer MIDI-Datei
    midi = mido.MidiFile()
    track = mido.MidiTrack()
    for i in range(len(noten_in_spektrogramm)):
        note = noten_in_spektrogramm[i][1]
        track.append(mido.Message('note_on', note=note, velocity=64))
        track.append(mido.Message('note_off', note=note, velocity=64))

    if akkorde_identifiziert is not None:
        akkord = akkorde_identifiziert
        for i in range(len(akkord)):
            track.append(mido.Message('program_change', program=akkord[i]))

    midi.tracks.append(track)

    # Speichern der MIDI-Datei
    with open('ausgabe.mid', 'wb') as f:
        midi.save(f)
