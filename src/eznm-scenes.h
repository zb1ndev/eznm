#if !defined(EZNM_SCENES_H)
#define EZNM_SCENES_H

#include "eznm-utils.h"

void        eznm_render_home                    (eznm_state_t* state, char input);
bool        eznm_input_home                     (eznm_state_t* state, char input);

void        eznm_render_saved_connections       (eznm_state_t* state, char input);
bool        eznm_input_saved_connections        (eznm_state_t* state, char input);

void        eznm_render_new_connections         (eznm_state_t* state, char input);
bool        eznm_input_new_connections          (eznm_state_t* state, char input);

void        eznm_render_current_connections     (eznm_state_t* state, char input);
bool        eznm_input_current_connections      (eznm_state_t* state, char input);

void        eznm_render_credentials             (eznm_state_t* state, char input);
bool        eznm_input_credentials              (eznm_state_t* state, char input);

#endif // EZNM_SCENES_H