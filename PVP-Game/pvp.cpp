#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <curses.h>

#include <stdlib.h>
#include <string>
#include <sstream>

#include "pvp.h"
#include "io.h"
#include "poke327.h"

// Server
#define MAX 80
// #define PORT 8080
#define SAS struct sockaddr
#define SAC struct sockaddr




std::string connect_enc(std::string in)
{
    char c;
    std::string ret;

    for(int i = 0; i < in.size(); i++)
    {
        if (in[i] == '\n') break;
        switch(c = in[i])
        {
            case '0':
                ret += 'a';
                break;
            case '1':
                ret += 'b';
                break;
            case '2':
                ret += 'c';
                break;
            case '3':
                ret += 'd';
                break;
            case '4':
                ret += 'e';
                break;
            case '5':
                ret += 'f';
                break;
            case '6':
                ret += 'g';
                break;
            case '7':
                ret += 'h';
                break;
            case '8':
                ret += 'i';
                break;
            case '9':
                ret += 'j';
                break;
            case '.':
                ret += 'y';
                break;
            default:
                ret += 'z';
        }
    }
    ret += 'x';
    if(rand() % 2) ret += 'p';
    return ret;
}

std::string connect_dec(std::string in)
{
    char c;
    std::string ret;

    for(int i = 0; i < in.size(); i++)
    {
        if (in[i] == '\n') break;
        switch(c = in[i])
        {
            case 'a':
                ret += '0';
                break;
            case 'b':
                ret += '1';
                break;
            case 'c':
                ret += '2';
                break;
            case 'd':
                ret += '3';
                break;
            case 'e':
                ret += '4';
                break;
            case 'f':
                ret += '5';
                break;
            case 'g':
                ret += '6';
                break;
            case 'h':
                ret += '7';
                break;
            case 'i':
                ret += '8';
                break;
            case 'j':
                ret += '9';
                break;
            case 'y':
                ret += '.';
                break;
            case 'x':
            case 'p':
                break;
            default:
                ret += ' ';
        }
    }
    return ret;
}

connect_t my_connection()
{
    connect_t connection;

    char hostbuffer[256];
    gethostname(hostbuffer, sizeof(hostbuffer));
    struct hostent *host_entry = gethostbyname(hostbuffer);
    std::string ip = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));
    int port = 6666;

    // Search for first open port
    // while(port < 0) {
    //     port = socket(AF_INET, SOCK_STREAM, 0);
    // }

    connection.ip = ip;
    connection.port = port;

    return connection;
}

connect_t get_connection(std::string enc_connection)
{
    std::string connect_str = connect_dec(enc_connection);
    connect_t ret;
    std::string ip;

    for(int i = 0; i < connect_str.size(); i++) {
        if(connect_str[i] != ' ') ip += connect_str[i];
        else {
            std::string t_port;
            while(i < connect_str.size()) {
                t_port += connect_str[i];
                i++;
            }
            ret.port = atoi(t_port.c_str());
        }
    }
    ret.ip = ip;

    return ret;
}

std::string get_connect_str(connect_t connection)
{
    std::string final;
    final = connection.ip;
    final += " ";
    final += std::to_string(connection.port);
    return final;
}

std::string check_in(std::string in)
{
    std::string ret;
    char c;
    for(int i = 0; i < in.size(); i++) {
        switch(c = in[i])
        {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'y':
            case 'z':
            case 'x':
            case 'p':
                ret += c;
                break;
            default:
                break;
        }
    }
    return ret;
}

std::string clean_buff(char *buff)
{
    std::string ret;

    for(int i = 0; i < sizeof(buff); i++)
    {
        if(buff[i] == '.') return ret;
        ret += buff[i];
    }

    return buff;
}

void wait_for_connection()
{

}

void connect_delay(int *c)
{
    *c = -1;
}

std::string player_data()
{
    // Pokemon Display Data: name, gender, level, hp, max hp
    std::string pd = world.pc.captured[world.pc.curr_pokemon]->get_species();
    pd += ',';
    pd += world.pc.captured[world.pc.curr_pokemon]->get_gender_string();
    pd += ",";
    pd += std::to_string(world.pc.captured[world.pc.curr_pokemon]->get_lvl());
    pd += ",";
    pd += std::to_string(world.pc.captured[world.pc.curr_pokemon]->get_hp());
    pd += ",";
    pd += std::to_string(world.pc.captured[world.pc.curr_pokemon]->get_max_hp());
    return pd;
}

PVP_C parse_player(std::string in)
{
    int i = 0;
    PVP_C player;
    std::stringstream ss(in);
    std::string substr;

    while(ss.good()) {
        getline(ss, substr, ',');
        if(i == 0) player.message = substr;
        if(i == 1) player.name = substr;
        if(i == 2) player.gender = substr;
        if(i == 3) player.level = atoi(substr.c_str());
        if(i == 4) player.hp = atoi(substr.c_str());
        if(i == 5) player.max_hp = atoi(substr.c_str());
        if(i == 6) world.pc.captured[world.pc.curr_pokemon]->damage(atoi(substr.c_str()));
        i++;
    }


    return player;
}

