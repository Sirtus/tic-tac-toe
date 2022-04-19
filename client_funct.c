#include "client.h"


void get_input(int argc, char** argv){
        if(argc < 4){
        printf("Błędna liczba argumentów\n");
        exit(0);        
    }
    strcpy(client_name, argv[1]);
    if(strcmp(argv[2], "unix")==0){
        mode = UNIX;
        strcpy(server_path, argv[3]);
    }
    else if(strcmp(argv[2], "inet")==0){
        mode = INET;
        strcpy(ip, argv[3]);
        port = atoi(argv[4]);
    }
    else{
        printf("Niepoprawny argument\n");
        exit(0);
    }
}

void register_client(){
    response_t response;
    strcpy(response.name, client_name);
    response.type = REGISTER;
    strcpy(client.name, response.name);
    if(send(socket_fd, (void*) &response, sizeof(response), 0) == -1)
        perror("send");
    
}

void init_client(){
    struct sockaddr* server_addr;
    socklen_t addr_len;
    if(mode == UNIX) {
        if((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
            perror("unix socket");
        struct sockaddr_un serv_ux_addr;
        memset(&serv_ux_addr, 0, sizeof(serv_ux_addr));
        serv_ux_addr.sun_family = AF_UNIX;
        strcpy(serv_ux_addr.sun_path, server_path);
        server_addr = (struct sockaddr*) &serv_ux_addr;
        addr_len = sizeof(struct sockaddr_un);
    }
    else if( mode == INET){
        if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            perror("inet socket");
        struct sockaddr_in serv_in_addr;
        memset(&serv_in_addr, 0, sizeof(serv_in_addr));
        serv_in_addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip,&(serv_in_addr.sin_addr.s_addr));
        serv_in_addr.sin_port = htons(port);
        server_addr = (struct sockaddr*) &serv_in_addr;
        addr_len = sizeof(struct sockaddr_in);
    }
    else{
        fprintf(stderr, "\nsomething went wrong\n");
        exit_function();
    }
    if(connect(socket_fd, server_addr, addr_len) == -1) 
        perror("connect");
    printf("połączono z serwerem\n");
}

void play_the_game(){
    response_t response;
    printf("Wpisz 'play' aby grać, 'disconnect' aby zakończyć program\n");
    scanf("%s",response.msg);
    if(strcmp(response.msg, "play") == 0) response.type = PLAY;
    else if(strcmp(response.msg, "disconnect") == 0) exit_function();
    write(socket_fd, (void*) &response, sizeof(response_t));
    response_t read_res;
    response_t ping_res;
    ping_res.type = PING;

    while(1){
        read(socket_fd, (void*) &read_res, sizeof(response_t));

        switch(read_res.type){
            case WAIT:
                printf("%s\n",read_res.msg);
                break;
            case PLAY:
                client.sign = read_res.sign;
                game_id = read_res.game_id;
                printf("Otrzymano znak: ");
                if(read_res.sign == O) printf("O\n");
                else printf("X\n");
                break;
            case MOVEMENT:
                printf("%s\n",read_res.msg);
                printf("Twój ruch\n");
                int field;
                int is_valid = 0;
                int is_result = 0;
                pthread_t ping_tr;
                pthread_create(&ping_tr, NULL, ping, NULL);
                while(!is_valid && !is_result){
                    scanf("%d", &field);
                    is_valid = valide(field, read_res.board);
                    if(!is_valid) printf("\nTam nie możesz zrobić ruchu\n");
                }
                if(!is_result){
                    pthread_cancel(ping_tr);
                    response_t res;
                    res.type = MOVEMENT;
                    res.game_id = game_id;
                    res.client_arr_id = read_res.client_arr_id;
                    res.sign = client.sign;
                    res.field = field;
                    write(socket_fd, (void*) &res, sizeof(response_t));
                }
                
                break;

            case RESULT:
                printf("%s\n",read_res.msg);
                response_t response;
                game_id = -1;
                printf("Wpisz 'play' aby grać, 'disconnect' aby zakończyć program\n");
                scanf("%s",response.msg);
                if(strcmp(response.msg, "play") == 0) response.type = PLAY;
                else if(strcmp(response.msg, "disconnect") == 0) exit_function();
                write(socket_fd, (void*) &response, sizeof(response_t));
                break;

            case PING:
                write(socket_fd, (void*) &ping_res, sizeof(response_t));
                break;
            default:
                break;
        }
    }
}

void* ping(void* arg){

    response_t r, ping_res;
    ping_res.type = PING;
    while(1){
        read(socket_fd, (void*) &r, sizeof(response_t));

        switch(r.type){
            case PING:
                write(socket_fd, (void*) &ping_res, sizeof(response_t));
                break;
            case RESULT:
                is_result = 1;
                printf("%s\n",r.msg);
                response_t response;
                game_id = -1;
                printf("Wpisz 'play' aby grać, 'disconnect' aby zakończyć program\n");
                scanf("%s",response.msg);
                if(strcmp(response.msg, "play") == 0) response.type = PLAY;
                else if(strcmp(response.msg, "disconnect") == 0) response.type = DISCONNECT;
                write(socket_fd, (void*) &response, sizeof(response_t));
                return NULL;
            default:
                break;
        }
    }
    return NULL;
}


int valide(int field, sign_type board[9]){
    if(field < 0 || field > 8  || board[field] != NONE) return 0;
    return 1;
}

void exit_function(){
    response_t response;
    response.type = DISCONNECT;
    response.game_id = game_id;
    write(socket_fd, (void*) &response, sizeof(response_t));
    exit(0);
}