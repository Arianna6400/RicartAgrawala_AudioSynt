#include "node.h"
#include "logger.h" // Logger incluso per l'utilizzo delle funzioni di log
#include "message_structs.h"
#include <random>

// Definizione del mutex statico
//std::mutex Node::cout_mtx;

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
    //my_request_ts_ = clock_;

    /*{
        std::lock_guard<std::mutex> lk(cout_mtx);
        std::cout << "[LOG] Node " << id_
                  << " sending REQUEST with clock "
                  << my_request_ts_ << std::endl;
    }*/

    Logger::log_request(id_, my_request_ts_);  // Logga la richiesta

    // **Reset ACK count**
    //ack_count_->store(0);
    //deferred_acks_.clear();

    // Serializza il messaggio REQUEST
    std::string message = serialize_message(Message(MessageType::REQUEST, id_, my_request_ts_, 0));
    
    // Invia il messaggio serializzato a tutti gli altri nodi
    for (int i = 0; i < 3; ++i) {
        if (i != id_) {
            network_->send_message(i, message);
        }
    }

    // Aspetta che tutti gli ACK siano ricevuti
    std::unique_lock<std::mutex> lock(*mtx_);
    cv_->wait(lock, [this] { return ack_count_->load() == 2; }); // Aspetta di ricevere 2 ACK
    /*std::lock_guard<std::mutex> lk(cout_mtx);
    std::cout << "[DEBUG Node " << id_ << "] Trying to enter CS with ack_count=" 
              << ack_count_->load() 
              << ", deferred=" << deferred_acks_.size() 
              //<< ", my_req_ts=" << my_request_ts_
              << ", my_req_ts=" << clock_ 
              << "\n";*/
    enter_critical_section();
}

void Node::enter_critical_section() {
    Logger::log_critical_section_entry(id_);  // Logga l'ingresso nella sezione critica
    std::cout << "[Node " << id_ << "] Entering critical section..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));  // Simula l'elaborazione di una risorsa (in questo caso traccia audio)
    std::cout << "[Node " << id_ << "] AAAAAAAAAAAA" << std::endl;
    release_critical_section();
}

/*void Node::send_ack(int to) {
    std::string ack = serialize_message(
        Message{MessageType::ACK, id_, clock_, 0}
    );
    {
        std::lock_guard<std::mutex> lk(cout_mtx);
      std::cout << "[DEBUG Node "<<id_<<"] Sending deferred ACK to "<<to
                <<" @clk="<<clock_<<"\n";
    }
    network_->send_message(to, ack);
}*/

void Node::release_critical_section() {
    requesting_->store(false);
    clock_++;
    // Invia deferred **prima** di qualsiasi altra cosa
    /*for (int pid : deferred_acks_) {
        send_ack(pid);
    }
    deferred_acks_.clear();*/
    Logger::log_critical_section_exit(id_);  // Logga l'uscita dalla sezione critica
    std::cout << "[Node " << id_ << "] Sending RELEASE with clock: " << clock_ << std::endl;

    // Serializza il messaggio RELEASE e lo invia a tutti gli altri nodi
    for (int i = 0; i < 3; ++i) {
        if (i != id_) {
            std::string message = serialize_message(Message(MessageType::RELEASE, id_, clock_, 0));
            network_->send_message(i, message);
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

void Node::test_message_exchange() {
    // Simula l'invio di una richiesta
    std::string request_message = serialize_message(Message(MessageType::REQUEST, id_, clock_, 0));
    network_->send_message((id_ + 1) % 3, request_message);  // Invia a un altro nodo
    network_->send_message((id_ + 2) % 3, request_message);  // Invia anche al terzo nodo

    // Simula la ricezione e la risposta con un ACK
    std::string received_message = serialize_message(Message(MessageType::REQUEST, id_, clock_, 0));
    receive_message(received_message);  // Simula la ricezione di un messaggio
}
