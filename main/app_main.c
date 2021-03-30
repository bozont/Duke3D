
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bluetoothkeyboard.h"
#include "game.h"
#include "assert.h"


void dukeTask(void *pvParameters)
{
    bluetooth_task();

    char *argv[]={"duke3d", "/nm", NULL};
    main(2, argv);

    // no sound
    //char *argv[]={"duke3d", "/ns", "/nm", NULL};
    //main(3, argv);
   
}


void app_main(void)
{
    /*
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    */
    ESP_ERROR_CHECK( esp_hid_gap_init(ESP_BT_MODE_BTDM) );
    ESP_ERROR_CHECK( esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler) );
    esp_hidh_config_t config = {
        .callback = hidh_callback,
    };
    ESP_ERROR_CHECK( esp_hidh_init(&config) );
    btkeyboard_queue_init();

    xTaskCreatePinnedToCore(&dukeTask, "dukeTask", 32000, NULL, 5, NULL, 0/*tskNO_AFFINITY*/);
}
