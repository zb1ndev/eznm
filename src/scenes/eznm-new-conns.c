#include "../eznm-scenes.h"

void eznm_render_new_connections(eznm_state_t* state, char input) {

    if (state->aps == NULL) {

        state->aps = g_ptr_array_new();
        const GPtrArray* devices = nm_client_get_devices(state->client);

        for (int d = 0; d < devices->len; d++) {

            NMDevice* dev = devices->pdata[d];
            if (nm_device_get_device_type(dev) != NM_DEVICE_TYPE_WIFI) 
                continue;

            NMDeviceWifi* wifi = NM_DEVICE_WIFI(dev);
            const GPtrArray* scan = nm_device_wifi_get_access_points(wifi);
            for (int i = 0; i < scan->len; i++)
                g_ptr_array_add(state->aps, scan->pdata[i]);

        }

    }

    if (input == 'd' && state->page < (int)(state->aps->len / MAX_APS_ON_LIST) - 1)
        state->page++;
    if (input == 'a' && state->page > 0)
        state->page--;
    int count = state->aps->len - (state->page * MAX_APS_ON_LIST);

    printf(
        "\033[2J\033[H"
        "new connections: press `q` to exit, '0' - '9' to select connection. page: %u, count: %u\n",
        state->page, count
    );

    for (int i = 0; i < (count > MAX_APS_ON_LIST ? MAX_APS_ON_LIST : count); i++) {

        NMAccessPoint* ap = state->aps->pdata[i + (state->page * MAX_APS_ON_LIST)];
        GBytes* ssid_bytes = nm_access_point_get_ssid(ap);
        const char* ssid = ssid_bytes
            ? nm_utils_ssid_to_utf8(g_bytes_get_data(ssid_bytes, NULL), g_bytes_get_size(ssid_bytes))
            : "(hidden)";

        NM80211Mode mode        = nm_access_point_get_mode(ap);
        unsigned char strength  = nm_access_point_get_strength(ap);
        unsigned int  freq      = nm_access_point_get_frequency(ap);
        const char* bssid       = nm_access_point_get_bssid(ap);
        
        const char* mode_str = 
            mode == NM_802_11_MODE_INFRA ? "infra" :
            mode == NM_802_11_MODE_MESH  ? "mesh"  :
            mode == NM_802_11_MODE_ADHOC ? "adhoc" : "?";

        printf("\t%d: %-32s | %-5s | %3u%% | %4u MHz | %s\n", i + (state->page * MAX_APS_ON_LIST), ssid, mode_str, strength, freq, bssid);
    }

    printf("\nlast page: 'a', next page: 'd'\n");

}

bool eznm_input_new_connections(eznm_state_t* state, char input) {

    if (input == 'q') {
        state->scene = ES_HOME;
        g_ptr_array_unref(state->aps);
        state->aps = NULL;
        return false;
    }

    if (input == 'd' || input == 'a')
        return true;

    if (input >= '0' && input <= '9') {
        state->selected = state->aps->pdata[(input - '0') + (state->page * MAX_APS_ON_LIST)];
        state->scene = ES_CREDENTIALS;
        return false;
    } 

    return false;
}