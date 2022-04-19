#include<stdio.h>
#include<pthread.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/un.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<time.h>
#include<signal.h>
#include<netinet/in.h>
#include<poll.h>
#include<errno.h>
#include "shared.h"

#ifndef POLLIN
#define POLLIN 0x001
#endif

#define MAX_CONNECTIONS 10



int unix_socket;
int inet_socket;
char server_path[100];
int port;

struct sockaddr_un server_ux_addr;
struct sockaddr_un client_ux_addr;
struct sockaddr_in server_in_addr;
struct sockaddr_in client_in_addr;

client_t clients[MAX_CONNECTIONS];
game_t games[MAX_CONNECTIONS];
int pings[MAX_CONNECTIONS];

void exit_funct();
void init_server();
void connect_client();
void* client_function(void*);
void play_game(int game_idx);
void do_movement(response_t r);
int check_result(sign_type board[9], int index);
void disconnect(response_t r, int fd_to_close);
void* ping(void* arg);
void get_input(int argc, char** argv);