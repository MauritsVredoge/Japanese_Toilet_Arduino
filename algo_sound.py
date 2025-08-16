import numpy as np
import sounddevice as sd
import time

# Global parameters
SAMPLE_RATE = 44100  # Hz
NOTE_DURATION = 0.8  # seconds
VOLUME = 0.3

def note_to_freq(note):
    A4 = 440
    semitones = {'C': 0, 'C#':1, 'D':2, 'D#':3, 'E':4, 'F':5, 'F#':6,
                 'G':7, 'G#':8, 'A':9, 'A#':10, 'B':11}
    flat_to_sharp = {'Db': 'C#', 'Eb': 'D#', 'Fb': 'E', 'Gb': 'F#', 'Ab': 'G#', 'Bb': 'A#', 'Cb': 'B'}

    if len(note) > 1 and note[-1].isdigit():
        octave = int(note[-1])
        key = note[:-1]
    else:
        octave = 4
        key = note

    key = flat_to_sharp.get(key, key)

    if key not in semitones:
        raise ValueError(f"Unknown note: {note}")

    key_number = semitones[key] + (octave - 4) * 12
    return A4 * 2 ** (key_number / 12)


def play_note(note, duration=NOTE_DURATION):
    if note == 'R':  # Rest
        time.sleep(duration)
        return
    frequency = note_to_freq(note)
    t = np.linspace(0, duration, int(SAMPLE_RATE * duration), False)
    wave = np.sin(frequency * t * 2 * np.pi)
    envelope = np.exp(-3 * t) 
    wave *= envelope
    audio = VOLUME * wave
    sd.play(audio, samplerate=SAMPLE_RATE)
    sd.wait()

melody = [
    ["D4", "B3", "A3", "G3"], ["D4", "B3", "A3", "G3"],
    ["E4", "C4", "B3", "A3"], ["E4", "C4", "B3", "A3"],
    ["D4", "B3", "A3", "G3"], ["D4", "B3", "A3", "G3"],
    ["C4", "A3", "G3", "F3"], ["C4", "A3", "G3", "F3"],
    ["D4", "B3", "A3", "G3"], ["D4", "B3", "A3", "G3"],
    ["E4", "C4", "B3", "A3"], ["E4", "C4", "B3", "A3"],
    ["D4", "B3", "A3", "G3"], ["D4", "B3", "A3", "G3"],
    ["C4", "A3", "G3", "F3"], ["C4", "A3", "G3", "F3"],

    # Gymnopédie 2-inspired phrase
    ["A3", "B3", "C4", "D4"], ["C4", "B3", "A3", "G3"],
    ["F4", "D4", "C4", "B3"], ["A3", "G3", "F3", "E3"],

    # Gnossienne No. 1-inspired theme
    ["E4", "G4", "F4", "E4"], ["D4", "C4", "A3", "G3"],
    ["F4", "E4", "D4", "C4"], ["B3", "A3", "G3", "F3"]
]

# Chords left-hand
chords = [
    "D3", "D3", "C3", "C3", "D3", "D3", "Bb2", "Bb2",
    "D3", "D3", "C3", "C3", "D3", "D3", "Bb2", "Bb2",
    "A2", "D3", "G2", "C3",
    "F2", "D3", "Bb2", "G2"
]


def play_gymnopedie():
    for measure, notes in enumerate(melody):
        # Play chord in left hand
        play_note(chords[measure], duration=2 * NOTE_DURATION)
        # Play melody (right hand)
        for n in notes:
            play_note(n)

if __name__ == "__main__":
    play_gymnopedie()
