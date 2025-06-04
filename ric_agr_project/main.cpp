#include "node.h"               
#include "logger.h"             
#include "network.h"            
#include <thread>               // Per la gestione dei thread
#include <vector>               // Per l'uso del contenitore std::vector
#include <fstream>              // Per la lettura/scrittura su file
#include <iostream>             // Per input/output standard
#include <nlohmann/json.hpp>    // Libreria JSON header-only per parsing e serializzazione
#include <unistd.h>             // Libreria POSIX (non usata direttamente qui)

using json = nlohmann::json;   

int main() {
    Logger::initialize_log("logs.txt");  

    std::string config_path = "config.json";     // Percorso al file di configurazione JSON
    std::ifstream ifs(config_path);              // Apertura del file in lettura
    if (!ifs.is_open()) {                        // Controllo apertura file
        std::cerr << "Errore: impossibile aprire " << config_path << std::endl;
        return 1;                                // Termina il programma con errore
    }

    json config_json;        
    ifs >> config_json;       // Parsing del contenuto JSON

    // Controllo che il campo "num_nodes" esista e sia un intero
    if (!config_json.contains("num_nodes") || !config_json["num_nodes"].is_number_integer()) {
        std::cerr << "Errore: num_nodes non valido o mancante\n";
        return 1;
    }

    int num_nodes = config_json["num_nodes"]; 

    // Controllo che il campo "nodes" esista e sia un array
    if (!config_json.contains("nodes") || !config_json["nodes"].is_array()) {
        std::cerr << "Errore: campo nodes mancante o non array\n";
        return 1;
    }

    std::vector<std::unique_ptr<Node>> nodes;  // Contenitore per tutti i nodi creati
    std::vector<std::thread> threads;          // Contenitore per tutti i thread associati ai nodi

    // Iterazione sull'array JSON "nodes" per creare ogni nodo
    for (const auto& node_json : config_json["nodes"]) {
        int id = node_json["id"];             
        std::string host = node_json["host"]; 
        int port = node_json["port"];         

        // Creazione dinamica di un oggetto Node
        auto node = std::make_unique<Node>(id, host, port, num_nodes);

        // Avvio del metodo start() del nodo in un nuovo thread
        threads.emplace_back(&Node::start, node.get());

        // Salvataggio del nodo nel vettore
        nodes.push_back(std::move(node));
    }

    // Breve attesa per permettere l’inizializzazione dei thread
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Attende la terminazione di tutti i thread lanciati
    for (auto& t : threads) {
        if (t.joinable())     
            t.join();        
    }

    Logger::close_log();     

    return 0;                
}
