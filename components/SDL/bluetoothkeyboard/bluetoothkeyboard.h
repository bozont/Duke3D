#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_hidh.h"

static const char *TAG = "ESP_HID_GAP";
#include "esp_hid_gap.h"

xQueueHandle btkeyboard_evt_queue = NULL;
#include "SDL_input.h"
#include "SDL_event.h"

typedef struct {
    SDL_Scancode scancode;
    SDL_Keycode keycode;
    uint8_t keyboard_scancode;
    char key_as_text[10];
    bool was_pressed;
} btKeyMapElement;

#define KBD_SCANCODE_UP         0x52
#define KBD_SCANCODE_RIGHT      0x4f
#define KBD_SCANCODE_DOWN       0x51
#define KBD_SCANCODE_LEFT       0x50
#define KBD_SCANCODE_LCRTL      0x01
#define KBD_SCANCODE_e          0x08
#define KBD_SCANCODE_ESCAPE     0x29
#define KBD_SCANCODE_SPACE      0x2c
#define KBD_SCANCODE_0          0x27
#define KBD_SCANCODE_1          0x1e
#define KBD_SCANCODE_2          0x1f
#define KBD_SCANCODE_3          0x20
#define KBD_SCANCODE_4          0x21
#define KBD_SCANCODE_5          0x22
#define KBD_SCANCODE_6          0x23
#define KBD_SCANCODE_7          0x24
#define KBD_SCANCODE_8          0x25
#define KBD_SCANCODE_9          0x26
#define KBD_SCANCODE_m          0x10
#define KBD_SCANCODE_LSHIFT     0x02

#define BT_KEY_MAX 20

btKeyMapElement bt_keymap[BT_KEY_MAX] = {
    {SDL_SCANCODE_UP,       SDLK_UP,        KBD_SCANCODE_UP,        "UP",       false},
    {SDL_SCANCODE_RIGHT,    SDLK_RIGHT,     KBD_SCANCODE_RIGHT,     "RIGHT",    false},
    {SDL_SCANCODE_DOWN,     SDLK_DOWN,      KBD_SCANCODE_DOWN,      "DOWN",     false},
    {SDL_SCANCODE_LEFT,     SDLK_LEFT,      KBD_SCANCODE_LEFT,      "LEFT",     false},
    {SDL_SCANCODE_LCTRL,    SDLK_LCTRL,     KBD_SCANCODE_LCRTL,     "FIRE",     false},
    {SDL_SCANCODE_SPACE,    SDLK_SPACE,     KBD_SCANCODE_e,         "USE",      false},
    {SDL_SCANCODE_ESCAPE,   SDLK_ESCAPE,    KBD_SCANCODE_ESCAPE,    "ESC",      false},
    {SDL_SCANCODE_A,        SDLK_a,         KBD_SCANCODE_SPACE,     "JUMP",     false},
    {SDL_SCANCODE_KP_1,     SDLK_1,         KBD_SCANCODE_1,         "1",        false},
    {SDL_SCANCODE_KP_2,     SDLK_2,         KBD_SCANCODE_2,         "2",        false},
    {SDL_SCANCODE_KP_3,     SDLK_3,         KBD_SCANCODE_3,         "3",        false},
    {SDL_SCANCODE_KP_4,     SDLK_4,         KBD_SCANCODE_4,         "4",        false},
    {SDL_SCANCODE_KP_5,     SDLK_5,         KBD_SCANCODE_5,         "5",        false},
    {SDL_SCANCODE_KP_6,     SDLK_6,         KBD_SCANCODE_6,         "6",        false},
    {SDL_SCANCODE_KP_7,     SDLK_7,         KBD_SCANCODE_7,         "7",        false},
    {SDL_SCANCODE_KP_8,     SDLK_8,         KBD_SCANCODE_8,         "8",        false},
    {SDL_SCANCODE_KP_9,     SDLK_9,         KBD_SCANCODE_9,         "9",        false},
    {SDL_SCANCODE_KP_0,     SDLK_0,         KBD_SCANCODE_0,         "0",        false},
    {SDL_SCANCODE_M,        SDLK_m,         KBD_SCANCODE_m,         "m",        false},
    {SDL_SCANCODE_Z,        SDLK_z,         KBD_SCANCODE_LSHIFT,    "CROUCH",   false},
};


void btkeyboard_queue_init() {
    btkeyboard_evt_queue = xQueueCreate(BT_KEY_MAX * 2, sizeof(BtKeyEvent));
}

