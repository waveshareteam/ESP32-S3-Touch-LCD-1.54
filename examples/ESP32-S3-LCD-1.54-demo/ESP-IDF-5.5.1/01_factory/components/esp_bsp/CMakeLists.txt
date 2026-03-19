file(GLOB_RECURSE SRC_FILES "*.c")

idf_component_register(SRCS ${SRC_FILES}
                    INCLUDE_DIRS "."
                    REQUIRES "esp_lcd" "driver" "esp_adc"  "espressif__esp_lcd_touch_cst816s"  "fatfs" "nvs_flash" "lwip" "esp_wifi" "espressif__esp_codec_dev")
