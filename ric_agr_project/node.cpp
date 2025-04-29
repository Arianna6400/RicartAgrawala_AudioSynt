// Nodo che implementa Ricart-Agrawala, includendo le funzioni
// dichiarate nel node.h

#include "node.h"

Node::Node(int id, int port)
    : id_(id), port_(port), clock_(0), requesting_(false), ack_count_(0) {
    network_ = std::make_unique<Network>(port_);
}

void Node::start() {
    // Avvia il server per la comunicazione con altri nodi
    std::thread server_thread(&Network::start_server, network_.get());
    server_thread.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Aspetta che parta il server

    // Simulazione delle richieste periodiche di accesso alla traccia
    for (int i=0; i<5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // RIchiedi ogni 5 secondi
        request_critical_section();
    }
}

void Node::request_critical_section() {
    requesting_ = true;
    clock_++;
    std::cout << "[Node" << id_ << "] Sending REQUEST with clock: " << clock_ << std::endl;

    // Invia la richiesta a tutti gli altri nodi
    for (int i=0; i<3; ++i) {
        if (i != id_) {
            std::string message = "REQUEST" + std::to_string(id_) + " " + std::to_string(clock_);
            network_->send_message(i, message);
        }
    }

    // Aspetta che tutti gli ACK siano ricevuti
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] {return ack_count_ == 2; }); //  Aspetta di ricevere 2 ACK
    enter_critical_section();
}

void Node::enter_critical_section() {
    std::cout << "[Node " << id_ << "] Entering critical section..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));  // Simula l'elaborazione di una risorsa (in questo caso traccia audio)
    release_critical_section();
}

void Node::release_critical_section() {
    requesting_ = false;
    clock_++;
    std::cout << "[Node " << id_ << "] Sending RELEASE with clock: " << clock_ << std::endl;

    // Rilascia la risorsa (cioè la tracccia) inviando RELEASE a tutti gli altri nodi
    for (int i = 0; i < 3; ++i) {
        if (i != id_) {
            std::string message = "RELEASE " + std::to_string(id_) + " " + std::to_string(clock_);
            network_->send_message(i, message);
        }
    }
}

void Node::send_message(int target_node, const std::string& message) {
    // Funzione per inviare un messaggio (implementa la logica di invio)
    network_->send_message(target_node, message);
}

void Node::receive_message(const std::string& message) {
    // Gestisce i messaggi ricevuti (REQUEST, ACK, RELEASE)
    std::cout << "[Node " << id_ << "] Received message: " << message << std::endl;
    
    // Elabora la logica per ciascun tipo di messaggio
    if (message.find("REQUEST") == 0) {
        // Invia ACK se il nodo non sta usando la traccia
        ack_count_++;
        std::string ack_message = "ACK " + std::to_string(id_) + " " + std::to_string(clock_);
        network_->send_message(id_, ack_message);
    } else if (message.find("RELEASE") == 0) {
        // Rilascia la traccia (se il nodo è in sezione critica)
        std::cout << "[Node " << id_ << "] Released the critical section." << std::endl;
    }
}