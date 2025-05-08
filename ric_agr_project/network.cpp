#include "network.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

using json = nlohmann::json;

Network::Network(int port, const std::string& config_path)
    : port_(port)
{
    load_config(config_path);
}

void Network::set_receive_callback(std::function<void(const std::string&)> cb) {
    recv_cb_ = std::move(cb);
}

void Network::load_config(const std::string& config_path) {
    std::ifstream in(config_path);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }
    json j;
    in >> j;
    for (auto& node : j["nodes"]) {
        int id = node["id"];
        std::string host = node["host"];
        int port = node["port"];
        // Escludi se stesso
        if (port != port_) {
            peers_.emplace_back(id, host, port);
        }
    }
}

void Network::start_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    std::cout << "Network: server listening on port " << port_ << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        // Thread per gestire ogni connessione
        std::thread([this, client_fd]() {
            constexpr size_t BUF_SIZE = 1024;
            char buffer[BUF_SIZE];
            ssize_t len;
            // Leggi fino a newline
            std::string message;
            while ((len = recv(client_fd, buffer, BUF_SIZE, 0)) > 0) {
                message.append(buffer, len);
                auto pos = message.find('\n');
                while (pos != std::string::npos) {
                    std::string line = message.substr(0, pos);
                    if (recv_cb_) recv_cb_(line);
                    message.erase(0, pos + 1);
                    pos = message.find('\n');
                }
            }
            close(client_fd);
        }).detach();
    }
    close(server_fd);
}

void Network::send_message(int target_id, const std::string& message) {
    // Trova host e porta del peer
    auto it = std::find_if(peers_.begin(), peers_.end(), [&](const std::tuple<int, std::string, int>& tup) {
        return std::get<0>(tup) == target_id;
    });    
    if (it == peers_.end()) {
        std::cerr << "Network: peer " << target_id << " not found\n";
        return;
    }
    std::string host = std::get<1>(*it);
    int port = std::get<2>(*it);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return;
    }

    std::string msg = message + "\n";
    send(sock, msg.c_str(), msg.size(), 0);
    close(sock);
}