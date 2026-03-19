#include <stdio.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "nvs_flash.h"

#include "lvgl.h"
#include "demos/lv_demos.h"
#include <esp_lvgl_port.h>
#include "lvgl_ui.h"

#include "bsp_display.h"
#include "bsp_touch.h"
#include "bsp_i2c.h"
#include "bsp_wifi.h"
#include "bsp_sdcard.h"
#include "bsp_qmi8658.h"
#include "bsp_codec.h"
// #include "bsp_battery.h"
#include "bsp_power_manager.h"

#include "iot_button.h"
#include "button_gpio.h"

#define EXAMPLE_DISPLAY_ROTATION 0
#define EXAMPLE_LCD_H_RES 240
#define EXAMPLE_LCD_V_RES 240

#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)

// A buffer used for storing recorded data
#define RECORD_CHUNK_SIZE   1000
#define SAMPLE_RATE         16000
#define BITS_PER_SAMPLE     2
#define CHANNELS            1
#define RECORD_SECONDS      3
#define RECORD_BUF_SIZE     (SAMPLE_RATE * BITS_PER_SAMPLE * CHANNELS * RECORD_SECONDS)

static const char *TAG = "factory";

static button_handle_t boot_btn = NULL;
static button_handle_t pwr_btn = NULL;
static button_handle_t plus_btn = NULL;

i2c_master_bus_handle_t bus_handle;
esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_touch_handle_t touch_handle = NULL;
lv_disp_drv_t disp_drv;
lv_indev_t *indev_touchpad;

lv_display_t *lvgl_disp = NULL;
lv_indev_t *lvgl_touch_indev = NULL;

SemaphoreHandle_t codec_recording_BinarySemaphore;
SemaphoreHandle_t codec_stop_BinarySemaphore;

static uint8_t *record_buf = NULL;
static int record_total_size = 0;
static bool has_recording = false;


void lv_port_init(void);

extern lv_obj_t *tileview;
static uint32_t col_id = 0;
extern SemaphoreHandle_t wifi_scanf_semaphore;

static void button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    ESP_LOGI(TAG, "BOOT KEY %s", iot_button_get_event_str(event));
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static bool long_press_flag = false;

    if (event == BUTTON_SINGLE_CLICK)
    {
        col_id = (col_id + 3) % 4;
        if (lvgl_port_lock(0))
        {
            lv_obj_set_tile_id(tileview, col_id, 0, LV_ANIM_ON);
            lvgl_port_unlock();
        }
    }
    else if (event == BUTTON_LONG_PRESS_START)
    {
        xSemaphoreGiveFromISR(codec_recording_BinarySemaphore, &xHigherPriorityTaskWoken);
    }
}

static void pwr_button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    ESP_LOGI(TAG, "PWR KEY %s", iot_button_get_event_str(event));
    static uint8_t brightness_last = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (event == BUTTON_LONG_PRESS_START)
    {
        bsp_power_off();
    }
    else if (event == BUTTON_SINGLE_CLICK)
    {
        if (col_id == 3)
        {
            xSemaphoreGiveFromISR(wifi_scanf_semaphore, &xHigherPriorityTaskWoken);
        }
        
    }
    else if (event == BUTTON_DOUBLE_CLICK)
    {
        if (bsp_display_get_brightness() == 0)
        {
            if (brightness_last == 0)
            {
                bsp_display_set_brightness(80);
            }
            else
            {
                bsp_display_set_brightness(brightness_last);
            }
            brightness_last = 0;
        }
        else
        {
            brightness_last = bsp_display_get_brightness();
            bsp_display_set_brightness(0);
        }
    }
    else if (event == BUTTON_PRESS_UP)
    {
        if (touch_handle == NULL)
        {
            iot_button_register_cb(pwr_btn, BUTTON_SINGLE_CLICK, NULL, pwr_button_event_cb, NULL);
        }
        iot_button_register_cb(pwr_btn, BUTTON_LONG_PRESS_START, NULL, pwr_button_event_cb, NULL);
        iot_button_register_cb(pwr_btn, BUTTON_DOUBLE_CLICK, NULL, pwr_button_event_cb, NULL);
        iot_button_unregister_cb(pwr_btn, BUTTON_PRESS_UP, NULL);
    }
}

static void plus_button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    ESP_LOGI(TAG, "PLUS KEY %s", iot_button_get_event_str(event));
    if (event == BUTTON_LONG_PRESS_START)
    {
    }
    else if (event == BUTTON_SINGLE_CLICK)
    {
        col_id = (col_id + 1) % 4;
        if (lvgl_port_lock(0))
        {
            lv_obj_set_tile_id(tileview, col_id, 0, LV_ANIM_ON);
            lvgl_port_unlock();
        }
    }
}

