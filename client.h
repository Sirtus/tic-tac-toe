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

#include "shared.h"

char ip[20] ;//"127.06.04.02";
int port ;
char server_path[100];
char client_name[MAX_CLIENT_NAME_LEN];

typedef enum {UNIX, INET} con_mode_t;

int socket_fd;
con_mode_t mode; 
client_t client;
int game_id;
int is_result;

void exit_function();
void init_client();
void register_client();
void play_the_game();
int valide(int field, sign_type board[9]);
void* ping(void* arg);
void get_input(int argc, char** argv);