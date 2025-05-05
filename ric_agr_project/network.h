#pragma once

#include <functional>
#include <string>
#include <vector>
#include <tuple>

class Network {
public:
    // Costruisce il modulo di rete e carica la configurazione dei peer
    explicit Network(int port, const std::string& config_path = "config.json");

    // Imposta la callback da chiamare quando arriva un messaggio
    void set_receive_callback(std::function<void(const std::string&)> cb);

    // Avvia il server TCP per ricevere messaggi
    void start_server();

    // Invia un messaggio al nodo target (specificato da ID)
    void send_message(int target_id, const std::string& message);

private:
    int port_;
    std::vector<std::tuple<int, std::string, int>> peers_;  // (node_id, host, port)
    std::function<void(const std::string&)> recv_cb_;

    // Carica la configurazione dei peer da file JSON
    void load_config(const std::string& config_path);
};