static void button_init(void)
{
    button_config_t btn_cfg = {};
    button_gpio_config_t btn_gpio_cfg = {};
    btn_gpio_cfg.gpio_num = GPIO_NUM_0;
    btn_gpio_cfg.active_level = 0;
    // static button_handle_t btn = NULL;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &boot_btn));

    iot_button_register_cb(boot_btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb, NULL);
    // iot_button_register_cb(boot_btn, BUTTON_LONG_PRESS_HOLD, NULL, button_event_cb, NULL);
    // iot_button_register_cb(boot_btn, BUTTON_LONG_PRESS_UP, NULL, button_event_cb, NULL);
    iot_button_register_cb(boot_btn, BUTTON_PRESS_END, NULL, button_event_cb, NULL);

    btn_gpio_cfg.gpio_num = GPIO_NUM_5;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &pwr_btn));
    iot_button_register_cb(pwr_btn, BUTTON_PRESS_UP, NULL, pwr_button_event_cb, NULL);

    btn_gpio_cfg.gpio_num = GPIO_NUM_4;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &plus_btn));

    if (touch_handle == NULL)
    {
        iot_button_register_cb(boot_btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
        iot_button_register_cb(plus_btn, BUTTON_SINGLE_CLICK, NULL, plus_button_event_cb, NULL);
    }
}

void es8311_test_task(void *arg)
{
    const int chunk_size = RECORD_CHUNK_SIZE;
    uint8_t temp[chunk_size];

    while (1)
    {
        //Wait for long press to start
        if (xSemaphoreTake(codec_recording_BinarySemaphore, portMAX_DELAY) == pdTRUE)
        {
            if (!has_recording)
            {
                //  First long press: Record for 3 seconds
                ESP_LOGI("codec", "Start recording...");
                bsp_codec_set_in_volume(70.0);
                record_total_size = 0;

                TickType_t start_tick = xTaskGetTickCount();
                while ((xTaskGetTickCount() - start_tick) < pdMS_TO_TICKS(3000))
                {
                    bsp_codec_recording(temp, chunk_size);
                    if (record_total_size + chunk_size <= RECORD_BUF_SIZE)
                    {
                        memcpy(record_buf + record_total_size, temp, chunk_size);
                        record_total_size += chunk_size;
                    }
                    else
                    {
                        break;
                    }
                }

                bsp_codec_set_in_volume(0.0);
                has_recording = true;
                ESP_LOGI("codec", "Recording done, size=%d", record_total_size);
            }
            else
            {
                // Second long press: Play the recording
                ESP_LOGI("codec", "Start playing...");
                bsp_codec_set_out_volume(100.0);

                int offset = 0;
                while (offset < record_total_size)
                {
                    int play_size = (record_total_size - offset) > chunk_size
                                    ? chunk_size
                                    : (record_total_size - offset);
                    bsp_codec_playing(record_buf + offset, play_size);
                    offset += play_size;
                }

                bsp_codec_set_out_volume(0.0);
                has_recording = false;
                ESP_LOGI("codec", "Playback done");
            }
        }
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2c_master_bus_handle_t i2c_bus_handle;
    i2c_bus_handle = bsp_i2c_init();

    codec_recording_BinarySemaphore = xSemaphoreCreateBinary();
    codec_stop_BinarySemaphore = xSemaphoreCreateBinary();

    bsp_display_brightness_init();
    bsp_display_set_brightness(100);

    bsp_power_manager_init();
    bsp_qmi8658_init(i2c_bus_handle);
    bsp_sdcard_init();
    bsp_codec_init(i2c_bus_handle);

    bsp_wifi_init("WSTEST", "waveshare0755");
    button_init();

    bsp_display_init(&io_handle, &panel_handle, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT);
    bsp_touch_init(&touch_handle, i2c_bus_handle, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, EXAMPLE_DISPLAY_ROTATION, NULL);
    lv_port_init();

    record_buf = (uint8_t *)heap_caps_malloc(RECORD_BUF_SIZE, MALLOC_CAP_8BIT);
    assert(record_buf != NULL);
    xTaskCreate(es8311_test_task, "es8311_test_task", 1024 * 8, NULL, 5, NULL);

    if (lvgl_port_lock(0))
    {
        lvgl_ui_init();
        // lv_demo_benchmark();
        // lv_demo_music();
        // lv_demo_widgets();
        lvgl_port_unlock();
    }
}

void lv_port_init(void)
{
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&port_cfg);
    ESP_LOGI(TAG, "Adding LCD screen");
    lvgl_port_display_cfg_t display_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .control_handle = NULL,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = true,
        .trans_size = 0,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };
    lvgl_disp = lvgl_port_add_disp(&display_cfg);
#if EXAMPLE_DISPLAY_ROTATION == 90
    lv_disp_set_rotation(lvgl_disp, LV_DISP_ROT_90);
#elif EXAMPLE_DISPLAY_ROTATION == 180
    lv_disp_set_rotation(lvgl_disp, LV_DISP_ROT_180);
#elif EXAMPLE_DISPLAY_ROTATION == 270
    lv_disp_set_rotation(lvgl_disp, LV_DISP_ROT_270);
#else
    lv_disp_set_rotation(lvgl_disp, LV_DISP_ROT_NONE);
#endif

    if (touch_handle)
    {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lvgl_disp,
            .handle = touch_handle,
        };
        lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
    }
}