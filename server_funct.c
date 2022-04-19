#include "server.h"

socklen_t unix_addr_len = sizeof(struct sockaddr_un);
socklen_t inet_addr_len = sizeof(struct sockaddr_in);



pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;


void get_input(int argc, char** argv){
    if(argc < 2){
        printf("Błędna liczba argumentów\n");
        exit(0);        
    }
    port = atoi(argv[1]);
    strcpy(server_path, argv[2]);
}

void init_server(){
    if((unix_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
        perror("socket - unix");
    if((inet_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
        perror("socket - inet");
    
    memset(&server_ux_addr, 0, sizeof(server_ux_addr));
    memset(&client_ux_addr, 0, sizeof(client_ux_addr));
    memset(&server_in_addr, 0, sizeof(server_in_addr));
    memset(&client_in_addr, 0, sizeof(client_in_addr));
    
    server_ux_addr.sun_family = AF_UNIX;
    strcpy(server_ux_addr.sun_path, server_path);

    server_in_addr.sin_family = AF_INET;
    
    server_in_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_in_addr.sin_port = htons(port);


    if(bind(unix_socket, (struct sockaddr*) &server_ux_addr, unix_addr_len) == -1)
        perror("unix binding");
    if(bind(inet_socket, (struct sockaddr*) &server_in_addr, inet_addr_len) == -1)
        perror("inet binding");
    

    if(listen(unix_socket, MAX_CONNECTIONS) == -1) perror("unix listen");
    if(listen(inet_socket, MAX_CONNECTIONS) == -1) perror("inet listen");
    
}   

void connect_client(){
    struct pollfd sockets[2];
    sockets[0].fd = unix_socket;
    sockets[0].events = POLLIN;
    sockets[1].fd = inet_socket;
    sockets[1].events = POLLIN;
    client_t new_client;
    while(1){
        poll(sockets, 2, -1);
        pthread_mutex_lock(&server_mutex);
        if(sockets[0].revents & POLLIN){
            sockets[0].events = POLLIN;
            new_client.socket_fd = accept(unix_socket, NULL, NULL);
            if(new_client.socket_fd == -1) {
                if(new_client.socket_fd == EAGAIN || new_client.socket_fd == EWOULDBLOCK) 
                    perror("unix accept");
                pthread_mutex_unlock(&server_mutex);
            }
            else{
                if(pthread_create(&new_client.thread_id, NULL, client_function, &new_client));
            }
        }
        else{
            pthread_mutex_unlock(&server_mutex);
        }
        pthread_mutex_lock(&server_mutex);
        if(sockets[1].revents & POLLIN) {
            new_client.socket_fd = accept(inet_socket, NULL, NULL);
            if(new_client.socket_fd == -1) {
                if(new_client.socket_fd == EAGAIN || new_client.socket_fd == EWOULDBLOCK) 
                    perror("inet accept");
                pthread_mutex_unlock(&server_mutex);
            }
            else{
                if(pthread_create(&new_client.thread_id, NULL, client_function, &new_client));
            }
        }
        else{
            pthread_mutex_unlock(&server_mutex);
        }
    }
}

void* client_function(void* arg){
    client_t client;
    client.socket_fd = ((client_t*) arg)->socket_fd;
    client.thread_id = ((client_t*) arg)->thread_id;

    pthread_t ping_thr;

    
    pthread_mutex_unlock(&server_mutex);

    struct pollfd pol;
    pol.fd = client.socket_fd;
    pol.events = POLLIN;
    
    while(1){
        response_t response;
        poll(&pol, 1, -1);

        if(pol.revents&POLLIN){
            if(recv(client.socket_fd, (void*) &response,sizeof(response), MSG_WAITALL ) == -1)
                perror("recv");
            switch(response.type){
                case REGISTER:
                    printf("registration %s\n",response.name);
                    pthread_mutex_lock(&client_mutex);
                    for(int i = 0; i < MAX_CONNECTIONS; ++i){
                        if(clients[i].socket_fd < 0){
                            clients[i].socket_fd = client.socket_fd;
                            clients[i].thread_id = client.thread_id;
                            clients[i].array_id = i;
                            strcpy(clients[i].name, response.name);
                            strcpy(client.name, response.name);
                            client.array_id = i;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&client_mutex);
                    break;
                
                case PLAY:
                    pthread_mutex_lock(&game_mutex);
                    pthread_create(&ping_thr, NULL, ping, &client);
                    for(int i = 0; i < MAX_CONNECTIONS; ++i){
                        if(games[i].player2.socket_fd < 0){
                            if(games[i].player1.socket_fd < 0){
                                games[i].player1.socket_fd = client.socket_fd;
                                games[i].player1.array_id = client.array_id;
                                strcpy(games[i].player1.name, client.name);
                                response_t res;
                                res.type = WAIT;
                                strcpy(res.msg, "Poczekaj aż dołączy twój przeciwnik\n");
                                write(client.socket_fd, (void*)&res, sizeof(response_t));
                                pthread_mutex_unlock(&game_mutex);
                            }
                            else {
                                games[i].player2.socket_fd = client.socket_fd;
                                games[i].player2.array_id = client.array_id;
                                strcpy(games[i].player2.name, client.name);
                                response_t res;
                                res.type = WAIT;
                                strcpy(res.msg, "Gra zaraz się rozpocznie\n");
                                write(client.socket_fd, (void*)&res, sizeof(response_t));
                                write(games[i].player1.socket_fd, (void*)&res, sizeof(response_t));
                                pthread_mutex_unlock(&game_mutex);
                                play_game(i);
                            }
                            break;
                        }     
                    }
                    break;
                case MOVEMENT:
                    do_movement(response);
                    break;
                case DISCONNECT:
                    disconnect(response, client.socket_fd);
                    break;
                case PING:
                    pings[client.array_id] = 1;
                    break;
                default:
                    break;
            }
            fsync(client.socket_fd);
        }
        pol.revents = -1;
    }
    close(client.socket_fd);
    return NULL;
}

void* ping(void* arg){

    client_t client;
    client.array_id =  ((client_t*)arg)->array_id;
    client.socket_fd =  ((client_t*)arg)->socket_fd;
    pings[client.array_id] = 1;
    response_t response;
    response.type = PING;

    while(pings[client.array_id]){
        pings[client.array_id] = 0;
        write(client.socket_fd, (void*)&response, sizeof(response_t));
        sleep(10);
    }
    // DISCONNECT
    response_t r;
    r.game_id = -1;
    for(int i = 0; i < MAX_CONNECTIONS; ++i){
        if(games[i].player1.socket_fd == client.socket_fd || games[i].player2.socket_fd == client.socket_fd){
            r.game_id = i;
            break;
        }
    }
    r.client_arr_id = client.array_id;

    disconnect(r, client.socket_fd);
    return NULL;
}

void disconnect(response_t r, int fd_to_close){
    
    response_t response;
    int oponent_fd;
    if(r.game_id != -1){
        
        if(r.client_arr_id == games[r.game_id].player1.array_id) {
            response.client_arr_id = games[r.game_id].player2.array_id;
            oponent_fd = games[r.game_id].player2.socket_fd;
        }
        else {
            response.client_arr_id = games[r.game_id].player1.array_id;
            oponent_fd = games[r.game_id].player1.socket_fd;
        }

        strcpy(response.msg, "Koniec rozgrywki, twój przeciwnik opóścił spotkanie\n");
        response.type = RESULT;

        write(oponent_fd, (void*) &response, sizeof(response_t));
    }


    memset(games+r.game_id, 0, sizeof(game_t));
    games[r.game_id].player1.socket_fd = -1;
    games[r.game_id].player2.socket_fd = -1;

    close(fd_to_close);
}

void do_movement(response_t r){
    response_t response;
    response.game_id = r.game_id;
    games[response.game_id].board[r.field] = r.sign;
    int is_winner = check_result(games[response.game_id].board, r.field);
    if(!is_winner){
        for(int i = 0; i < 9; ++i){
            char field[10];
            if(games[r.game_id].board[i] == O) sprintf(field, " O |");
            else if(games[r.game_id].board[i] == X) sprintf(field, " X |");
            else if(games[r.game_id].board[i] == NONE) sprintf(field, " %d |",i);
            response.board[i] = games[r.game_id].board[i];
            if((i+1) % 3 == 0 ) strcat(field, "\n");
            strcat(response.msg, field);
        }
        response.type = (r.client_arr_id == games[r.game_id].player1.array_id) ? WAIT : MOVEMENT;
        response.client_arr_id = (r.client_arr_id == games[r.game_id].player1.array_id) ?  games[r.game_id].player2.array_id : games[r.game_id].player1.array_id;
        write(games[r.game_id].player1.socket_fd, (void*) &response, sizeof(response_t));
        response.type = (response.type == WAIT) ? MOVEMENT : WAIT;
        write(games[r.game_id].player2.socket_fd, (void*) &response, sizeof(response_t));
        memset(&response, 0, sizeof(response_t));
    }
    else{
        response.type = RESULT;
        if(r.client_arr_id == games[r.game_id].player1.array_id){
            strcpy(response.msg, "Wygrałeś\n");
        }
        else{
            strcpy(response.msg, "Przegrałeś\n");
        }
        write(games[r.game_id].player1.socket_fd, (void*) &response, sizeof(response_t));
        if(r.client_arr_id == games[r.game_id].player2.array_id){
            strcpy(response.msg, "Wygrałeś\n");
        }
        else{
            strcpy(response.msg, "Przegrałeś\n");
        }
        write(games[r.game_id].player2.socket_fd, (void*) &response, sizeof(response_t));

        memset(games+r.game_id, 0, sizeof(game_t));
        games[r.game_id].player1.socket_fd = -1;
        games[r.game_id].player2.socket_fd = -1;
    }
}

void play_game(int game_idx){
    int first = rand()%2;
    int sign = rand()%2;
    response_t response, sign_res;
    sign_res.sign = (sign == 0) ? O : X;
    sign_res.type = PLAY;
    sign_res.game_id = game_idx;
    write(games[game_idx].player1.socket_fd, (void*) &sign_res, sizeof(response_t));
    sign_res.sign = (sign == 0) ? X : O;
    write(games[game_idx].player2.socket_fd, (void*) &sign_res, sizeof(response_t));

    char msg[MSG_MAX_LEN];
    //char names[MAX_CLIENT_NAME_LEN * 2];
    sprintf(msg, "Pojedynek %s vs %s\n",games[game_idx].player1.name, games[game_idx].player2.name);

    for(int i = 0; i < 9; ++i){
        char field[10];
        games[game_idx].board[i] = NONE;
        response.board[i] = NONE;
        sprintf(field, " %d |",i);
        if((i+1) % 3 == 0 ) strcat(field, "\n");
        strcat(msg, field);
    }
    char msg_cd[MAX_CLIENT_NAME_LEN + 20];
    if(first == 0) {
        response.type = MOVEMENT;
        sprintf(msg_cd, "Zaczyna %s\n",games[game_idx].player1.name);
        response.client_arr_id = games[game_idx].player1.array_id;
    }
    else {
        response.type = WAIT;
        sprintf(msg_cd, "Zaczyna %s\n",games[game_idx].player2.name);
        response.client_arr_id = games[game_idx].player2.array_id;
    }
    strcat(msg, msg_cd);
    strcpy(response.msg, msg);
    response.game_id = game_idx;
    write(games[game_idx].player1.socket_fd, (void*) &response, sizeof(response_t));
    response.type = (response.type == WAIT) ? MOVEMENT : WAIT;
    write(games[game_idx].player2.socket_fd, (void*) &response, sizeof(response_t));
    
}

int check_result(sign_type board[9], int index){
    int mod = index % 3;
    switch(mod){
        case 0:
            if(board[index+1] == board[index] && board[index+2] == board[index]) return 1;
            break;
        case 1:
            if(board[index-1] == board[index] && board[index+1] == board[index]) return 1;
            break;
        case 2:
            if(board[index-1] == board[index] && board[index-2] == board[index]) return 1;
            break;
    }
    int is_winner = 1;
    for(int i = mod; i < 9; i+=3) {
        if(board[i] != board[index]) is_winner = 0;
    }
    if(is_winner) return 1;
    if((board[0] == board[index] && board[4] == board[index] && board[8] == board[index]) ||
    (board[2] == board[index] && board[4] == board[index] && board[6] == board[index])) return 1;

    return 0;
}

void exit_funct(){
    for(int i = 0; i < MAX_CONNECTIONS; ++i) {
        if(clients[i].socket_fd > 0) close(clients[i].socket_fd);
    }
    close(unix_socket);
    close(inet_socket);
    unlink(server_path);
    remove(server_path);
    exit(0);
}