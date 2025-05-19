#include "node.h"
#include "logger.h"
#include "network.h"
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>  // libreria JSON header-only
#include <unistd.h>

using json = nlohmann::json;

int main() {
    Logger::initialize_log("logs.txt");

    std::string config_path = "config.json";
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        std::cerr << "Errore: impossibile aprire " << config_path << std::endl;
        return 1;
    }

    json config_json;
    ifs >> config_json;

    if (!config_json.contains("num_nodes") || !config_json["num_nodes"].is_number_integer()) {
        std::cerr << "Errore: num_nodes non valido o mancante\n";
        return 1;
    }

    int num_nodes = config_json["num_nodes"];

    if (!config_json.contains("nodes") || !config_json["nodes"].is_array()) {
        std::cerr << "Errore: campo nodes mancante o non array\n";
        return 1;
    }

    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::thread> threads;

    for (const auto& node_json : config_json["nodes"]) {
        int id = node_json["id"];
        std::string host = node_json["host"];
        int port = node_json["port"];

        auto node = std::make_unique<Node>(id, host, port, num_nodes);
        threads.emplace_back(&Node::start, node.get());
        nodes.push_back(std::move(node));
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }

    Logger::close_log();

    return 0;
}
