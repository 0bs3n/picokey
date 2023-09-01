#include "config.h"
#include "pico/bootrom.h"

unsigned char coord_to_scan_code(int column, int row, bool fn) { 
    return fn ? layer1[row][column] : keymap[row][column]; 
}

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

unsigned int blink_interval_ms = BLINK_NOT_MOUNTED;

bool numlock_on = false;

unsigned int led_pwm_on_us = 1;
unsigned int led_pwm_off_us = 10;

const char *scancode_to_string(int scancode) {
    switch (scancode) {
    case KEY_NUMLOCK:
        return "KEY_NUMLOCK";
    case KEY_KPSLASH:
        return "KEY_KPSLASH";
    case KEY_KPASTERISK:
        return "KEY_KPASTERISK";
    case KEY_KPMINUS:
        return "KEY_KPMINUS";
    case KEY_KPPLUS:
        return "KEY_KPPLUS";
    case KEY_KPENTER:
        return "KEY_KPENTER";
    case KEY_KPDOT:
        return "KEY_KPDOT";
    case KEY_KP0:
        return "KEY_KP0";
    case KEY_KP1:
        return "KEY_KP1";
    case KEY_KP2:
        return "KEY_KP2";
    case KEY_KP3:
        return "KEY_KP3";
    case KEY_KP4:
        return "KEY_KP4";
    case KEY_KP5:
        return "KEY_KP5";
    case KEY_KP6:
        return "KEY_KP6";
    case KEY_KP7:
        return "KEY_KP7";
    case KEY_KP8:
        return "KEY_KP8";
    case KEY_KP9:
        return "KEY_KP9";
    case KEY_NONE:
        return "KEY_NONE";
    default:
        return "UNKNOWN";
    }
}

int 
get_macro(int row, int col)
{
    // keymap overrides macromap, if a key
    // is defined for the given coordinate
    // return MACRO_NONE regardless of defined
    // macros
    if (keymap[row][col] != KEY_NONE) 
        return MACRO_NONE;

    return macromap[row][col];
}

bool
fn_key_state(int *columns, int *rows)
{
    bool fn_pressed = false;

    gpio_put(columns[FN1_COL], 1);
    sleep_us(10);
    fn_pressed = gpio_get(rows[FN1_ROW]);
    gpio_put(columns[FN1_COL], 0);

    return fn_pressed;
}

void 
check_special_reset_bootloader(int *columns, int *rows)
{
    bool key1, key2, key3;

    // Fn key
    gpio_put(columns[FN1_COL], 1);
    sleep_us(10);
    key1 = gpio_get(rows[FN1_ROW]);
    gpio_put(columns[FN1_COL], 0);
    sleep_us(10);

    // backtick key
    gpio_put(columns[0], 1);
    sleep_us(10);
    key2 = gpio_get(rows[1]);
    gpio_put(columns[0], 0);
    sleep_us(10);
        
    // backspace key
    gpio_put(columns[13], 1);
    sleep_us(10);
    key3 = gpio_get(rows[0]);
    gpio_put(columns[13], 0);
    sleep_us(10);

    if (key1 && key2 && key3)
        reset_usb_boot(0, 0);
}


int 
poll_columns(int *columns, int *rows, int n_cols, int n_rows) 
{
    check_special_reset_bootloader(columns, rows);

    int current_key_index = 0;
    memset(keybuffer, 0x0, MAX_COINCIDENT_KEYS);

    modifiers = 0;

    int macro_idx;
    struct macro macro;

    // Check special keys:

    // Get Fn key state
    bool fn_state = fn_key_state(columns, rows);

    // poll columns
    for (int col = 0; col < n_cols; ++col) {
        gpio_put(columns[col], 1);
        sleep_us(10);
        for (int row = 0; row < n_rows; ++row) {
            int switch_status = gpio_get(rows[row]);
            if (switch_status) {
                if (current_key_index >= MAX_COINCIDENT_KEYS) {
                    memset(keybuffer, 0x01, MAX_COINCIDENT_KEYS);
                    return -1; // too many keys pressed
                }

                scancode_t current_scancode = coord_to_scan_code(col, row, fn_state);

                // Determine if the pressed key is a modifier, and if so set modifier bits
                if ((0xe0 & current_scancode) == 0xe0 && (current_scancode & 0x07) < 8) {
                    modifiers |= 1 << (current_scancode & 7);

                // else determine if key pressed is a macro key, if so set keybuffer
                // accordingly
                } else if ((macro_idx = get_macro(row, col)) >= 0) {
                    macro = macros[macro_idx];
                    if (current_key_index < MAX_COINCIDENT_KEYS - macro.len)
                        for (int i = 0; i < macro.len; i++)
                            keybuffer[current_key_index++] = macro.keycodes[i];
                    else
                        return -1; // too many keys pressed
                                   
                // otherwise set keybuffer with keycode for pressed key
                } else {
                    keybuffer[current_key_index] = current_scancode;
                    current_key_index++;
                }
            }
        }
        gpio_put(columns[col], 0);
        sleep_us(10);
    }
    return 0;
}

