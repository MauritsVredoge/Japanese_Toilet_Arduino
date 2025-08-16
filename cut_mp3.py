from pydub import AudioSegment
import os
from tkinter import Tk, filedialog, simpledialog, messagebox

def split_mp3_with_fades():
    # Setup GUI
    root = Tk()
    root.withdraw()

    # Select input MP3 file
    input_file = filedialog.askopenfilename(
        title="Select MP3 file",
        filetypes=[("MP3 files", "*.mp3")]
    )
    if not input_file:
        messagebox.showinfo("Cancelled", "No input file selected.")
        return

    # Select output folder
    output_folder = filedialog.askdirectory(title="Select output folder for chunks")
    if not output_folder:
        messagebox.showinfo("Cancelled", "No output folder selected.")
        return

    # Ask for chunk length in seconds
    chunk_seconds = simpledialog.askinteger("Chunk length", "Enter chunk length in seconds:", minvalue=2)
    if not chunk_seconds:
        messagebox.showinfo("Cancelled", "No chunk length provided.")
        return

    chunk_length_ms = chunk_seconds * 1000
    fade_duration_ms = min(7000, chunk_length_ms // 2)  # Cap fade at half of chunk

    try:
        audio = AudioSegment.from_mp3(input_file)
        total_length = len(audio)
        chunk_count = 0

        for i in range(0, total_length, chunk_length_ms):
            chunk = audio[i:i + chunk_length_ms]

            # Apply fade in and fade out
            chunk = chunk.fade_in(fade_duration_ms).fade_out(fade_duration_ms)

            chunk_name = os.path.join(output_folder, f"chunk_{chunk_count:04d}.mp3")
            chunk.export(chunk_name, format="mp3")
            print(f"Exported {chunk_name}")
            chunk_count += 1

        messagebox.showinfo("Done", f"Exported {chunk_count} chunks with fades.")
    except Exception as e:
        messagebox.showerror("Error", str(e))

# Run
if __name__ == "__main__":
    split_mp3_with_fades()
