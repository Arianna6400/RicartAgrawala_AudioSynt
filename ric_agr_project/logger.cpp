// Gestione dei log (operazioni sui nodi)

#include "logger.h"
#include <iostream>

// File di log
std::ofstream Logger::log_file;

// Inizializza il file di log (apre il file)
void Logger::initialize_log(const std::string& file_path) {
    log_file.open(file_path, std::ios_base::app); // Apre il file in modalità append
    if (!log_file.is_open()) {
        std::cerr << "[ERROR] Failed to open log file!" << std::endl;
    }
}

// Logga l'invio di una richiesta
void Logger::log_request(int node_id, int clock) {
    if (log_file.is_open()) {
        log_file << "[LOG] Node " << node_id << " sending REQUEST with clock " << clock << std::endl;
    }
}

// Logga la ricezione di un ACK
void Logger::log_ack_received(int node_id) {
    if (log_file.is_open()) {
        log_file << "[LOG] Node " << node_id << " received ACK" << std::endl;
    }
}

// Logga quando un nodo entra nella sezione critica
void Logger::log_critical_section_entry(int node_id) {
    if (log_file.is_open()) {
        log_file << "[LOG] Node " << node_id << " entering critical section" << std::endl;
    }
}

// Logga quando un nodo esce dalla sezione critica
void Logger::log_critical_section_exit(int node_id) {
    if (log_file.is_open()) {
        log_file << "[LOG] Node " << node_id << " exiting critical section" << std::endl;
    }
}

// Chiude il file di log
void Logger::close_log() {
    if (log_file.is_open()) {
        log_file.close();
    }
}
