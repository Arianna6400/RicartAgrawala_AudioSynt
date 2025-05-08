# RicartAgrawala_AudioSynt

## Parte 1: Setup e Esecuzione del Sintetizzatore

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

## Parte 2: Build e Run dell'Algoritmo

1. Posizionarsi sulla cartella contenente il ```Makefile```:

```bash
cd ric_agr_project
```

2. Installare le dipendenze:

```bash
make install_deps
```

3. Compilare per buildare l'eseguibile:

```bash
make
```

4. Eseguire il file ```node_simulator```:

```bash
make run
```

5. (Opzionale) Pulire una volta finito:

```bash
make clean
```