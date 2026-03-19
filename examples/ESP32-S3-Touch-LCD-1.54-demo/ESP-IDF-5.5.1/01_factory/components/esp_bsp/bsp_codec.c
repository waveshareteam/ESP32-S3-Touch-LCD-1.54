#include "esp_idf_version.h"

#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"

#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include "driver/i2c_master.h"
#include <driver/i2c_master.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_heap_caps.h"
#include "bsp_i2c.h"

#include <esp_log.h>

#define USE_IDF_I2C_MASTER

#define I2S_MCK_PIN 8
#define I2S_BCK_PIN 9
#define I2S_LRCK_PIN 10
#define I2S_DOUT_PIN 12
#define I2S_DIN_PIN 11
#define PA_CTRL_PIN 7

#define INPUT_SAMPLE_RATE 16000
#define OUTPUT_SAMPLE_RATE 16000

#define TAG "bsp_codec"

i2s_chan_handle_t tx_handle;
i2s_chan_handle_t rx_handle;
esp_codec_dev_handle_t output_dev;
esp_codec_dev_handle_t input_dev;
const audio_codec_data_if_t *data_if_;
const audio_codec_data_if_t *data_if_ = NULL;
const audio_codec_ctrl_if_t *out_ctrl_if_ = NULL;
const audio_codec_if_t *out_codec_if_ = NULL;
const audio_codec_ctrl_if_t *in_ctrl_if_ = NULL;
const audio_codec_if_t *in_codec_if_ = NULL;
const audio_codec_gpio_if_t *gpio_if_ = NULL;

static void es8311_i2s_init(void)
{
    // esp_err_t esp_err;
    // i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    // i2s_std_config_t std_cfg = {};
    // std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    // std_cfg.clk_cfg.sample_rate_hz = 16000;
    // std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;

    // std_cfg.slot_cfg.data_bit_width = 16;
    // std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
    // std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
    // std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    // std_cfg.slot_cfg.ws_width = 16;
    // std_cfg.slot_cfg.ws_pol = false;
    // std_cfg.slot_cfg.bit_shift = true;
    // std_cfg.slot_cfg.left_align = true;
    // std_cfg.slot_cfg.big_endian = false;
    // std_cfg.slot_cfg.bit_order_lsb = false;

    // esp_err = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);

    // esp_err = i2s_channel_init_std_mode(tx_handle, &std_cfg);

    // esp_err = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    // // For tx master using duplex mode
    // i2s_channel_enable(tx_handle);
    // i2s_channel_enable(rx_handle);

    // return esp_err;

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 240,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)OUTPUT_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256},
        .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT, .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, .slot_mode = I2S_SLOT_MODE_STEREO, .slot_mask = I2S_STD_SLOT_BOTH, .ws_width = I2S_DATA_BIT_WIDTH_16BIT, .ws_pol = false, .bit_shift = true, .left_align = true, .big_endian = false, .bit_order_lsb = false},
        .gpio_cfg = {.mclk = (gpio_num_t)I2S_MCK_PIN, .bclk = (gpio_num_t)I2S_BCK_PIN, .ws = (gpio_num_t)I2S_LRCK_PIN, .dout = (gpio_num_t)I2S_DOUT_PIN, .din = I2S_GPIO_UNUSED, .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};

    i2s_tdm_config_t tdm_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)INPUT_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            .bclk_div = 8,
        },
        .slot_cfg = {.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT, .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, .slot_mode = I2S_SLOT_MODE_STEREO, .slot_mask = I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3, .ws_width = I2S_TDM_AUTO_WS_WIDTH, .ws_pol = false, .bit_shift = true, .left_align = false, .big_endian = false, .bit_order_lsb = false, .skip_mask = false, .total_slot = I2S_TDM_AUTO_SLOT_NUM},
        .gpio_cfg = {.mclk = (gpio_num_t)I2S_MCK_PIN, .bclk = (gpio_num_t)I2S_BCK_PIN, .ws = (gpio_num_t)I2S_LRCK_PIN, .dout = I2S_GPIO_UNUSED, .din = (gpio_num_t)I2S_DIN_PIN, .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(rx_handle, &tdm_cfg));
    ESP_LOGI(TAG, "Duplex channels created");
}

