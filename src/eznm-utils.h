#if !defined(EZNM_UTILS_H)
#define EZNM_UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <libnm/NetworkManager.h>

#define internal        static
#define local_persist   static

#define MAX_APS_ON_LIST 5

typedef enum {

    ES_NONE             = -1,
    ES_HOME,
    ES_SAVED,
    ES_NEW_CONNECTION,
    ES_CONNECTIONS,
    ES_CREDENTIALS

} eznm_scene_t;

typedef struct eznm_state_t eznm_state_t;
struct eznm_state_t {
    
    bool running;
    GError* error;
    NMClient* client;
    eznm_scene_t scene;
    
    int page;
    int selected_idx;

    GPtrArray* aps;
    NMAccessPoint* selected;

};
#define empty_eznm_state_t                  (eznm_state_t){0}

internal void enable_altbuf(void) {
    printf("\033[?1049h\033[2J\033[H");
    fflush(stdout);
}

internal void disable_altbuf(void) {
    printf("\033[?1049l\033[2J\033[H");
    fflush(stdout);
}

internal void toggle_nonblock(void) {

    local_persist int persist_flags = -1;
    local_persist bool non_blocking = false;
    local_persist struct termios original_term, raw_term;

    non_blocking = !non_blocking;
    if (persist_flags == -1) {
        tcgetattr(STDIN_FILENO, &original_term);
        raw_term = original_term;
        raw_term.c_lflag &= ~(ICANON | ECHO); 
        persist_flags = fcntl(0, F_GETFL, 0);
    }
  
    int flags = non_blocking 
        ? persist_flags | O_NONBLOCK 
        : persist_flags;

    struct termios term = non_blocking
        ? raw_term
        : original_term;
    
    fcntl(0, F_SETFL, flags);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

}

internal char readc(void) {
    
    char result = -1;
    read(0, &result, 1);
    return result;

}

internal void eznm_on_connection_activate_cb(GObject* source_object, GAsyncResult* result, gpointer user_data) {
    
    eznm_state_t* state = (eznm_state_t*)user_data;
    NMActiveConnection* active_conn = NULL;

    active_conn = nm_client_add_and_activate_connection_finish(NM_CLIENT(source_object), result, &state->error);
    if (state->error != NULL) {
        printf("failed to connect: %s", &state->error->message);
        g_error_free(state->error);
        state->error = NULL;
    } else {
        if (active_conn != NULL)
            g_object_unref(active_conn);
    }

}

