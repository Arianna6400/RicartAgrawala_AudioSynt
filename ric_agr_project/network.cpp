#include "network.h"              
#include <nlohmann/json.hpp>      // Libreria per la gestione dei file JSON
#include <fstream>                // Per operazioni di lettura/scrittura file
#include <iostream>               // Per output su console
#include <thread>                 // Per gestione dei thread
#include <mutex>                  // Mutex (non usato direttamente in questo file)
#include <sys/socket.h>           // API per socket
#include <arpa/inet.h>            // Funzioni per indirizzi IP
#include <unistd.h>               // Funzioni POSIX (close, read, etc.)

using json = nlohmann::json;     

// Costruttore: inizializza la porta e carica la configurazione dei peer dal file
Network::Network(int port, const std::string& config_path)
    : port_(port)
{
    load_config(config_path);
}

// Imposta la callback da chiamare ogni volta che viene ricevuto un messaggio
void Network::set_receive_callback(std::function<void(const std::string&)> cb) {
    recv_cb_ = std::move(cb);
}

// Carica i peer (altri nodi) dal file di configurazione JSON
void Network::load_config(const std::string& config_path) {
    std::ifstream in(config_path);    // Apre il file
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }
    json j;
    in >> j;                          // Parsing del contenuto JSON

    // Itera su ogni nodo specificato nel file
    for (auto& node : j["nodes"]) {
        int id = node["id"];
        std::string host = node["host"];
        int port = node["port"];

        // Esclude se stesso dalla lista dei peer
        if (port != port_) {
            peers_.emplace_back(id, host, port);
            std::cout << "[Network" << port_ << "] Loaded peer: ID=" << id << ", host=" << host << ", port=" << port << std::endl;
        } else {
            std::cout << "[Network" << port_ << "] Skipping self: ID=" << id << ", host=" << host << ", port=" << port << std::endl;
        }
    }
}

// Avvia il server TCP per ricevere messaggi
void Network::start_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);  // Crea un socket TCP
    if (server_fd == -1) {
        perror("socket");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;               // Accetta connessioni da qualsiasi IP
    addr.sin_port = htons(port_);                    // Imposta la porta

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {                   // Mette il socket in ascolto
        perror("listen");
        close(server_fd);
        return;
    }

    std::cout << "Network: server listening on port " << port_ << std::endl;

    // Loop infinito per accettare nuove connessioni
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);  // Accetta una connessione

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // Crea un thread per gestire il client connesso
        std::thread([this, client_fd]() {
            constexpr size_t BUF_SIZE = 1024;
            char buffer[BUF_SIZE];
            ssize_t len;
            std::string message;

            // Legge i dati finché il client è connesso
            while ((len = recv(client_fd, buffer, BUF_SIZE, 0)) > 0) {
                message.append(buffer, len);
                auto pos = message.find('\n'); // Cerca fine messaggio

                // Gestione di messaggi multipli separati da newline
                while (pos != std::string::npos) {
                    std::string line = message.substr(0, pos);
                    if (recv_cb_) recv_cb_(line);  // Chiamata alla callback
                    message.erase(0, pos + 1);
                    pos = message.find('\n');
                }
            }

            close(client_fd); // Chiude la connessione con il client
        }).detach();          // Detach del thread: viene eseguito in background
    }

    close(server_fd); // (mai raggiunta, ma buono per completezza)
}

// Invia un messaggio TCP al nodo specificato tramite target_id
void Network::send_message(int target_id, const std::string& message) {
    // Cerca il peer nella lista
    auto it = std::find_if(peers_.begin(), peers_.end(), [&](const std::tuple<int, std::string, int>& tup) {
        return std::get<0>(tup) == target_id;
    });

    if (it == peers_.end()) {
        std::cerr << "Network: peer " << target_id << " not found\n";
        std::cout << "[Network] peers_ size = " << peers_.size() << std::endl;
        std::cout << "[Network] peers_ = " << std::endl;
        for (const auto& [id, host, port] : peers_) {
            std::cout << "  - ID=" << id << ", host=" << host << ", port=" << port << "\n";
        }
        return;
    }

    std::string host = std::get<1>(*it); // Hostname/IP del peer
    int port = std::get<2>(*it);         // Porta del peer

    int sock = socket(AF_INET, SOCK_STREAM, 0); // Crea socket TCP
    if (sock < 0) {
        perror("socket");
        return;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);    // Porta del destinatario

    // Converte l'indirizzo IP da stringa a binario
    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return;
    }

    // Connessione al peer
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return;
    }

    // Aggiunge newline per indicare la fine del messaggio
    std::string msg = message + "\n";
    send(sock, msg.c_str(), msg.size(), 0);  // Invia il messaggio

    // Chiude il socket dopo l'invio
    close(sock);
}
