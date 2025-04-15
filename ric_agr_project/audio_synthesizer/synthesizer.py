from TTS.api import TTS # API di Mozilla Text-To-Speech
import os
import soundfile as sf

# Funzione per generare tracce vocali
def generate_audio(text, output_filename):
    # Carica il modello
    tts = TTS(model_name="tts_models/en/ljspeech/tacotron2-DDC", gpu=False)
    
    # Sintetizza la traccia
    wav = tts.tts(text)
    
    # Salva l'audio in un file WAV utilizzando soundfile
    sf.write(output_filename, wav, 22050)  # 22050 è la frequenza di campionamento di default
    print(f"Audio generato: {output_filename}")

# Testo da sintetizzare
text = "Hello, this is a track for our distributed system!"

# Creazione del file audio
output_filename = os.path.join("output_audio", "output_0.wav")
os.makedirs("output_audio", exist_ok=True)

generate_audio(text, output_filename)
