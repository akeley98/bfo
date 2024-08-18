#include QMK_KEYBOARD_H

#define BASE_LAYER 0
#define GAME_LAYER  1
#define FKEY_FN_LAYER 2
#define SYMB_FN_LAYER 3
#define META_FN_LAYER 4
#define CTRL_FN_LAYER 5

enum custom_keycodes {
    FKEY_FN = SAFE_RANGE,
    SYMB_FN,
    META_FN,
    CTRL_FN,

    GAME_MODE,
    SYN_SFT,
    SYN_CTL,
    SYN_ALT,
    SYN_GUI,
    MODLK,
    META_FN_LK,
    CTRL_FN_LK,
    SYMB_FN_LK,
    FKEY_FN_LK,
    PARENS,
    U_LOCK,
};

#define PCTL LCTL
#define PSFT LSFT
#define PALT LALT
#define PGUI LGUI

#define VCTL RCTL
#define VSFT RSFT
#define VALT RALT
#define VGUI RGUI

#define KC_VSFT KC_LSFT /* Need to avoid the weird qmk magic command */
#define KC_VCTL KC_LCTL /* My code is buggy */
#define KC_VALT KC_LALT /* Avoid AltGr, Alt-menu, etc. */
#define KC_VGUI KC_LGUI /* Avoid triggering Windows start menu (prefer Ctrl+Escape) */

#define KC_PSFT KC_LSFT
#define KC_PCTL KC_LCTL
#define KC_PALT KC_LALT
#define KC_PGUI KC_LGUI

#define MOD_UP 0
#define MOD_UP_STICKY_WAITING 1
#define MOD_UP_STICKY_ACTIVE 2
#define MOD_DOWN 3
#define MOD_DOWN_STICKY 4
#define MOD_LOCKED 5

#define MOD_STICKY_TIMEOUT 300

struct mod_state
{
    const uint16_t kc_physical;
    const uint16_t kc_virtual;
    uint16_t timer_when_down;
    uint8_t state;
};

static struct mod_state sft_mod_state = { KC_PSFT, KC_VSFT };
static struct mod_state ctl_mod_state = { KC_PCTL, KC_VCTL };
static struct mod_state alt_mod_state = { KC_PALT, KC_VALT };
static struct mod_state gui_mod_state = { KC_PGUI, KC_VGUI };

static void mod_state_on_syn_mod_down(struct mod_state* state)
{
    uint8_t old_state = state->state;
    state->timer_when_down = timer_read();
    if (old_state == MOD_LOCKED) return;
    state->state = MOD_DOWN_STICKY;
    register_code(state->kc_physical);
}

static void mod_state_on_syn_mod_up(struct mod_state* state)
{
    uint8_t old_state = state->state;
    switch (old_state) {
      default:
        unregister_code(state->kc_physical);
      break; case MOD_DOWN: non_sticky:
        state->state = MOD_UP;
        unregister_code(state->kc_physical);
      break; case MOD_DOWN_STICKY:
        if (timer_read() - state->timer_when_down > MOD_STICKY_TIMEOUT) {
            goto non_sticky;
        }
        state->state = MOD_UP_STICKY_WAITING;
        if (state->kc_physical != state->kc_virtual) {
            unregister_code(state->kc_physical);
            register_code(state->kc_virtual);
        }
      break; case MOD_LOCKED:
        if (state->kc_physical != state->kc_virtual) {
            unregister_code(state->kc_physical);
        }
    }
}

 static void mod_state_on_normal_key_down(struct mod_state* state)
{
    uint8_t old_state = state->state;
    switch (old_state) {
      default:
      break; case MOD_UP_STICKY_WAITING:
        state->state = MOD_UP_STICKY_ACTIVE;
      break; case MOD_UP_STICKY_ACTIVE:
        state->state = MOD_UP;
        unregister_code(state->kc_virtual);
      break; case MOD_DOWN_STICKY:
        state->state = MOD_DOWN;
    }
}

