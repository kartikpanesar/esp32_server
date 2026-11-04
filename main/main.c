#include "nvs_flash.h"
#include <esp_task.h>

void wifi_init_sta(void);
void server_task(void *args);

void app_main(void)
{
 // Wi-Fi stores calibration/config in NVS, so this is required.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    wifi_init_sta();              // returns once we have an IP

    xTaskCreate(server_task, "chat_server", 6144, NULL, 5, NULL);

}
