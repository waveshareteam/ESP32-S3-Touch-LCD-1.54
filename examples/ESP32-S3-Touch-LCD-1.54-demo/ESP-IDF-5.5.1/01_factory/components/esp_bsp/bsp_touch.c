#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

// #include "esp_lcd_touch_axs5106.h"
#include "esp_lcd_touch_cst816s.h"

#include "esp_log.h"
#include "bsp_touch.h"

static const char *TAG = "bsp_touch";
void bsp_touch_init(esp_lcd_touch_handle_t *touch_handle, i2c_master_bus_handle_t bus_handle, uint16_t xmax, uint16_t ymax, uint16_t rotation, esp_lcd_touch_interrupt_callback_t interrupt_callback)
{
    esp_err_t ret = ESP_OK;
    esp_lcd_panel_io_handle_t touch_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t touch_io_config = {};
    touch_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_CST816S_ADDRESS,
    touch_io_config.control_phase_bytes = 1;
    touch_io_config.dc_bit_offset = 0;
    touch_io_config.lcd_cmd_bits = 8;
    touch_io_config.flags.disable_control_phase = 1;
    touch_io_config.scl_speed_hz = 400 * 1000;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &touch_io_config, &touch_io_handle));

    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max = xmax < ymax ? xmax : ymax;
    tp_cfg.y_max = xmax < ymax ? ymax : xmax;;
    tp_cfg.rst_gpio_num = EXAMPLE_PIN_TP_RST;
    tp_cfg.int_gpio_num = EXAMPLE_PIN_TP_INT;
    tp_cfg.interrupt_callback = interrupt_callback;

    if (90 == rotation)
    {
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 0;
    }
    else if (180 == rotation)
    {
        tp_cfg.flags.swap_xy = 0;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 1;
    }
    else if (270 == rotation)
    {
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 0;
        tp_cfg.flags.mirror_y = 1;
    }
    else
    {
        tp_cfg.flags.swap_xy = 0;
        tp_cfg.flags.mirror_x = 0;
        tp_cfg.flags.mirror_y = 0;
    }

    ret = esp_lcd_touch_new_i2c_cst816s(touch_io_handle, &tp_cfg, touch_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGI(TAG, "Initialize touch failed");
    }
}