void pvp_battle(int sockfd, bool server = false)
{
    set_global_reset(20);
    noecho();
    char buff[MAX];
    std::string pd, ret_move;
    PVP_C enemy;

    world.pc.curr_pokemon = 0;

    while(world.pc.captured[world.pc.curr_pokemon]->get_hp() <= 0) {
        world.pc.curr_pokemon++;
    }

    if (server) {
        pd = player_data();
        pd = "WELCOME TO PVP!," + pd;
        pd += ".";
        write(sockfd, pd.c_str(), pd.size()-1);
        io_display_pvp_battle("Opponent has the first move.");
    }

    while(true) {
        bzero(buff, sizeof(buff));

        read(sockfd, buff, sizeof(buff));
        
        enemy = parse_player(clean_buff(buff));
        pd = player_data();

        if(enemy.message != "exit") {
            set_global_reset(-1);
            io_display_pvp_battle(&enemy);
            if(world.pc.captured[world.pc.curr_pokemon]->get_hp() <= 0) {
                std::string death_note;
                death_note = world.pc.captured[world.pc.curr_pokemon]->get_species();
                death_note += " fainted.  Choose a Pokemon to send in.";
                while(world.pc.captured[world.pc.curr_pokemon]->get_hp() <= 0) {
                    world.pc.curr_pokemon++;
                    if(world.pc.curr_pokemon > 5) break;
                }
                if(world.pc.curr_pokemon >= 6) pd = "exit,You lost.";
                else {
                    io_display_pvp_battle(death_note);
                    ret_move = io_get_move(world.pc.captured[world.pc.curr_pokemon]);
                    while(!(ret_move.find("Go") != std::string::npos)) {
                        ret_move = io_get_move(world.pc.captured[world.pc.curr_pokemon]);
                    }
                    io_display_pvp_battle(ret_move);
                    ret_move = "Opponent sent out ";
                    ret_move += world.pc.captured[world.pc.curr_pokemon]->get_species();
                    pd = ret_move + "," + player_data();
                }
            } else {
                ret_move = io_get_move(NULL);
                if(ret_move.find("Away") != std::string::npos) pd = "exit," + pd;
                else pd = ret_move + "," + pd;
                if(ret_move.find("used") != std::string::npos) { 
                    pd = pd + "," + std::to_string(world.pc.captured[world.pc.curr_pokemon]->get_atk());
                    enemy.hp -= world.pc.captured[world.pc.curr_pokemon]->get_atk();
                }
            }
            pd += ".";
            enemy.message = "What will opponenet do?";
        }
        else {
            write(sockfd, "exit.", strlen("exit."));
            set_global_reset(-1);
            io_display_pvp_battle("Your opponent left the match.");
            return;
        }

        set_global_reset(20);
        io_display_pvp_battle(&enemy);

        write(sockfd, pd.c_str(), pd.size()-1);
    }
    set_global_reset(-1);
}

/// GeeksforGeeks https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/

// Driver function
void server_driver(connect_t connection)
{
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
        io_display_msg("Unable to create socket.  Try again.");
        return;
	}
	else
		io_display_msg("Socket successfully created..");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(connection.port);

	// Binding newly created socket to given IP and verification
    while((bind(sockfd, (SAS*)&servaddr, sizeof(servaddr))) != 0) {
        io_display_msg("Unable to bind socket.  Try again.");
        connection.port++;
        servaddr.sin_port = htons(connection.port);
    }
    io_display_msg("Successfully bound socket.");
    
    std::string reconnect_msg = connect_enc(get_connect_str(connection));
    reconnect_msg = "(" + reconnect_msg + ") Waiting for opponent.";

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
        io_display_msg("Unable to listen for opponent.  Try again.");
        return;
	}
	else
		io_display_msg(reconnect_msg);
	len = sizeof(cli);


    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SAS*)&cli, (socklen_t *) &len);
    if (connfd < 0) {
        io_display_msg("Unable to connect.  Try again.");
        return;
    }
    else
        // printf("server accept the client...\n");
        io_display_msg("Opponent has connected.");

    timeout(0);

    // Start Battle
    pvp_battle(connfd, true);

    // After chatting close the socket
    close(sockfd);
}

// Driver Function
void client_driver(connect_t connection)
{
	int sockfd; // connfd
	struct sockaddr_in servaddr;    // cli

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		// printf("Socket creation failed.\n");
		// exit(0);
        io_display_msg("Unable to create socket.  Try again.");
        return;
	}
	else
		io_display_msg("Socket successfully created.  Waiting for server.");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(connection.ip.c_str());
	servaddr.sin_port = htons(connection.port);

	// connect the client socket to server socket
	if (connect(sockfd, (SAC*)&servaddr, sizeof(servaddr)) != 0) {
		// printf("connection with the server failed...\n");
		// exit(0);
        io_display_msg("Unable to connect to opponent.  Try again.");
        return;
	}
	else
		io_display_msg("Connection Successful!  Ready to Battle!\n");

	// Start Battle
    pvp_battle(sockfd);

	// close the socket
	close(sockfd);
}