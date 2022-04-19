#define MAX_CLIENT_NAME_LEN 50
#define MSG_MAX_LEN 500

typedef enum{NONE, O, X} sign_type;

typedef struct{
    char name[MAX_CLIENT_NAME_LEN];
    pthread_t thread_id;
    int socket_fd;
    sign_type sign;
    int game_num;
    int array_id;
} client_t;


typedef enum{REGISTER, DISCONNECT, PLAY, MOVEMENT, WAIT, RESULT, PING} response_type;

typedef struct{
    client_t player1;
    client_t player2;
    sign_type board[9];
}game_t;


typedef struct{
    char name[MAX_CLIENT_NAME_LEN];
    response_type type;
    int movement;
    int game_id;
    int field;
    sign_type sign;
    int client_arr_id;
    char msg[MSG_MAX_LEN];
    sign_type board[9];
}response_t;


//char* server_path = "/tmp/9Lq7BNBnBycd6nxy.socket";
//int port = 30020;