void bsp_codec_init(i2c_master_bus_handle_t bus_handle)
{
    es8311_i2s_init();
    // audio_codec_i2s_cfg_t i2s_cfg = {
    //     .rx_handle = rx_handle,
    //     .tx_handle = tx_handle,
    // };
    // const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    // static audio_codec_i2c_cfg_t i2c_cfg = {};
    // i2c_cfg.addr = ES8311_CODEC_DEFAULT_ADDR;
    // i2c_cfg.bus_handle = bus_handle;

    // const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    // i2c_cfg.addr = ES7210_CODEC_DEFAULT_ADDR;
    // const audio_codec_ctrl_if_t *in_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    // const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    // // New output codec interface
    // es8311_codec_cfg_t es8311_cfg = {};
    // es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    // es8311_cfg.ctrl_if = out_ctrl_if;
    // es8311_cfg.gpio_if = gpio_if;
    // es8311_cfg.pa_pin = PA_CTRL_PIN;
    // es8311_cfg.use_mclk = true;
    // es8311_cfg.hw_gain.pa_voltage = 5.0;
    // es8311_cfg.hw_gain.codec_dac_voltage = 3.3;

    // const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    // es7210_codec_cfg_t es7210_cfg = {
    //     .ctrl_if = in_ctrl_if,
    //     .mic_selected = ES7120_SEL_MIC1 | ES7120_SEL_MIC2 | ES7120_SEL_MIC3,
    // };
    // const audio_codec_if_t *in_codec_if = es7210_codec_new(&es7210_cfg);

    // esp_codec_dev_cfg_t dev_cfg = {
    //     .dev_type = ESP_CODEC_DEV_TYPE_OUT,
    //     .codec_if = out_codec_if,
    //     .data_if = data_if,
    // };
    // output_dev = esp_codec_dev_new(&dev_cfg);
    // assert(output_dev != NULL);

    // dev_cfg.codec_if = in_codec_if;
    // dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    // input_dev = esp_codec_dev_new(&dev_cfg);
    // assert(input_dev != NULL);

    // esp_codec_set_disable_when_closed(output_dev, false);
    // esp_codec_set_disable_when_closed(input_dev, false);

    // esp_codec_dev_sample_info_t fs = {
    //     .sample_rate = 16000,
    //     .channel = 2,
    //     .bits_per_sample = 16,
    // };
    // esp_codec_dev_open(output_dev, &fs);
    // esp_codec_dev_open(input_dev, &fs);

    // Do initialize of related interface: data_if, ctrl_if and gpio_if
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
    };
    data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
    assert(data_if_ != NULL);

    // Output
    audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = bus_handle,
    };
    out_ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(out_ctrl_if_ != NULL);

    gpio_if_ = audio_codec_new_gpio();
    assert(gpio_if_ != NULL);

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = out_ctrl_if_;
    es8311_cfg.gpio_if = gpio_if_;
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    es8311_cfg.pa_pin = PA_CTRL_PIN;
    es8311_cfg.use_mclk = true;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    out_codec_if_ = es8311_codec_new(&es8311_cfg);
    assert(out_codec_if_ != NULL);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = out_codec_if_,
        .data_if = data_if_,
    };
    output_dev = esp_codec_dev_new(&dev_cfg);
    assert(output_dev != NULL);

    // Input
    i2c_cfg.addr = ES7210_CODEC_DEFAULT_ADDR;
    in_ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(in_ctrl_if_ != NULL);

    es7210_codec_cfg_t es7210_cfg = {};
    es7210_cfg.ctrl_if = in_ctrl_if_;
    es7210_cfg.mic_selected = ES7120_SEL_MIC1 | ES7120_SEL_MIC2 | ES7120_SEL_MIC3 | ES7120_SEL_MIC4;
    in_codec_if_ = es7210_codec_new(&es7210_cfg);
    assert(in_codec_if_ != NULL);

    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    dev_cfg.codec_if = in_codec_if_;
    input_dev = esp_codec_dev_new(&dev_cfg);
    assert(input_dev != NULL);

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .sample_rate = (uint32_t)OUTPUT_SAMPLE_RATE,
        .mclk_multiple = 0,
    };
    ESP_ERROR_CHECK(esp_codec_dev_open(output_dev, &fs));
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev, 0));


    fs.bits_per_sample = 16;
    fs.channel = 4;
    fs.channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0);
    fs.sample_rate = (uint32_t)INPUT_SAMPLE_RATE;
    fs.mclk_multiple = 0;

    // fs.channel_mask |= ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1);

    ESP_ERROR_CHECK(esp_codec_dev_open(input_dev, &fs));
    ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(input_dev, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0), 30));

    ESP_LOGI(TAG, "BoxAudioDevice initialized");
}

void bsp_codec_set_out_volume(float volume)
{
    esp_codec_dev_set_out_vol(output_dev, volume);
}

void bsp_codec_set_in_volume(float volume)
{
    esp_codec_dev_set_in_gain(input_dev, volume);
}

void bsp_codec_recording(uint8_t *data, size_t limit_size)
{
    int err = 0;
    err = esp_codec_dev_read(input_dev, data, limit_size);
    if (err == ESP_CODEC_DEV_OK)
        printf("Read %d bytes\n", limit_size);
    else
        printf("Read error %d\n", err);
}

void bsp_codec_playing(uint8_t *data, size_t limit_size)
{
    int err = 0;

    err = esp_codec_dev_write(output_dev, data, limit_size);

    if (err == ESP_CODEC_DEV_OK)
        printf("Write %d bytes\n", limit_size);
    else
        printf("Write error %d\n", err);
}

void bsp_codec_test(void)
{
    int err = 0;
    // 2 Sec
    const int limit_size = 3 * 16000 * 1 * (16 >> 3);

    uint8_t *data = (uint8_t *)heap_caps_malloc(limit_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    esp_codec_dev_set_in_gain(input_dev, 40.0);

    err = esp_codec_dev_read(input_dev, data, limit_size);

    esp_codec_dev_set_in_gain(input_dev, 0.0);

    if (err == ESP_CODEC_DEV_OK)
        printf("Read %d bytes\n", limit_size);
    else
        printf("Read error %d\n", err);

    esp_codec_dev_set_out_vol(output_dev, 70.0);

    err = esp_codec_dev_write(output_dev, data, limit_size);

    esp_codec_dev_set_out_vol(output_dev, 0.0);

    if (err == ESP_CODEC_DEV_OK)
        printf("Write %d bytes\n", limit_size);
    else
        printf("Write error %d\n", err);

    heap_caps_free(data);
}