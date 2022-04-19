#include "client.h"

int main(int argc, char** argv){
    
    atexit(exit_function);
    get_input(argc, argv);
    signal(SIGINT, exit_function);
    init_client();
    register_client();
    play_the_game();
    return 0;
}
