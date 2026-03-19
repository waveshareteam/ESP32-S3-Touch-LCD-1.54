/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "string.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

// #include "esp_lcd_st7796.h"
#include "esp_lcd_touch_cst816s.h"
// #include "esp_io_expander_tca9554.h"
#include "demos/lv_demos.h"
/* LCD size */
#define EXAMPLE_LCD_H_RES (240)
#define EXAMPLE_LCD_V_RES (240)

/* LCD settings */
#define EXAMPLE_LCD_SPI_NUM (SPI3_HOST)
#define EXAMPLE_LCD_PIXEL_CLK_HZ (40 * 1000 * 1000)
#define EXAMPLE_LCD_CMD_BITS (8)
#define EXAMPLE_LCD_PARAM_BITS (8)
#define EXAMPLE_LCD_COLOR_SPACE (ESP_LCD_COLOR_SPACE_RGB)
#define EXAMPLE_LCD_BITS_PER_PIXEL (16)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_BL_ON_LEVEL (1)

/* LCD pins */
#define EXAMPLE_LCD_GPIO_SCLK (GPIO_NUM_38)
#define EXAMPLE_LCD_GPIO_MOSI (GPIO_NUM_39)
#define EXAMPLE_LCD_GPIO_RST (GPIO_NUM_40)
#define EXAMPLE_LCD_GPIO_DC (GPIO_NUM_45)
#define EXAMPLE_LCD_GPIO_CS (GPIO_NUM_21)
#define EXAMPLE_LCD_GPIO_BL (GPIO_NUM_46)

/* Touch settings */
#define EXAMPLE_TOUCH_I2C_NUM (0)
#define EXAMPLE_TOUCH_I2C_CLK_HZ (400000)

/* LCD touch pins */
#define EXAMPLE_TOUCH_I2C_SCL (GPIO_NUM_41)
#define EXAMPLE_TOUCH_I2C_SDA (GPIO_NUM_42)
#define EXAMPLE_TOUCH_GPIO_INT (GPIO_NUM_48)
#define EXAMPLE_TOUCH_GPIO_RST (GPIO_NUM_47)

static const char *TAG = "EXAMPLE";

// LVGL image declare
LV_IMG_DECLARE(esp_logo)

/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

/* LVGL display and touch */
static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

i2c_master_bus_handle_t i2c_bus_handle = NULL;
static esp_err_t app_lcd_init(void)
{

    // const st7796_lcd_init_cmd_t lcd_init_cmds[] = {
    //     // {cmd, { data }, data_size, delay_ms}
    //     {0x11, (uint8_t[]){0x00}, 0, 120},
    //     // {0x36, (uint8_t []){ 0x08 }, 1, 0},

    //     {0x3A, (uint8_t[]){0x05}, 1, 0},
    //     {0xF0, (uint8_t[]){0xC3}, 1, 0},
    //     {0xF0, (uint8_t[]){0x96}, 1, 0},
    //     {0xB4, (uint8_t[]){0x01}, 1, 0},
    //     {0xB7, (uint8_t[]){0xC6}, 1, 0},
    //     {0xC0, (uint8_t[]){0x80, 0x45}, 2, 0},
    //     {0xC1, (uint8_t[]){0x13}, 1, 0},
    //     {0xC2, (uint8_t[]){0xA7}, 1, 0},
    //     {0xC5, (uint8_t[]){0x0A}, 1, 0},
    //     {0xE8, (uint8_t[]){0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8, 0},
    //     {0xE0, (uint8_t[]){0xD0, 0x08, 0x0F, 0x06, 0x06, 0x33, 0x30, 0x33, 0x47, 0x17, 0x13, 0x13, 0x2B, 0x31}, 14, 0},
    //     {0xE1, (uint8_t[]){0xD0, 0x0A, 0x11, 0x0B, 0x09, 0x07, 0x2F, 0x33, 0x47, 0x38, 0x15, 0x16, 0x2C, 0x32}, 14, 0},
    //     {0xF0, (uint8_t[]){0x3C}, 1, 0},
    //     {0xF0, (uint8_t[]){0x69}, 1, 120},
    //     {0x21, (uint8_t[]){0x00}, 0, 0},
    //     {0x29, (uint8_t[]){0x00}, 0, 0},
    // };

    esp_err_t ret = ESP_OK;

    /* LCD backlight */
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_LCD_GPIO_BL};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    /* LCD initialization */
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_GPIO_SCLK,
        .mosi_io_num = EXAMPLE_LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_GPIO_DC,
        .cs_gpio_num = EXAMPLE_LCD_GPIO_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    // st7796_vendor_config_t vendor_config = {
    //     // Uncomment these lines if use custom initialization commands
    //     .init_cmds = lcd_init_cmds,
    //     .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7796_lcd_init_cmd_t),
    // };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_GPIO_RST,
        .color_space = EXAMPLE_LCD_COLOR_SPACE,
        .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
        // .vendor_config = &vendor_config,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    // esp_lcd_panel_mirror(lcd_panel, true, true);
    esp_lcd_panel_invert_color(lcd_panel, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    /* LCD backlight on */
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_LCD_GPIO_BL, EXAMPLE_LCD_BL_ON_LEVEL));

    return ret;

