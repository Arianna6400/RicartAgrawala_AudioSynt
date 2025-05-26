from TTS.api import TTS # API di Mozilla Text-To-Speech
import os
import soundfile as sf
import sys
import urllib.parse

# Funzione per generare tracce vocali
def generate_audio(text, output_filename):
    # Carica il modello
    tts = TTS(model_name="tts_models/en/ljspeech/tacotron2-DDC", gpu=False)
    
    # Sintetizza la traccia
    wav = tts.tts(text)
    
    # Salva l'audio in un file WAV utilizzando soundfile
    sf.write(output_filename, wav, 22050)  # 22050 è la frequenza di campionamento di default
    print(f"Audio generato: {output_filename}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 synthesizer.py <node_id> <text>")
        sys.exit(1)

    node_id = sys.argv[1]
    encoded_text = sys.argv[2]
    text = urllib.parse.unquote(encoded_text)

    os.makedirs("output_audio", exist_ok=True)
    output_path = os.path.join("output_audio", f"output_{node_id}.wav")
    generate_audio(text, output_path)
