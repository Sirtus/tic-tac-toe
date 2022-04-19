#include "server.h"


int main(int argc, char** argv){ 
    srand(time(NULL));
    atexit(exit_funct);
    get_input(argc, argv);
    for(int i = 0; i < MAX_CONNECTIONS; ++i) {
        clients[i].socket_fd = -1;
        games[i].player1.socket_fd = -1;
        games[i].player2.socket_fd = -1;
    }
    signal(SIGINT, exit_funct );
    init_server();
    connect_client();  

    return 0;
}
