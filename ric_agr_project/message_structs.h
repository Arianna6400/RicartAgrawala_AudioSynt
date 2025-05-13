// Definizioni dei messaggi (REQUEST, ACK, RELEASE)

#ifndef MESSAGE_STRUCTS_H
#define MESSAGE_STRUCTS_H

#include <string>
#include <tuple>

enum class MessageType {
    REQUEST,  // Richiesta di accesso alla traccia
    ACK,      // Risposta (acknowledgement)
    RELEASE   // Rilascio della traccia
};

// Struttura di un messaggio
struct Message {
    MessageType type;     // Tipo del messaggio (REQUEST, ACK, RELEASE)
    int sender_id;        // ID del nodo che invia il messaggio
    int logical_clock;    // Clock logico del nodo (per Ricart-Agrawala)
    int deadline_ms;      // Deadline associata al messaggio (se applicabile)

    // Costruttore della struttura
    Message(MessageType t, int id, int clock, int deadline)
        : type(t), sender_id(id), logical_clock(clock), deadline_ms(deadline) {}
};

// Funzione per serializzare un messaggio in una stringa
std::string serialize_message(const Message& msg);

// Funzione per deserializzare una stringa in un messaggio
Message deserialize_message(const std::string& str);

#endif // MESSAGE_STRUCTS_H
