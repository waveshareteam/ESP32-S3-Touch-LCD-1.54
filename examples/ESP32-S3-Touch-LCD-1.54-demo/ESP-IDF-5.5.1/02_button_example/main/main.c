/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "iot_button.h"
#include "esp_sleep.h"
#include "esp_idf_version.h"
#include "button_gpio.h"
#include "esp_log.h"
#include "esp_check.h"

/* Most development boards have "boot" button attached to GPIO0.
 * You can also change this to another pin.
 */
#define BOOT_BUTTON_NUM         0
#define PWR_BUTTON_NUM          5
#define PLUS_BUTTON_NUM         4
button_handle_t boot_btn;
button_handle_t pwr_btn;
button_handle_t plus_btn;


static const char *TAG = "button_example";

static const char *button_event_str[] = {
    "BUTTON_PRESS_DOWN",
    "BUTTON_PRESS_UP",
    "BUTTON_PRESS_REPEAT",
    "BUTTON_PRESS_REPEAT_DONE",
    "BUTTON_SINGLE_CLICK",
    "BUTTON_DOUBLE_CLICK",
    "BUTTON_MULTIPLE_CLICK",
    "BUTTON_LONG_PRESS_START",
    "BUTTON_LONG_PRESS_HOLD",
    "BUTTON_LONG_PRESS_UP",
    "BUTTON_PRESS_END",
    "BUTTON_EVENT_MAX",
    "BUTTON_NONE_PRESS",
};

static void button_event_cb(void *arg, void *data)
{
    if (arg == boot_btn)
    {
        ESP_LOGI(TAG, "Left Button event: %s", button_event_str[iot_button_get_event((button_handle_t)arg)]); 
    }
    else if (arg == pwr_btn)
    {
        ESP_LOGI(TAG, "Middle Button event: %s", button_event_str[iot_button_get_event((button_handle_t)arg)]); 
    }
    else if (arg == plus_btn)
    {
        ESP_LOGI(TAG, "Right Button event: %s", button_event_str[iot_button_get_event((button_handle_t)arg)]); 
    }
}
void button_init(void)
{
    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BOOT_BUTTON_NUM,
        .active_level = 0,
        .enable_power_save = true,
    };

    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &boot_btn);
    assert(ret == ESP_OK);

    gpio_cfg.gpio_num = PWR_BUTTON_NUM;
    ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &pwr_btn);
    assert(ret == ESP_OK);

    gpio_cfg.gpio_num = PLUS_BUTTON_NUM;
    ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &plus_btn);
    assert(ret == ESP_OK);

    ret = iot_button_register_cb(boot_btn, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_PRESS_REPEAT, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_PRESS_REPEAT_DONE, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_DOUBLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_LONG_PRESS_HOLD, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_LONG_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(boot_btn, BUTTON_PRESS_END, NULL, button_event_cb, NULL);

    ret = iot_button_register_cb (pwr_btn, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_PRESS_REPEAT, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_PRESS_REPEAT_DONE, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_DOUBLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_LONG_PRESS_HOLD, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_LONG_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(pwr_btn, BUTTON_PRESS_END, NULL, button_event_cb, NULL);

    ret = iot_button_register_cb (plus_btn, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_PRESS_REPEAT, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_PRESS_REPEAT_DONE, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_DOUBLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_LONG_PRESS_HOLD, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_LONG_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(plus_btn, BUTTON_PRESS_END, NULL, button_event_cb, NULL);
}


void app_main(void)
{
    button_init();
}
