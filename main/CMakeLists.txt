# idf_component_register(SRCS "esp32_server.c"
#                     INCLUDE_DIRS ".")
#
idf_component_register(SRCS "main.c" "server.c" "client_ds.c" "wifi.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_netif esp_event nvs_flash lwip)
