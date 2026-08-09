#include "../eznm-scenes.h"

void eznm_render_saved_connections(eznm_state_t* state, char input) {

    local_persist int page = 0;
    const GPtrArray* connections = nm_client_get_connections(state->client);

    if (input == 'd' && page < (int)(connections->len / MAX_APS_ON_LIST) - 1)
        page++;
    if (input == 'a' && page > 0)
        page--;
    int count = connections->len - (page * MAX_APS_ON_LIST);

    printf("\033[2J\033[H" "saved connections: press `q` to exit. page: %u, count: %u\n", page, count);

    for (int i = 0; i < (count > MAX_APS_ON_LIST ? MAX_APS_ON_LIST : count); i++) {

        NMConnection* connection = connections->pdata[i + (page * MAX_APS_ON_LIST)];
        NMSettingConnection* setting = nm_connection_get_setting_connection(connection);

        printf("\t%d: %s | %s | %s | %s\n", 
            i + (page * MAX_APS_ON_LIST), 
            nm_setting_connection_get_id(setting), 
            nm_setting_connection_get_uuid(setting),
            nm_setting_connection_get_connection_type(setting),
            nm_connection_get_path(connection)
        );

    } 

    printf("\nlast page: 'a', next page: 'd'\n");

}

bool eznm_input_saved_connections(eznm_state_t* state, char input) {

    if (input == 'q') {
        state->scene = ES_HOME;
        return false;
    }
    
    if (input == 'd' || input == 'a')
        return true;

    return false;

}