static void mod_state_on_normal_key_up(struct mod_state* state)
{
    uint8_t old_state = state->state;
    switch (old_state) {
      default:
      break; case MOD_UP_STICKY_ACTIVE:
        state->state = MOD_UP;
        unregister_code(state->kc_virtual);
    }
}

static bool mod_state_on_modlk(struct mod_state* state)
{
    uint8_t old_state = state->state;
    switch (old_state) {
      default:
        return false;
      break; case MOD_DOWN: case MOD_DOWN_STICKY: case MOD_UP_STICKY_WAITING:
        state->state = MOD_LOCKED;
        register_code(state->kc_virtual);
        return true;
      break; case MOD_UP_STICKY_ACTIVE:
        state->state = MOD_UP;
        unregister_code(state->kc_virtual);
        return false;
    }
}

static void mod_state_on_unlock(struct mod_state* state)
{
    if (state->state == MOD_LOCKED) {
        state->state = MOD_UP; // XXX
        unregister_code(state->kc_virtual);
    }
}

struct fn_state
{
    uint8_t fn_layer;
    uint8_t state;
    uint16_t timer_when_down;
};

static struct fn_state meta_fn_state = { META_FN_LAYER };
static struct fn_state ctrl_fn_state = { CTRL_FN_LAYER };
static struct fn_state symb_fn_state = { SYMB_FN_LAYER };
static struct fn_state fkey_fn_state = { FKEY_FN_LAYER };

#define FN_UP 0
#define FN_UP_STICKY 1
#define FN_DOWN 2
#define FN_DOWN_STICKY 3
#define FN_LOCKED 4

static void fn_state_on_fn_down(struct fn_state* state)
{
    state->state = FN_DOWN_STICKY;
    state->timer_when_down = timer_read();
    layer_on(state->fn_layer);
}

static void fn_state_on_fn_up(struct fn_state* state)
{
    uint8_t old_state = state->state;
    switch (old_state) {
      default:
      break; case FN_DOWN: non_sticky:
        state->state = FN_UP;
        layer_off(state->fn_layer);
      break; case FN_DOWN_STICKY:
        if (timer_read() - state->timer_when_down > MOD_STICKY_TIMEOUT) {
            goto non_sticky;
        }
        state->state = FN_UP_STICKY;
    }
}

static void fn_state_on_normal_key_down(struct fn_state* state)
{
    uint8_t old_state = state->state;
    switch (old_state) {
      default:
      break; case FN_UP_STICKY:
        state->state = FN_UP;
        layer_off(state->fn_layer);
      break; case FN_DOWN_STICKY:
        state->state = FN_DOWN;
    }
}

#define LEGACY_REMNANT_LAYOUT( \
    L01, L02, L03, L04, L05, R01, R02, R03, R04, R05, \
    L11, L12, L13, L14, L15, R11, R12, R13, R14, R15, \
    L21, L22, L23, L24, L25, R21, R22, R23, R24, R25, \
         L32, L33, L34, L35, R31, R32, R33, R34, \
    L41, L42, L43, L44,           R42, R43, R44, R45 ) \
