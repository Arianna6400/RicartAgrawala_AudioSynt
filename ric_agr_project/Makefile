# Nome dell'eseguibile finale
TARGET = node_simulator

# Compilatore C++
CXX = g++

# Flags di compilazione
CXXFLAGS = -std=c++17 -Wall -g

# Directory dei file sorgenti
SRC_DIR = .

# Directory di output per gli oggetti compilati
OBJ_DIR = build

# File oggetto (.o) per ogni file sorgente (.cpp)
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Librerie necessarie (se necessarie)
LIBS = -lsndfile

# Comando per creare la directory di output
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Installazione delle dipendenze
install_deps:
	# Verifica e installa libsndfile
	@echo "Verifica se libsndfile è installata..."
	@dpkg -s libsndfile1-dev || sudo apt-get install -y libsndfile1-dev
	# Verifica e installa nlohmann/json
	@echo "Verifica se nlohmann/json è installata..."
	@dpkg -s nlohmann-json3-dev || sudo apt-get install -y nlohmann-json3-dev
	# Verifica e installa VLC
	@echo "Verifica se VLC è installato..."
	@dpkg -s vlc || sudo apt-get install -y vlc

# Regola di compilazione per il target finale (eseguibile)
$(TARGET): $(OBJ_DIR) $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

# Regola per compilare i file oggetto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pulizia dei file oggetto e dell'eseguibile
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Esegui il programma
run: $(TARGET)
	./$(TARGET)
