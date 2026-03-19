#include "bsp_power_manager.h"

#include <esp_log.h>

#include <driver/gpio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <math.h>


#define TAG "bsp_power_manager"

gpio_num_t charging_pin_ = GPIO_NUM_3;
gpio_num_t bat_adc_pin_ = GPIO_NUM_1;
gpio_num_t bat_power_pin_ = GPIO_NUM_2;
adc_oneshot_unit_handle_t adc_handle_ = NULL;
adc_cali_handle_t adc_cali_handle_ = NULL;
adc_channel_t adc_channel_;
bool do_calibration = false;

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }
    return calibrated;
}

void bsp_power_manager_init(void)
{
    // 初始化充电引脚
    if (charging_pin_ != GPIO_NUM_NC)
    {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = 1ULL << charging_pin_;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io_conf);
    }
    // 初始化电池使能引脚
    if (bat_power_pin_ != GPIO_NUM_NC)
    {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << bat_power_pin_;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
        gpio_set_level(bat_power_pin_, 1);
    }

    // 初始化adc
    if (bat_adc_pin_ != GPIO_NUM_NC)
    {
        adc_oneshot_unit_init_cfg_t init_config = {};
        init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
        if (bat_adc_pin_ >= GPIO_NUM_1 && bat_adc_pin_ <= GPIO_NUM_10)
        {
            init_config.unit_id = ADC_UNIT_1;
            adc_channel_ = (adc_channel_t)((int)bat_adc_pin_ - 1);
        }
        else if (bat_adc_pin_ >= GPIO_NUM_11 && bat_adc_pin_ <= GPIO_NUM_20)
        {
            init_config.unit_id = ADC_UNIT_2;
            adc_channel_ = (adc_channel_t)((int)bat_adc_pin_ - 11);
        }
        else
        {
            return;
        }
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle_));
        adc_oneshot_chan_cfg_t config = {};
        config.bitwidth = ADC_BITWIDTH_DEFAULT;
        config.atten = ADC_ATTEN_DB_12;
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, adc_channel_, &config));
        do_calibration = adc_calibration_init(init_config.unit_id, adc_channel_, config.atten, &adc_cali_handle_);
    }
}

float bsp_get_battery_voltage(void)
{
    int adc_raw = 0;
    int voltage_int = 0;
    const float voltage_float_threshold = 0.1f;
    float voltage_float = 0.0f;
    static float last_voltage_float = 0.0f;

    static int last_battery_level = 0;

    if (adc_handle_ != NULL)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_, adc_channel_, &adc_raw));
        if (do_calibration)
        {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle_, adc_raw, &voltage_int));
            voltage_float = (voltage_int / 1000.0f) * 3.0;
            return voltage_float;
        }
    }
    return 4.2;
}

int bsp_get_battery_level(void)
{
    const float voltage_float_threshold = 0.1f;
    float voltage_float = 0.0f;
    static float last_voltage_float = 0.0f;

    static int last_battery_level = 0;

    voltage_float = bsp_get_battery_voltage();

    if (fabs(voltage_float - last_voltage_float) >= voltage_float_threshold)
    {
        last_voltage_float = voltage_float;
        if (voltage_float < 3.52)
        {
            last_battery_level = 1;
        }
        else if (voltage_float < 3.64)
        {
            last_battery_level = 20;
        }
        else if (voltage_float < 3.76)
        {
            last_battery_level = 40;
        }
        else if (voltage_float < 3.88)
        {
            last_battery_level = 60;
        }
        else if (voltage_float < 4.0)
        {
            last_battery_level = 80;
        }
        else
        {
            last_battery_level = 100;
        }
    }
    return last_battery_level;
}

bool bsp_is_charging(void)
{
    if (bsp_get_battery_level() == 100)
    {
        return false;
    }
    if (charging_pin_ != GPIO_NUM_NC)
    {
        return gpio_get_level(charging_pin_) == 0 ? true : false;
    }
    return false;
}

void bsp_power_off(void)
{
    if (bat_power_pin_ != GPIO_NUM_NC)
    {
        gpio_set_level(bat_power_pin_, 0);
    }
}
void bsp_power_on(void)
{
    if (bat_power_pin_ != GPIO_NUM_NC)
    {
        gpio_set_level(bat_power_pin_, 1);
    }
}