void 
keypins_init()
{
    puts("Inside keypins_init");
    for (int i = 0; i < N_ROWS; ++i) {
        gpio_init(config_row_map[i]);
        gpio_set_dir(config_row_map[i], GPIO_IN);
        gpio_pull_down(config_row_map[i]);
    }

    puts("after keypins_init rows_init");

    for (int i = 0; i < N_COLS; ++i) {
        gpio_init(config_column_map[i]);
        gpio_set_dir(config_column_map[i], GPIO_OUT);
        gpio_put(config_column_map[i], 0);
    }
    puts("End keypins_init");
}

void 
board_led_write(bool state) 
{
    gpio_put(LED_PIN, state);
}

void 
board_init() 
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(NUMLOCK_LED_PIN);
    gpio_set_dir(NUMLOCK_LED_PIN, GPIO_OUT);
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void 
led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

static void 
send_hid_report(uint8_t report_id) 
{
    if ( !tud_hid_ready() ) return;
    tud_hid_keyboard_report(report_id, modifiers, keybuffer);
}

// Every 10ms, we will send 1 report for each HID profile
// tud_hid_report_complete_cb() is used to send the next 
// report after the previous one is complete
void 
hid_task(void) 
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    int poll_status = poll_columns(config_column_map, config_row_map, N_COLS, N_ROWS);
    if (poll_status != 0) printf("poll status: %d\n", poll_status);
    /*
    printf("[%02x | %02x | %02x | %02x | %02x | %02x\n", 
            keybuffer[0],
            keybuffer[1],
            keybuffer[2],
            keybuffer[3],
            keybuffer[4],
            keybuffer[5]);
            */
    send_hid_report(REPORT_ID_KEYBOARD);
}

uint64_t
board_us(void) 
{
    return to_us_since_boot(get_absolute_time());
}

void 
led_pwm_task(void)
{
    static uint32_t start_us = 0;
    if (numlock_on) {
        bool led_state = gpio_get(NUMLOCK_LED_PIN);
        if (led_state) {
            if ( board_us() - start_us < led_pwm_on_us) return; // not enough time
            start_us += led_pwm_on_us;
            gpio_put(NUMLOCK_LED_PIN, 0);
        } else {
            if ( board_us() - start_us < led_pwm_off_us) return; // not enough time
            start_us += led_pwm_off_us;
            gpio_put(NUMLOCK_LED_PIN, 1);
        }
    }
}

int 
main(void) 
{
    board_init();
    puts("BOARD_INIT");
    keypins_init();
    puts("KEYS_INIT");
    tusb_init();
    puts("TUSB_INIT");

    while (1) {
        tud_task();
        hid_task();
        led_blinking_task();
        // led_pwm_task();
    }
    return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}


//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}


/*
void 
tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) itf;
    (void) report_id;
    (void) report_type;

    tud_hid_report(0, buffer, bufsize);
}
*/
// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) itf;

    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        // Set keyboard LED e.g. CAPSLOCK, NUMLOCK, etc.
        if (bufsize < 1) return;
       //uint8_t const kbd_leds = buffer[0]; // on my machine it looks like the second byte does this
        uint8_t const kbd_leds = buffer[1];
        if (kbd_leds & KEYBOARD_LED_NUMLOCK) {
            blink_interval_ms = 0;
            board_led_write(1);
            numlock_on = true;
        } else {
            blink_interval_ms = BLINK_MOUNTED;
            board_led_write(0);
            numlock_on = false;
            gpio_put(NUMLOCK_LED_PIN, 0);
        }
    }
}
