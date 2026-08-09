#include "../eznm-scenes.h"

void eznm_render_current_connections(eznm_state_t* state, char input) {

    const GPtrArray* connections = nm_client_get_active_connections(state->client);
    guint total_len = (connections != NULL) 
        ? connections->len
        : 0;

    int max_pages = (total_len > 0) 
        ? (int)((total_len - 1) / MAX_APS_ON_LIST) + 1 
        : 1;

    if (state->page >= max_pages) 
        state->page = max_pages - 1;

    if (state->page < 0) 
        state->page = 0;

    if (state->selected_idx >= (int)total_len) 
        state->selected_idx = (int)total_len - 1;

    if (state->selected_idx < 0) 
        state->selected_idx = 0;

    int count = total_len - (state->page * MAX_APS_ON_LIST);

    printf("\033[2J\033[Hcurrent connections: press `q` to exit. page: %d/%d, count: %u\n\n", state->page + 1, max_pages, total_len);

    if (total_len == 0) {
        printf("\t(no active connections found)\n");
    } else {
    
        int items_on_page = (count > MAX_APS_ON_LIST) 
            ? MAX_APS_ON_LIST 
            : count;

        for (int i = 0; i < items_on_page; i++) {

            int global_idx = i + (state->page * MAX_APS_ON_LIST);
            NMActiveConnection* connection = g_ptr_array_index(connections, global_idx);
            NMActiveConnectionState state_val = nm_active_connection_get_state(connection);

            const char* id   = nm_active_connection_get_id(connection);
            const char* type = nm_active_connection_get_connection_type(connection);

            const char* state_str =
                (state_val == NM_ACTIVE_CONNECTION_STATE_ACTIVATING)   ? "activating"   :
                (state_val == NM_ACTIVE_CONNECTION_STATE_ACTIVATED)    ? "activated"    :
                (state_val == NM_ACTIVE_CONNECTION_STATE_DEACTIVATING) ? "deactivating" :
                (state_val == NM_ACTIVE_CONNECTION_STATE_DEACTIVATED)  ? "deactivated"  : "unknown";

            bool is_selected = (global_idx == state->selected_idx);
            printf("  %s %2d: %-24s | %-16s | %-12s\n", is_selected ? ">" : " ", global_idx, id ? id : "(unknown)", type ? type : "(unknown)", state_str);

        }
    }

    printf("\n`w` / `s` select | `r` disconnect / remove | last page: 'a', next page: 'd', (%d/%d) | `q` back\n", state->page + 1, max_pages);

}

bool eznm_input_current_connections(eznm_state_t* state, char input) {

    const guint MAX_CONNECTIONS_ON_LIST = 5;
    const GPtrArray* connections = nm_client_get_active_connections(state->client);
    guint total_len = connections ? connections->len : 0;

    if (input == 'q') {
        state->scene = ES_HOME;
        return false;
    }

    // Page navigation
    if (input == 'd') {
        int max_pages = total_len > 0 ? (int)((total_len - 1) / MAX_CONNECTIONS_ON_LIST) + 1 : 1;
        if (state->page < max_pages - 1) {
            state->page++;
            state->selected_idx = state->page * MAX_CONNECTIONS_ON_LIST;
            return true;
        }
    }
    if (input == 'a') {
        if (state->page > 0) {
            state->page--;
            state->selected_idx = state->page * MAX_CONNECTIONS_ON_LIST;
            return true;
        }
    }

    // Up/Down Selection
    if (input == 'w') { // Up
        if (state->selected_idx > 0) {
            state->selected_idx--;
            state->page = state->selected_idx / MAX_CONNECTIONS_ON_LIST;
            return true;
        }
    }
    if (input == 's') { // Down
        if (state->selected_idx < (int)total_len - 1) {
            state->selected_idx++;
            state->page = state->selected_idx / MAX_CONNECTIONS_ON_LIST;
            return true;
        }
    }

    // Remove / Deactivate selected connection
    if (input == 'r') {
        if (total_len > 0 && state->selected_idx >= 0 && state->selected_idx < (int)total_len) {
            NMActiveConnection* target = g_ptr_array_index(connections, state->selected_idx);
            
            // Deactivate active connection from NetworkManager
            nm_client_deactivate_connection_async(
                state->client,
                target,
                NULL,
                eznm_on_deactivate_cb,
                state
            );

            // Adjust selection index if last element was deleted
            if (state->selected_idx > 0 && state->selected_idx == (int)total_len - 1) {
                state->selected_idx--;
            }

            return true;
            
        }
    }

    return false;

}