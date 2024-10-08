#define CFG_TUSB_OS OPT_OS_PICO
#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb_hid_keys.h"
#include "macro.h"

#define REPORT_ID_KEYBOARD 1
#define MAX_COINCIDENT_KEYS 6

#define N_ROWS 5
#define N_COLS 14

#define FN1_ROW 4
#define FN1_COL 2

const uint32_t LED_PIN = PICO_DEFAULT_LED_PIN;

// Set to unused pin
const uint32_t NUMLOCK_LED_PIN = 23;

typedef uint8_t scancode_t;

uint8_t keybuffer[MAX_COINCIDENT_KEYS] = {0};
uint8_t modifiers = 0;

int config_row_map[N_ROWS] = {18, 17, 16, 14, 15};
int config_column_map[N_COLS] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 22, 21, 20, 19};

struct macro macros[] = {
    { .len = 2, .keycodes = { KEY_LEFTCTRL, KEY_C } }
};

static signed int macromap[N_ROWS][N_COLS] = {
    { MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE },
    { MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE },
    { MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE },
    { MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE },
    { MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE, MACRO_NONE }
};

static unsigned char keymap[N_ROWS][N_COLS] = {
    { KEY_1,         KEY_2,        KEY_3,    KEY_4,    KEY_5,       KEY_6,    KEY_7,    KEY_8,     KEY_9,    KEY_0,     KEY_MINUS,     KEY_EQUAL,      KEY_RIGHTCTRL,  KEY_BACKSPACE },
    { KEY_GRAVE,     KEY_Q,        KEY_W,    KEY_E,    KEY_R,       KEY_T,    KEY_Y,    KEY_U,     KEY_I,    KEY_O,     KEY_P,         KEY_LEFTBRACE,  KEY_RIGHTBRACE, KEY_BACKSLASH },
    { KEY_TAB,       KEY_A,        KEY_S,    KEY_D,    KEY_F,       KEY_G,    KEY_H,    KEY_J,     KEY_K,    KEY_L,     KEY_SEMICOLON, KEY_APOSTROPHE, KEY_NONE,       KEY_ENTER },
    { KEY_ESC,       KEY_NONE,     KEY_Z,    KEY_X,    KEY_C,       KEY_V,    KEY_B,    KEY_N,     KEY_M,    KEY_COMMA, KEY_DOT,       KEY_SLASH,      KEY_RIGHTSHIFT, KEY_RIGHTMETA },
    { KEY_LEFTSHIFT, KEY_LEFTCTRL, KEY_NONE, KEY_NONE, KEY_RIGHTALT, KEY_NONE, KEY_NONE, KEY_SPACE, KEY_NONE, KEY_NONE,  KEY_NONE,      KEY_LEFTALT,   KEY_RIGHTCTRL,  KEY_NONE }
                                   /* fn key */
};

static unsigned char layer1[N_ROWS][N_COLS] = {
    { KEY_F1,        KEY_F2,              KEY_F3,   KEY_F4,       KEY_F5,       KEY_F6,   KEY_F7,   KEY_F8,     KEY_F9,   KEY_F10,   KEY_F11,       KEY_F12,        KEY_DELETE,     KEY_HOME },
    { KEY_GRAVE,     KEY_Q,               KEY_W,    KEY_E,        KEY_R,        KEY_T,    KEY_Y,    KEY_PAGEUP, KEY_I,    KEY_O,     KEY_P,         KEY_LEFTBRACE,  KEY_RIGHTBRACE, KEY_END },
    { KEY_TAB,       KEY_A,               KEY_S,    KEY_PAGEDOWN, KEY_F,        KEY_G,    KEY_LEFT, KEY_DOWN,   KEY_UP,   KEY_RIGHT, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_NONE,       KEY_ENTER },
    { KEY_ESC,       KEY_NONE,            KEY_Z,    KEY_X,        KEY_C,        KEY_V,    KEY_B,    KEY_N,      KEY_M,    KEY_COMMA, KEY_DOT,       KEY_SLASH,      KEY_RIGHTSHIFT, KEY_LEFTMETA },
    { KEY_LEFTSHIFT, KEY_LEFTCTRL, /*fn*/ KEY_NONE, KEY_NONE,     KEY_RIGHTALT, KEY_NONE, KEY_NONE, KEY_SPACE,  KEY_NONE, KEY_NONE,  KEY_NONE,      KEY_LEFTALT,    KEY_RIGHTCTRL,  KEY_NONE }
};
