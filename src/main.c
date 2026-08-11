#include "eznm-scenes.h"

internal int eznm_initialize(eznm_state_t* state) {

    enable_altbuf();
    toggle_nonblock();

    state->running = true;
    state->scene = ES_HOME;
    state->client = nm_client_new(NULL, &state->error);
    if (state->client == NULL) {
        g_printerr("failed to connect to network manager: %s\n", state->error->message);
        g_clear_error(&state->error);
        return -1;
    }

}

internal void eznm_loop(eznm_state_t* state) {

    local_persist bool force_render = false;
    local_persist eznm_scene_t last_scene = ES_NONE;

    char input = readc();    
    if (input == 'q' && state->scene == ES_HOME)
        state->running = false;

    switch (state->scene) {

        case ES_HOME            : force_render = eznm_input_home(state, input);                 break;
        case ES_SAVED           : force_render = eznm_input_saved_connections(state, input);    break;
        case ES_NEW_CONNECTION  : force_render = eznm_input_new_connections(state, input);      break;
        case ES_CONNECTIONS     : force_render = eznm_input_current_connections(state, input);  break;
        case ES_CREDENTIALS     : force_render = eznm_input_credentials(state, input);          break;

    }

    if (state->scene != last_scene || force_render) {

        force_render = false;
        last_scene = state->scene;

        switch (state->scene) {

            case ES_HOME            : eznm_render_home(state, input);                   break;
            case ES_SAVED           : eznm_render_saved_connections(state, input);      break;
            case ES_NEW_CONNECTION  : eznm_render_new_connections(state, input);        break;
            case ES_CONNECTIONS     : eznm_render_current_connections(state, input);    break;
            case ES_CREDENTIALS     : eznm_render_credentials(state, input);            break;

        }
    
    }

}

internal int eznm_deinitialize(eznm_state_t* state) {

    enable_altbuf();
    toggle_nonblock();

    g_object_unref(state->client);

    return EXIT_SUCCESS;

}

int main(void) {

    eznm_state_t state = empty_eznm_state_t;
    eznm_initialize(&state);

    while(state.running) {

        while (g_main_context_iteration(NULL, FALSE));
        eznm_loop(&state);
    
    }

    return eznm_deinitialize(&state);

}