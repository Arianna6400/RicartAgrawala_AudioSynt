#include "node.h"
#include "logger.h" // Logger incluso per l'utilizzo delle funzioni di log
#include "message_structs.h"
#include <random>

// Definizione del mutex statico
//std::mutex Node::cout_mtx;

Node::Node(int id, const std::string& host, int port, int num_nodes)
    : id_(id), host_(host), port_(port), clock_(0), num_nodes_(num_nodes),
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

    // Prepara il generator di numeri casuali
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 5);

    // Simulazione delle richieste periodiche di accesso alla traccia
    for (int i = 0; i < 5; ++i) {
        int delay = dist(gen);
        std::this_thread::sleep_for(std::chrono::seconds(delay)); // Richiedi ogni 5 secondi
        request_critical_section();
    }
}

void Node::request_critical_section() {
    requesting_->store(true);
    {
        std::lock_guard<std::mutex> lk(clock_mtx_);
        clock_++;
        my_request_ts_ = clock_;  // 🔥 fondamentale
    }

    Logger::log_request(id_, my_request_ts_);  // Logga la richiesta

    // **Reset ACK count**
    //ack_count_->store(0);
    //deferred_acks_.clear();

    // Serializza il messaggio REQUEST
    std::string message = serialize_message(Message(MessageType::REQUEST, id_, my_request_ts_, 0));
    
    // Invia il messaggio serializzato a tutti gli altri nodi
    for (int i = 0; i < num_nodes_; ++i) {
        if (i != id_) {
            network_->send_message(i, message);
        }
    }

    // Aspetta che tutti gli ACK siano ricevuti
    std::unique_lock<std::mutex> lock(*mtx_);
    cv_->wait(lock, [this] { return ack_count_->load() == num_nodes_ -1; }); // Aspetta di ricevere num_nodes - 1 ACK
    enter_critical_section();
}

void Node::enter_critical_section() {
    Logger::log_critical_section_entry(id_);  // Logga l'ingresso nella sezione critica
    std::cout << "[Node " << id_ << "] Entering critical section..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));  // Simula l'elaborazione di una risorsa (in questo caso traccia audio)
    std::cout << "[Node " << id_ << "] Entered in critical section!!!" << std::endl;
    release_critical_section();
}

void Node::release_critical_section() {
    requesting_->store(false);
    clock_++;
    
    Logger::log_critical_section_exit(id_);  // Logga l'uscita dalla sezione critica
    std::cout << "[Node " << id_ << "] Sending RELEASE with clock: " << clock_ << std::endl;

    // Serializza il messaggio RELEASE e lo invia a tutti gli altri nodi
    std::string release_message = serialize_message(Message(MessageType::RELEASE, id_, clock_, 0));
    for (int i = 0; i < num_nodes_; ++i) {
        if (i != id_) {
            network_->send_message(i, release_message);
        }
    }

    // Invia gli ACK differiti
    for (int deferred_id : deferred_acks_) {
        std::string ack_message = serialize_message(Message(MessageType::ACK, id_, clock_, 0));
        network_->send_message(deferred_id, ack_message);
    }
    deferred_acks_.clear();
    ack_count_->store(0);
}

void Node::send_message(int target_node, const std::string& message) {
    // Funzione per inviare un messaggio (implementa la logica di invio)
    network_->send_message(target_node, message);
}

void Node::receive_message(const std::string& message) {
    // Deserializza il messaggio ricevuto
    Message received_msg = deserialize_message(message);

    // Logga il messaggio ricevuto
    std::cout << "[Node " << id_ << "] Received message: " << message << std::endl;

    // Elabora la logica per ciascun tipo di messaggio
    if (received_msg.type == MessageType::REQUEST) {
        // Aggiorna clock
        {
            std::lock_guard<std::mutex> clock_lock(clock_mtx_);
            clock_ = std::max(clock_, received_msg.logical_clock) + 1;
        }
    
        bool defer_ack = false;
    
        {
            std::lock_guard<std::mutex> lock(*mtx_);
            // Decide se deferire l'ACK
            if (requesting_->load()) {
                if (received_msg.logical_clock < my_request_ts_ ||
                    (received_msg.logical_clock == my_request_ts_ && received_msg.sender_id < id_)) {
                    // Il messaggio ha priorità: non deferire
                    defer_ack = false;
                } else {
                    defer_ack = true;
                }
            }
        }
    
        if (defer_ack) {
            deferred_acks_.push_back(received_msg.sender_id);
        } else {
            std::string ack_message = serialize_message(Message(MessageType::ACK, id_, clock_, 0));
            network_->send_message(received_msg.sender_id, ack_message);
        }
    }else if (received_msg.type == MessageType::ACK) {
        {
            std::lock_guard<std::mutex> clock_lock(clock_mtx_);
            clock_ = std::max(clock_, received_msg.logical_clock) + 1;
        }

        ack_count_->fetch_add(1);
        cv_->notify_all();
    }
}

