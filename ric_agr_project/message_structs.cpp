// Funzioni di serializzazione dei messaggi

#include "message_structs.h"
#include <sstream>
#include <iostream>

// Funzione per serializzare un messaggio
std::string serialize_message(const Message& msg) {
    std::ostringstream oss;
    // Serializza il tipo del messaggio (conversione in int per l'enum)
    oss << static_cast<int>(msg.type) << " " << msg.sender_id << " " 
        << msg.logical_clock << " " << msg.deadline_ms;
    return oss.str();  // Restituisce il messaggio serializzato come stringa
}

// Funzione per deserializzare un messaggio da una stringa
Message deserialize_message(const std::string& str) {
    std::istringstream iss(str);
    int type, sender_id, logical_clock, deadline;
    // Deserializza i campi dalla stringa
    iss >> type >> sender_id >> logical_clock >> deadline;
    // Crea e restituisce il messaggio deserializzato
    return Message(static_cast<MessageType>(type), sender_id, logical_clock, deadline);
}
