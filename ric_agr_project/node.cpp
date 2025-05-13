#include "node.h"

Node::Node(int id, int port)
    : id_(id), port_(port), clock_(0),
      requesting_(std::make_shared<std::atomic<bool>>(false)),
      ack_count_(std::make_shared<std::atomic<int>>(0)),
      mtx_(std::make_shared<std::mutex>()),
      cv_(std::make_shared<std::condition_variable>()) {
    network_ = std::make_unique<Network>(port_);
    network_->set_receive_callback([this](const std::string& msg) {
        this->receive_message(msg);
    });
}

void Node::start() {
    // Avvia il server per la comunicazione con altri nodi
    std::thread server_thread(&Network::start_server, network_.get());
    server_thread.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Aspetta che parta il server

    // Simulazione delle richieste periodiche di accesso alla traccia
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Richiedi ogni 5 secondi
        request_critical_section();
    }
}

void Node::request_critical_section() {
    requesting_->store(true);
    clock_++;
    std::cout << "[Node " << id_ << "] Sending REQUEST with clock: " << clock_ << std::endl;

    // Invia la richiesta a tutti gli altri nodi
    for (int i = 0; i < 3; ++i) {
        if (i != id_) {
            std::string message = "REQUEST " + std::to_string(id_) + " " + std::to_string(clock_);
            network_->send_message(i, message);
        }
    }

    // Aspetta che tutti gli ACK siano ricevuti
    std::unique_lock<std::mutex> lock(*mtx_);
    cv_->wait(lock, [this] { return ack_count_->load() == 2; }); // Aspetta di ricevere 2 ACK
    enter_critical_section();
}

void Node::enter_critical_section() {
    std::cout << "[Node " << id_ << "] Entering critical section..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));  // Simula l'elaborazione di una risorsa (in questo caso traccia audio)
    std::cout << "[Node " << id_ << "] AAAAAAAAAAAA" << std::endl;
    release_critical_section();
}

void Node::release_critical_section() {
    requesting_->store(false);
    clock_++;
    std::cout << "[Node " << id_ << "] Sending RELEASE with clock: " << clock_ << std::endl;

    // Rilascia la risorsa (cioè la traccia) inviando RELEASE a tutti gli altri nodi
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
        ack_count_->fetch_add(1);
        std::string ack_message = "ACK " + std::to_string(id_) + " " + std::to_string(clock_);
        //network_->send_message(id_, ack_message);
    } else if (message.find("RELEASE") == 0) {
        // Rilascia la traccia (se il nodo è in sezione critica)
        std::cout << "[Node " << id_ << "] Released the critical section." << std::endl;
    }
}

void Node::test_message_exchange() {
    // Simula l'invio di una richiesta
    std::string request_message = "REQUEST " + std::to_string(id_) + " " + std::to_string(clock_);
    network_->send_message((id_ + 1) % 3, request_message);  // Invia a un altro nodo
    network_->send_message((id_ + 2) % 3, request_message);  // Invia anche al terzo nodo

    // Simula la ricezione e la risposta con un ACK
    std::string received_message = "REQUEST " + std::to_string(id_) + " " + std::to_string(clock_);
    receive_message(received_message);  // Simula la ricezione di un messaggio
}
