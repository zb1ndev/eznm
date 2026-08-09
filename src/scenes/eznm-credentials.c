#include "../eznm-scenes.h"

typedef enum eznm_credstate_t {

    CS_NONE,
    CS_UNAME,
    CS_PASS

} eznm_credstate_t;

typedef struct eznm_creds_t eznm_creds_t;
struct eznm_creds_t {
    
    char username_buf[256];
    char password_buf[256];
    size_t uname_len, pass_len;
    eznm_credstate_t state;

}; eznm_creds_t credentials = (eznm_creds_t){0};

void eznm_render_credentials(eznm_state_t* state, char input) {

    if (state->selected == NULL) {
        state->scene = ES_HOME;
        return;
    }

    GBytes* ssid_bytes = nm_access_point_get_ssid(state->selected);
    const char* ssid = ssid_bytes
        ? nm_utils_ssid_to_utf8(g_bytes_get_data(ssid_bytes, NULL), g_bytes_get_size(ssid_bytes))
        : "(hidden)";

    NM80211ApSecurityFlags wpa = nm_access_point_get_wpa_flags(state->selected);
    NM80211ApSecurityFlags rsn = nm_access_point_get_rsn_flags(state->selected);
    bool is_enterprise = ((wpa & NM_802_11_AP_SEC_KEY_MGMT_802_1X) || (rsn & NM_802_11_AP_SEC_KEY_MGMT_802_1X));

    printf("\033[2J\033[H");
    printf("credentials form, press `e` to edit, `enter` to submit, `q` to cancel | SSID: %s ===\n", ssid);
    printf("security: %s\n\n", is_enterprise ? "wpa-enterprise (802.1X)" : "wpa-personal / open");

    char masked_pass[256];
    memset(masked_pass, '*', credentials.pass_len);
    masked_pass[credentials.pass_len] = '\0';

    if (is_enterprise)
        printf("  %s username: %s%s\n", (credentials.state == CS_UNAME) ? ">" : " ", credentials.username_buf, (credentials.state == CS_UNAME) ? "_" : "");
    printf("  %s password: %s%s\n\n", (credentials.state == CS_PASS) ? ">" : " ", masked_pass, (credentials.state == CS_PASS) ? "_" : "");
    fflush(stdout);

}

bool eznm_input_credentials(eznm_state_t* state, char input) {
    
    if (state->selected == NULL) 
        return false;

    NM80211ApSecurityFlags wpa = nm_access_point_get_wpa_flags(state->selected);
    NM80211ApSecurityFlags rsn = nm_access_point_get_rsn_flags(state->selected);
    bool is_enterprise = ((wpa & NM_802_11_AP_SEC_KEY_MGMT_802_1X) || (rsn & NM_802_11_AP_SEC_KEY_MGMT_802_1X));

    if (credentials.state == CS_NONE) {

        if (input == 'q') {
            memset(&credentials, 0, sizeof(eznm_creds_t));
            state->scene = ES_HOME;
            if (state->aps) {
                g_ptr_array_unref(state->aps);
                state->aps = NULL;
            }
            state->selected = NULL;
            return true;
        }

        if (input == 'e') {
            credentials.state = is_enterprise ? CS_UNAME : CS_PASS;
            return true;
        }

        if (input == '\n') {
            state->scene = ES_HOME;
            return true;
        }

        return false;

    }
    
    char* active_buf    = (credentials.state == CS_UNAME) ? credentials.username_buf : credentials.password_buf;
    size_t* active_len  = (credentials.state == CS_UNAME) ? &credentials.uname_len : &credentials.pass_len;

    if (input == '\n') {

        credentials.state = (credentials.state == CS_UNAME) 
            ? CS_PASS 
            : CS_NONE;

        if (credentials.state == CS_NONE) {
            eznm_state_connect_selected(state, is_enterprise ? credentials.username_buf : NULL, credentials.password_buf);
            memset(&credentials, 0, sizeof(eznm_creds_t));
        }

        return true;
        
    }

    if (input == 127 || input == 8 || input == '\b') {
        if (*active_len > 0) {
            (*active_len)--;
            active_buf[*active_len] = '\0';
            return true;
        }
        return false;
    }

    if (isprint((unsigned char)input) && *active_len < 255) {
        active_buf[*active_len] = input;
        (*active_len)++;
        active_buf[*active_len] = '\0';
        return true;
    }

    return false;

}