void post_bt_key_event(int sdl_scancode, int sdl_keycode, int key_event_type) {
    BtKeyEvent ev;
    ev.scancode = sdl_scancode;
    ev.keycode = sdl_keycode;
    ev.type = key_event_type;
    xQueueSend(btkeyboard_evt_queue, &ev, NULL);
}

void process_bt_key_event(uint8_t* data, size_t size) {

    for(uint32_t i = 0; i < BT_KEY_MAX; i++) {
        if(bt_keymap[i].was_pressed) {
            bt_keymap[i].was_pressed = false;
            post_bt_key_event(bt_keymap[i].scancode, bt_keymap[i].keycode, SDL_KEYUP);
        }
    }

    for(size_t i = 0; i < size; i++) {
        for(uint32_t j = 0; j < BT_KEY_MAX; j++) {
            if(bt_keymap[j].keyboard_scancode == data[i]) {
                bt_keymap[j].was_pressed = true;
                post_bt_key_event(bt_keymap[j].scancode, bt_keymap[j].keycode, SDL_KEYDOWN);
                ets_printf("BluetoothKeyboard: %s\n", bt_keymap[j].key_as_text);
                break;
            }
        }
    }

}

void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDH_OPEN_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->open.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " OPEN: %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->open.dev));
        esp_hidh_dev_dump(param->open.dev, stdout);
        break;
    }
    case ESP_HIDH_BATTERY_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->battery.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " BATTERY: %d%%", ESP_BD_ADDR_HEX(bda), param->battery.level);
        break;
    }
    case ESP_HIDH_INPUT_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " INPUT: %8s, MAP: %2u, ID: %3u, Len: %d, Data:", ESP_BD_ADDR_HEX(bda), esp_hid_usage_str(param->input.usage), param->input.map_index, param->input.report_id, param->input.length);
        ESP_LOG_BUFFER_HEX(TAG, param->input.data, param->input.length);
        process_bt_key_event(param->input.data, param->input.length);
        break;
    }
    case ESP_HIDH_FEATURE_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->feature.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " FEATURE: %8s, MAP: %2u, ID: %3u, Len: %d", ESP_BD_ADDR_HEX(bda), esp_hid_usage_str(param->feature.usage), param->feature.map_index, param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDH_CLOSE_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->close.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " CLOSE: '%s' %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->close.dev), esp_hid_disconnect_reason_str(esp_hidh_dev_transport_get(param->close.dev), param->close.reason));
        //MUST call this function to free all allocated memory by this device
        esp_hidh_dev_free(param->close.dev);
        break;
    }
    default:
        ESP_LOGI(TAG, "EVENT: %d", event);
        break;
    }
}

#define SCAN_DURATION_SECONDS 5

void bluetooth_task() {
    size_t results_len = 0;
    esp_hid_scan_result_t *results = NULL;
    ESP_LOGI(TAG, "SCAN...");
    //start scan for HID devices
    esp_hid_scan(SCAN_DURATION_SECONDS, &results_len, &results);
    ESP_LOGI(TAG, "SCAN: %u results", results_len);
    if (results_len) {
        esp_hid_scan_result_t *r = results;
        esp_hid_scan_result_t *cr = NULL;
        while (r) {
            printf("  %s: " ESP_BD_ADDR_STR ", ", (r->transport == ESP_HID_TRANSPORT_BLE) ? "BLE" : "BT ", ESP_BD_ADDR_HEX(r->bda));
            printf("RSSI: %d, ", r->rssi);
            printf("USAGE: %s, ", esp_hid_usage_str(r->usage));
            if (r->transport == ESP_HID_TRANSPORT_BLE) {
                cr = r;
                printf("APPEARANCE: 0x%04x, ", r->ble.appearance);
                printf("ADDR_TYPE: '%s', ", ble_addr_type_str(r->ble.addr_type));
            } else {
                cr = r;
                printf("COD: %s[", esp_hid_cod_major_str(r->bt.cod.major));
                esp_hid_cod_minor_print(r->bt.cod.minor, stdout);
                printf("] srv 0x%03x, ", r->bt.cod.service);
                print_uuid(&r->bt.uuid);
                printf(", ");
            }
            printf("NAME: %s ", r->name ? r->name : "");
            printf("\n");
            r = r->next;
        }
        if (cr) {
            //open the last result
            esp_hidh_dev_open(cr->bda, cr->transport, cr->ble.addr_type);
        }
        //free the results
        esp_hid_scan_results_free(results);
    }

}
