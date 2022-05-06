#ifndef PVP_H
#define PVP_H

#include <string>

/* PVP_H */

typedef struct connecter {
    std::string ip;
    int port;
} connect_t;

// Connection Functions
connect_t my_connection();
connect_t get_connection(std::string enc_connection);
std::string get_connect_str(connect_t connection);
std::string connect_enc(std::string in);
std::string connect_dec(std::string in);
std::string check_in(std::string in);

// Server / Client Functions
void server_driver(connect_t connection);
void client_driver(connect_t connection);

#endif