LAYOUT( \
KC_NO, KC_NO, KC_NO, KC_NO,       L01, L02, L03, L04, L05, R01, R02, R03, R04, R05, KC_NO, KC_NO, KC_NO, KC_NO, \
KC_NO, KC_NO, KC_NO, KC_NO,       L11, L12, L13, L14, L15, R11, R12, R13, R14, R15, KC_NO, KC_NO, KC_NO, KC_NO, \
KC_NO, KC_NO, KC_NO, KC_NO, GAME_MODE, L22, L23, L24, L25, R21, R22, R23, R24, R25, KC_NO, KC_NO, KC_NO, KC_NO, \
KC_NO, KC_NO, KC_NO, KC_NO,       L21, L32, L33, L34, L35, R31, R32, R33, R34, R25, KC_NO, KC_NO, KC_NO, KC_NO, \
KC_NO, KC_NO, KC_NO, KC_NO,       L41, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, R45, KC_NO, KC_NO, KC_NO, KC_NO, \
KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, L42, L43, L44, KC_NO, KC_NO, R42, R43, R44, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[BASE_LAYER] = LEGACY_REMNANT_LAYOUT( \
MODLK,  KC_ESC,  KC_TAB, KC_Z,   KC_NO,  KC_NO,  KC_Q,   KC_W,   KC_V,   RSFT(KC_MINS), \
KC_QUOTE,KC_O,   KC_U,   KC_P,   KC_Y,   KC_F,   KC_G,   KC_C,   KC_R,   KC_L, \
KC_I,    KC_E,   KC_SPC, KC_A,   KC_J,   KC_D,   KC_H,   KC_T,   KC_N,   KC_S, \
       SYN_CTL, SYN_SFT, KC_K,   KC_X,   KC_B,   KC_M, SYN_SFT, SYN_CTL,         \
SYN_ALT, SYN_GUI, META_FN, CTRL_FN,           SYMB_FN, FKEY_FN, KC_APP, SYN_ALT ),

[GAME_LAYER] = LEGACY_REMNANT_LAYOUT( \
KC_1   , KC_2,    KC_3,    KC_4,    KC_5,    _______, _______, _______, _______, _______,
KC_ESC , _______, _______, _______, _______, _______, _______, _______, _______, _______,
_______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
         KC_LCTL, KC_LSFT, _______, _______, _______, _______, _______, _______,
CTRL_FN, KC_LGUI, KC_LALT, KC_Z,                      _______, _______, U_LOCK , _______),

[SYMB_FN_LAYER] = LEGACY_REMNANT_LAYOUT( \
SYMB_FN_LK, KC_7,   KC_8,   KC_9, KC_NO,        KC_NO,  KC_NO,   KC_NO,  RSFT(KC_9), RSFT(KC_0),
KC_EQL,     KC_4,   KC_5,   KC_6, KC_RGUI,      KC_NO,  KC_NO,   KC_LBRC,KC_RBRC,KC_BACKSLASH, \
KC_MINS,    KC_1,   KC_2,   KC_3, KC_GRV,       KC_NO,  KC_0,    KC_COMMA,KC_DOT,KC_SCLN, \
 _______, _______,        PARENS, KC_NO,        KC_NO,  KC_SLASH, _______, _______, \
_______, _______, _______, _______, _______, _______, _______, _______),

[FKEY_FN_LAYER] = LEGACY_REMNANT_LAYOUT( \
FKEY_FN_LK, KC_F7,  KC_F8,  KC_F9,  KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
KC_VOLU,    KC_F4,  KC_F5,  KC_F6,  KC_NO, KC_NO, KC_NO, KC_PRINT_SCREEN, KC_SCROLL_LOCK, KC_PAUSE,
KC_VOLD,    KC_F1,  KC_F2,  KC_F3,  KC_NO, KC_NO, KC_F9, KC_F10, KC_F11, KC_F12,
 _______, _______,          KC_MUTE,KC_NO, KC_NO, KC_MUTE, _______, _______, \
_______, _______, _______, _______, _______, _______, _______, _______),

[META_FN_LAYER] = LEGACY_REMNANT_LAYOUT( \
META_FN_LK, KC_NO,  KC_NO, KC_NO, LALT(KC_Z), KC_NO, LALT(KC_Q), LALT(KC_W), KC_PGUP, KC_NO, \
LALT(KC_Y), LALT(KC_O), LALT(KC_U), LALT(KC_P), LALT(KC_Y), RCTL(KC_RIGHT), RCTL(KC_UP), LALT(KC_C), LALT(KC_R), LALT(KC_L), \
LALT(KC_I), RCTL(KC_END), LALT(KC_SPC), RCTL(KC_HOME), LALT(KC_J), RCTL(KC_DELETE), RCTL(KC_BACKSPACE), RCTL(KC_PGUP), RCTL(KC_DOWN), LALT(KC_S), \
      _______, _______, LALT(KC_K),   LALT(KC_X),   RCTL(KC_LEFT), RCTL(KC_ENTER), _______, _______, \
_______, _______, _______, _______, _______, _______, _______, _______),

[CTRL_FN_LAYER] = LEGACY_REMNANT_LAYOUT( \
CTRL_FN_LK, RCTL(KC_BSLS),  KC_NO, RCTL(KC_Z), KC_NO, KC_NO,  RCTL(KC_Q),   RCTL(KC_W),   KC_PGDN, RCTL(RSFT(KC_MINS)), \
RCTL(KC_Y), RCTL(KC_O), RCTL(KC_U),  RCTL(KC_G), RCTL(KC_Y), KC_RIGHT, KC_UP, RCTL(KC_C), RCTL(KC_R), RCTL(KC_L), \
KC_TAB,KC_END, RCTL(KC_SPC), KC_HOME,   RCTL(KC_J), KC_DELETE,  KC_BACKSPACE, RCTL(KC_PGDN), KC_DOWN, RCTL(KC_S), \
      _______, _______,   RCTL(KC_K),   RCTL(KC_X), KC_LEFT,    KC_ENTER, _______, _______, \
_______, _______, _______, _______, _______, _______, _______, _______),

};

