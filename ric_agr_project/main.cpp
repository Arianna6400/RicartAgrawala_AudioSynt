#include "node.h"
#include "network.h"
#include <thread>
#include <vector>

int main() {
    // Leggi la configurazione da config.json
    // Carica il file di configurazione
    std::string config_path = "config.json";
    
    Network network(8080);  // Nodo server
    network.load_config(config_path);  // Carica i nodi dal file di configurazione
    
    // Creazione dei nodi, passando l'ID e la porta (caricati dal config.json)
    std::vector<Node> nodes;
    for (int i = 0; i < 3; ++i) {
        int port = 8080 + i;  // Usa una porta diversa per ogni nodo
        nodes.push_back(Node(i, port));  // Crea un nodo con ID e porta specifica
    }

    // Crea i thread per far partire i nodi (simula il comportamento concorrente)
    std::vector<std::thread> threads;
    for (auto& node : nodes) {
        threads.push_back(std::thread(&Node::start, &node));  // Ogni nodo parte in un thread separato
    }

    // Testare la comunicazione tra i nodi (simula l'invio dei messaggi)
    for (auto& node : nodes) {
        node.test_message_exchange();  // Ogni nodo invia una richiesta di esempio
    }

    // Aspetta che tutti i thread siano completati
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
