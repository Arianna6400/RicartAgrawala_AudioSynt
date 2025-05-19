// Funzioni del nodo. Contiene le dichiarazioni di funzioni e variabili
// necessarie per la gestione di ogni nodo.

#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory> // per gestire gli oggetti non copiabili
#include <thread>
#include "network.h"

class Node{
public:
    // Costruttore del nodo
    Node(int id, const std::string& host, int port, int num_nodes);

    // Funzione per avviare il nodo
    void start();

    // Funzione per richiedere l'accesso alla sezione critica
    void request_critical_section();

    // Funzione per entrare nella sezione critica 
    void enter_critical_section();

    // Funzione per uscire dalla sezione critica
    void release_critical_section();

    // Funzione per inviare i messaggi
    void send_message(int target_node, const std::string& message);

    // Funzione per ricevere i messaggi
    void receive_message(const std::string& message);

private:
    int id_;    // ID del nodo
    std::string host_; // Host del nodo
    int port_;  // Porta di comunicazione
    int num_nodes_; // Numero totale nodi
    int clock_; // Clock logico per Ricart-Agrawala
    std::shared_ptr<std::atomic<bool>> requesting_; // Flag per la richiesta
    std::shared_ptr<std::atomic<int>> ack_count_;  // Contatore degli ACK ricevuti
    std::shared_ptr<std::mutex> mtx_;              // Mutex per la sincronizzazione
    std::shared_ptr<std::condition_variable> cv_;  // Condizione per la sincronizzazione
    std::unique_ptr<Network> network_;  // Riferimento alla rete di comunicazione
    std::vector<int> deferred_acks_;
    int my_request_ts_{0};
    std::mutex clock_mtx_;
};

#endif // NODE_H