static bool in_game_mode = false;
static bool is_normal_key;
static bool should_process;
static bool modlk_state_changed;
static bool previous_was_modlk;
static bool modlk_active = false;

void handle_modlk(void)
{
    modlk_state_changed |= mod_state_on_modlk(&sft_mod_state);
    modlk_state_changed |= mod_state_on_modlk(&ctl_mod_state);
    modlk_state_changed |= mod_state_on_modlk(&alt_mod_state);
    modlk_state_changed |= mod_state_on_modlk(&gui_mod_state);

    if (!modlk_active && !previous_was_modlk) {
        modlk_active = true;
        if (!modlk_state_changed) {
            mod_state_on_syn_mod_down(&sft_mod_state);
            mod_state_on_syn_mod_up(&sft_mod_state);
            mod_state_on_modlk(&sft_mod_state);
        }
    }
    else {
        modlk_active = false;
        if (meta_fn_state.state == FN_LOCKED) {
            meta_fn_state.state = FN_UP;
            layer_off(META_FN_LAYER);
        }
        if (ctrl_fn_state.state == FN_LOCKED) {
            ctrl_fn_state.state = FN_UP;
            layer_off(CTRL_FN_LAYER);
        }
        if (symb_fn_state.state == FN_LOCKED) {
            symb_fn_state.state = FN_UP;
            layer_off(SYMB_FN_LAYER);
        }
        if (fkey_fn_state.state == FN_LOCKED) {
            fkey_fn_state.state = FN_UP;
            layer_off(FKEY_FN_LAYER);
        }
        mod_state_on_unlock(&sft_mod_state);
        mod_state_on_unlock(&ctl_mod_state);
        mod_state_on_unlock(&alt_mod_state);
        mod_state_on_unlock(&gui_mod_state);
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
    should_process = false;
    is_normal_key = true;
    modlk_state_changed = false;
    bool this_is_modlk = false;

    switch (keycode) {
      default:
        should_process = true;
      break; case GAME_MODE:
        if (record->event.pressed) {
            clear_keyboard();
            in_game_mode = !in_game_mode;
            in_game_mode ? layer_on(GAME_LAYER) : layer_off(GAME_LAYER);
            is_normal_key = false;
        }
      break; case META_FN:
        is_normal_key = false;
        if (record->event.pressed) {
            fn_state_on_fn_down(&meta_fn_state);
        } else {
            fn_state_on_fn_up(&meta_fn_state);
        }
      break; case CTRL_FN:
        is_normal_key = false;
        if (record->event.pressed) {
            fn_state_on_fn_down(&ctrl_fn_state);
        } else {
            fn_state_on_fn_up(&ctrl_fn_state);
        }
      break; case SYMB_FN:
        is_normal_key = false;
        if (record->event.pressed) {
            fn_state_on_fn_down(&symb_fn_state);
        } else {
            fn_state_on_fn_up(&symb_fn_state);
        }
      break; case FKEY_FN:
        is_normal_key = false;
        if (record->event.pressed) {
            fn_state_on_fn_down(&fkey_fn_state);
        } else {
            fn_state_on_fn_up(&fkey_fn_state);
        }
      break; case SYN_SFT:
        is_normal_key = false;
        if (record->event.pressed) {
            mod_state_on_syn_mod_down(&sft_mod_state);
        } else {
            mod_state_on_syn_mod_up(&sft_mod_state);
        }
      break; case SYN_CTL:
        is_normal_key = false;
        if (record->event.pressed) {
            mod_state_on_syn_mod_down(&ctl_mod_state);
        } else {
            mod_state_on_syn_mod_up(&ctl_mod_state);
        }
      break; case SYN_ALT:
        is_normal_key = false;
        if (record->event.pressed) {
            mod_state_on_syn_mod_down(&alt_mod_state);
        } else {
            mod_state_on_syn_mod_up(&alt_mod_state);
        }
      break; case SYN_GUI:
        is_normal_key = false;
        if (record->event.pressed) {
            mod_state_on_syn_mod_down(&gui_mod_state);
        } else {
            mod_state_on_syn_mod_up(&gui_mod_state);
        }
      break; case META_FN_LK:
        if (record->event.pressed) {
            if (meta_fn_state.state != FN_LOCKED) modlk_state_changed = true;
            meta_fn_state.state = FN_LOCKED;
            layer_on(META_FN_LAYER);
            handle_modlk();
            this_is_modlk = true;
        }
      break; case CTRL_FN_LK:
        if (record->event.pressed) {
            if (ctrl_fn_state.state != FN_LOCKED) modlk_state_changed = true;
            ctrl_fn_state.state = FN_LOCKED;
            layer_on(CTRL_FN_LAYER);
            handle_modlk();
            this_is_modlk = true;
        }
      break; case SYMB_FN_LK:
        if (record->event.pressed) {
            if (symb_fn_state.state != FN_LOCKED) modlk_state_changed = true;
            symb_fn_state.state = FN_LOCKED;
            layer_on(SYMB_FN_LAYER);
            handle_modlk();
            this_is_modlk = true;
        }
      break; case FKEY_FN_LK:
        if (record->event.pressed) {
            if (fkey_fn_state.state != FN_LOCKED) modlk_state_changed = true;
            fkey_fn_state.state = FN_LOCKED;
            layer_on(FKEY_FN_LAYER);
            handle_modlk();
            this_is_modlk = true;
        }
      break; case MODLK:
        is_normal_key = true;
        if (record->event.pressed) {
            handle_modlk();
            this_is_modlk = true;
        }
      break; case PARENS:
        should_process = true;
        if (record->event.pressed) {
            // SEND_STRING("()");
            register_code(KC_RSFT);
            tap_code(KC_9);
            tap_code(KC_0);
            unregister_code(KC_RSFT);
            tap_code(KC_LEFT);
        }
      break; case U_LOCK:
        if (record->event.pressed) {
            register_code(KC_U);
        }
    }

    if (is_normal_key && record->event.pressed) {
        mod_state_on_normal_key_down(&sft_mod_state);
        mod_state_on_normal_key_down(&ctl_mod_state);
        mod_state_on_normal_key_down(&alt_mod_state);
        mod_state_on_normal_key_down(&gui_mod_state);
    }

    if (is_normal_key && !record->event.pressed) {
        mod_state_on_normal_key_up(&sft_mod_state);
        mod_state_on_normal_key_up(&ctl_mod_state);
        mod_state_on_normal_key_up(&alt_mod_state);
        mod_state_on_normal_key_up(&gui_mod_state);
    }

    if (record->event.pressed) {
        previous_was_modlk = this_is_modlk;
    }

    return should_process;
}

void post_process_record_user(uint16_t keycode, keyrecord_t *record)
{
    if (is_normal_key && record->event.pressed) {
        fn_state_on_normal_key_down(&meta_fn_state);
        fn_state_on_normal_key_down(&ctrl_fn_state);
        fn_state_on_normal_key_down(&symb_fn_state);
        fn_state_on_normal_key_down(&fkey_fn_state);
    }
}
