# RicartAgrawala_AudioSynt

## Setup e Esecuzione del Progetto

### Creazione dell'ambiente Conda
1. Crea un ambiente con Python 3.9:
   ```bash
   conda create --name ric_agr_project python==3.9

2. Attiva l'ambiente:
    ```bash
    conda activate ric_agr_project

### Installazione delle dipendenze
1. Spostarsi nella cartella corretta:
    ```bash
    cd ric_agr_project/audio_synthesizer

2. Installa le dipendenze:
    ```bash
    pip install -r requirements.txt

### Download Modello

Per usare il sintetizzatore vocale, bisogna scaricare il modello:

```bash
python -m TTS.utils.download --model_name "tts_models/en/ljspeech/tacotron2-DDC"
```

### Esecuzione

1. Runnare il file ```synthesizer.py```. Per personalizzare il contenuto, cambiare il contenuto della variabile ```text``` all'interno delle parentesi.

2. Il risultato, in formato ```WAV```, sarà visibile all'interno della cartella ```output_audio```.