err:
    if (lcd_panel)
    {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io)
    {
        esp_lcd_panel_io_del(lcd_io);
    }
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    return ret;
}

static esp_err_t app_touch_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = (i2c_port_num_t)EXAMPLE_TOUCH_I2C_NUM;
    i2c_mst_config.scl_io_num = EXAMPLE_TOUCH_I2C_SCL;
    i2c_mst_config.sda_io_num = EXAMPLE_TOUCH_I2C_SDA;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = 1;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));

    esp_lcd_panel_io_handle_t touch_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t touch_io_config = {};
    touch_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_CST816S_ADDRESS,
    touch_io_config.control_phase_bytes = 1;
    touch_io_config.dc_bit_offset = 0;
    touch_io_config.lcd_cmd_bits = 8;
    touch_io_config.flags.disable_control_phase = 1;
    touch_io_config.scl_speed_hz = 400 * 1000;

    /* Touch IO handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &touch_io_config, &touch_io_handle));
    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max = EXAMPLE_LCD_H_RES;
    tp_cfg.y_max = EXAMPLE_LCD_V_RES;
    tp_cfg.rst_gpio_num = EXAMPLE_TOUCH_GPIO_RST;
    tp_cfg.int_gpio_num = EXAMPLE_TOUCH_GPIO_INT;
    tp_cfg.flags.swap_xy = 0;
    tp_cfg.flags.mirror_x = 0;
    tp_cfg.flags.mirror_y = 0;
    return esp_lcd_touch_new_i2c_cst816s(touch_io_handle, &tp_cfg, &touch_handle);
}

static esp_err_t app_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       /* LVGL task priority */
        .task_stack = 1024 * 10,  /* LVGL task stack size */
        .task_affinity = -1,      /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 5      /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
        }};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    /* Add touch input (for selected screen) */
    if (touch_handle != NULL)
    {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lvgl_disp,
            .handle = touch_handle,
        };
        lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
    }
    return ESP_OK;
}

static void _app_button_cb(lv_event_t *e)
{
    lv_disp_rotation_t rotation = lv_disp_get_rotation(lvgl_disp);
    rotation++;
    if (rotation > LV_DISPLAY_ROTATION_270)
    {
        rotation = LV_DISPLAY_ROTATION_0;
    }

    /* LCD HW rotation */
    lv_disp_set_rotation(lvgl_disp, rotation);
}

static void app_main_display(void)
{
    char str[20];
    lv_obj_t *scr = lv_scr_act();

    /* Your LVGL objects code here .... */

    /* Create image */
    lv_obj_t *img_logo = lv_img_create(scr);
    lv_img_set_src(img_logo, &esp_logo);
    lv_obj_align(img_logo, LV_ALIGN_TOP_MID, 0, 20);

    /* Label */
    lv_obj_t *label = lv_label_create(scr);
    lv_obj_set_width(label, EXAMPLE_LCD_H_RES);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
#if LVGL_VERSION_MAJOR == 8
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, "#FF0000 " LV_SYMBOL_BELL " Hello world Espressif and LVGL " LV_SYMBOL_BELL "#\n#FF9400 " LV_SYMBOL_WARNING " For simplier initialization, use BSP " LV_SYMBOL_WARNING " #");
#else
    lv_label_set_text(label, LV_SYMBOL_BELL " Hello world Espressif and LVGL " LV_SYMBOL_BELL "\n " LV_SYMBOL_WARNING " For simplier initialization, use BSP " LV_SYMBOL_WARNING);
#endif
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);

    label = lv_label_create(scr);
    lv_snprintf(str, sizeof(str), "Version: V%d.%d.%d ", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    lv_label_set_text(label, str);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);
    /* Button */
    lv_obj_t *btn = lv_btn_create(scr);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, "Rotate screen");
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, _app_button_cb, LV_EVENT_CLICKED, NULL);
}

void app_main(void)
{
    /* LCD HW initialization */
    ESP_ERROR_CHECK(app_lcd_init());

    /* Touch initialization */
    app_touch_init();

    /* LVGL initialization */
    ESP_ERROR_CHECK(app_lvgl_init());

    /* Show LVGL objects */
    /* Task lock */
    lvgl_port_lock(0);
    /* Option 1: Create a simple ui
     * ---------------------
     */
    if (touch_handle != NULL)
    {
        app_main_display();
    }
    else
    {
        lv_demo_benchmark();
    }

    /* Option 2: Or try out a demo.
     * -------------------------------------------------------------------------------------------
     */
    // lv_demo_widgets();
    // lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    // lv_demo_music();
    /* Task unlock */
    lvgl_port_unlock();
}