internal bool eznm_state_connect_selected(eznm_state_t* state, const char* identity, const char* password) {

    if (state == NULL || state->client == NULL || state->selected == NULL) {
        printf("state, client, or selected AP is NULL.");
        return false;
    }

    NMDevice* wifi_device = NULL;
    const GPtrArray* devices = nm_client_get_devices(state->client);
    if (devices != NULL) {
        for (size_t i = 0; i < devices->len; i++) {
            NMDevice* dev = g_ptr_array_index(devices, i);
            if (NM_IS_DEVICE_WIFI(dev)) {
                wifi_device = dev;
                break;
            }
        }
    }

    if (wifi_device == NULL) {
        printf("no Wi-Fi device found on the system.");
        return false;
    }

    GBytes* ssid_bytes = nm_access_point_get_ssid(state->selected);
    if (ssid_bytes == NULL) {
        printf("selected AP has no valid SSID.");
        return false;
    }

    NM80211ApSecurityFlags wpa_flags = nm_access_point_get_wpa_flags(state->selected);
    NM80211ApSecurityFlags rsn_flags = nm_access_point_get_rsn_flags(state->selected);
    bool is_enterprise = ((wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X) || (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X));

    NMConnection* connection = nm_simple_connection_new();
    NMSettingConnection* setting_connection = (NMSettingConnection*)nm_setting_connection_new();
    
    char *uuid = nm_utils_uuid_generate();
    g_object_set (
        setting_connection,
        NM_SETTING_CONNECTION_ID, "eznm-wifi-conn",
        NM_SETTING_CONNECTION_UUID, uuid,
        NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
        NM_SETTING_CONNECTION_AUTOCONNECT, TRUE,
        NULL
    );
    g_free(uuid);    
    nm_connection_add_setting(connection, NM_SETTING(setting_connection));

    NMSettingWireless* setting_wifi = (NMSettingWireless*)nm_setting_wireless_new();
    g_object_set(setting_wifi, NM_SETTING_WIRELESS_SSID, ssid_bytes, NULL);
    nm_connection_add_setting(connection, NM_SETTING(setting_wifi));

    if (is_enterprise) {

        NMSettingWirelessSecurity* setting_security = (NMSettingWirelessSecurity*)nm_setting_wireless_security_new();
        g_object_set(setting_security, NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-eap", NULL);
        nm_connection_add_setting(connection, NM_SETTING(setting_security));

        NMSetting8021x* setting_8021x = (NMSetting8021x*)nm_setting_802_1x_new();
        nm_setting_802_1x_add_eap_method(setting_8021x, "peap");
        g_object_set (
            setting_8021x,    
            NM_SETTING_802_1X_IDENTITY, identity,                     
            NM_SETTING_802_1X_PASSWORD, password,
            NM_SETTING_802_1X_PHASE2_AUTH, "mschapv2",
            NULL
        );
        nm_connection_add_setting(connection, NM_SETTING(setting_8021x));

    } else if (password && *password != '\0') {

        NMSettingWirelessSecurity* setting_security = (NMSettingWirelessSecurity*)nm_setting_wireless_security_new();
        g_object_set(setting_security, NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk", NM_SETTING_WIRELESS_SECURITY_PSK, password, NULL);
        nm_connection_add_setting(connection, NM_SETTING(setting_security));

    }

    nm_client_add_and_activate_connection_async (
        state->client,
        connection,
        wifi_device,
        nm_object_get_path(NM_OBJECT(state->selected)),
        NULL,
        eznm_on_connection_activate_cb,
        state
    );

    g_object_unref(connection);
    return true;

}

internal void eznm_on_deactivate_cb(GObject *source_object, GAsyncResult *res, gpointer user_data) {

    eznm_state_t* state = (eznm_state_t*)user_data;
    if (!nm_client_deactivate_connection_finish(NM_CLIENT(source_object), res, &state->error)) {
        printf("failed to disconnect: %s", state->error->message);
        g_error_free(state->error);
    } else printf("successfully disconnected from active connection.\n");

}

internal bool eznm_state_disconnect_selected(eznm_state_t* state) {

    if (state == NULL || state->client == NULL || state->selected == NULL) {
        g_warning("state, client, or selected AP is NULL.");
        return false;
    }

    const GPtrArray* active_connections = nm_client_get_active_connections(state->client);
    if (active_connections == NULL) 
        return false;

    NMActiveConnection* target_active = NULL;
    GBytes* selected_ssid = nm_access_point_get_ssid(state->selected);

    for (guint i = 0; i < active_connections->len; i++) {

        NMActiveConnection* active = g_ptr_array_index(active_connections, i);
        if (g_strcmp0(nm_active_connection_get_connection_type(active), NM_SETTING_WIRELESS_SETTING_NAME) != 0)
            continue;

        const GPtrArray* devices = nm_active_connection_get_devices(active);
        if (devices == NULL || devices->len == 0) 
            continue;

        NMDevice* device = g_ptr_array_index(devices, 0);
        if (NM_IS_DEVICE_WIFI(device)) {
            NMAccessPoint* active_ap = nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(device));
            if (active_ap != NULL) {
                GBytes* active_ssid = nm_access_point_get_ssid(active_ap);
                if (active_ssid && selected_ssid && g_bytes_equal(active_ssid, selected_ssid)) {
                    target_active = active;
                    break;
                }
            }
        }

    }

    if (target_active == NULL) {
        printf("selected AP is not currently connected.\n");
        return false;
    }

    nm_client_deactivate_connection_async(
        state->client,
        target_active,
        NULL,
        eznm_on_deactivate_cb,
        state
    );

    return true;
    
}

#endif // EZNM_UTILS_H