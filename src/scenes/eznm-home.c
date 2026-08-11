#include "../eznm-scenes.h"

void eznm_render_home(eznm_state_t* state, char input) {
        
    printf("\033[2J\033[H"
        "welcome to eznm: press `q` to exit.\n"
        "\t1: saved connections\n"
        "\t2: new connection\n"
        "\t3: disconnect\n"
    );

}

bool eznm_input_home(eznm_state_t* state, char input) {

    switch (input) {
        case '1' : state->scene = ES_SAVED; break;
        case '2' : state->scene = ES_NEW_CONNECTION; break;
        case '3' : state->scene = ES_CONNECTIONS; break;
        default : break;
    }

    return false;

}