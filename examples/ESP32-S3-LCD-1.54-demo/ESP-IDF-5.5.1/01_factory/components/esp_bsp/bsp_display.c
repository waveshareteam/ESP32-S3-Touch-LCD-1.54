#include "bsp_display.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"


#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"


// #include "bsp_spi.h"

#include "esp_log.h"

static const char *TAG = "bsp_display";

static uint8_t g_brightness = 0;


void bsp_display_init(esp_lcd_panel_io_handle_t *io_handle, esp_lcd_panel_handle_t *panel_handle, size_t max_transfer_sz)
{
    ESP_LOGI(TAG, "SPI BUS init");
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_SCLK,
        .mosi_io_num = EXAMPLE_PIN_MOSI,
        .miso_io_num = EXAMPLE_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_transfer_sz,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(EXAMPLE_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));


    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = EXAMPLE_PIN_LCD_CS;
    io_config.dc_gpio_num = EXAMPLE_PIN_LCD_DC;
    io_config.spi_mode = 3;
    io_config.pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_SPI_HOST, &io_config, io_handle));
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    esp_lcd_new_panel_st7789(*io_handle, &panel_config, panel_handle);

    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel_handle, true));
}


void bsp_display_brightness_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LCD_BL_LEDC_MODE,
    ledc_timer.timer_num = LCD_BL_LEDC_TIMER;
    ledc_timer.duty_resolution = LCD_BL_LEDC_DUTY_RES;
    ledc_timer.freq_hz = LCD_BL_LEDC_FREQUENCY; // Set output frequency at 5 kHz
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.speed_mode = LCD_BL_LEDC_MODE;
    ledc_channel.channel = LCD_BL_LEDC_CHANNEL;
    ledc_channel.timer_sel = LCD_BL_LEDC_TIMER;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num = EXAMPLE_PIN_LCD_BL;
    ledc_channel.duty = 0, // Set duty to 0%
    ledc_channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void bsp_display_set_brightness(uint8_t brightness)
{
    if (brightness > 100)
    {
        brightness = 100;
        ESP_LOGE(TAG, "Brightness value out of range");
    }

    g_brightness = brightness;
    uint32_t duty = (brightness * (LCD_BL_LEDC_DUTY - 1)) / 100;

    ESP_ERROR_CHECK(ledc_set_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL));

    ESP_LOGI(TAG, "LCD brightness set to %d%%", brightness);
}

uint8_t bsp_display_get_brightness(void)
{
    return g_brightness;
}