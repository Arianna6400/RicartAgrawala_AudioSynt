// Interfaccia del logger

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>  // Per scrivere su file

class Logger {
public:
    // Funzione per inizializzare il file di log
    static void initialize_log(const std::string& file_path);

    // Funzioni per loggare eventi specifici
    static void log_request(int node_id, int clock);
    static void log_ack_received(int node_id);
    static void log_critical_section_entry(int node_id);
    static void log_critical_section_exit(int node_id);

    // Funzione per chiudere il file di log
    static void close_log();

private:
    static std::ofstream log_file;  // File di log (scrittura su file)
};

#endif // LOGGER_H
