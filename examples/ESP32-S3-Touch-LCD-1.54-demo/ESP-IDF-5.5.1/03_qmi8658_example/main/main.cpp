/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"

#include "SensorQMI8658.hpp"

#define I2C_PORT_NUM 0
#define EXAMPLE_PIN_I2C_SDA GPIO_NUM_42
#define EXAMPLE_PIN_I2C_SCL GPIO_NUM_41

SensorQMI8658 qmi;
i2c_master_bus_handle_t i2c_bus_handle;
extern "C" void app_main(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = (i2c_port_num_t)I2C_PORT_NUM;
    i2c_mst_config.scl_io_num = EXAMPLE_PIN_I2C_SCL;
    i2c_mst_config.sda_io_num = EXAMPLE_PIN_I2C_SDA;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = 1;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));

    while (!qmi.begin(i2c_bus_handle, QMI8658_L_SLAVE_ADDRESS))
    {
        printf("Failed to find QMI8658 - check your wiring!\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    /* Get chip id*/
    printf("Device ID:");
    printf(" %x\r\n", qmi.getChipID());

    if (qmi.selfTestAccel())
    {
        printf("Accelerometer self-test successful!\r\n");
    }
    else
    {
        printf("Accelerometer self-test failed!\r\n");
    }

    if (qmi.selfTestGyro())
    {
        printf("Gyroscope self-test successful!\r\n");
    }
    else
    {
        printf("Gyroscope self-test failed!\r\n");
    }

    qmi.configAccelerometer(
        /*
         * ACC_RANGE_2G
         * ACC_RANGE_4G
         * ACC_RANGE_8G
         * ACC_RANGE_16G
         * */
        SensorQMI8658::ACC_RANGE_4G,
        /*
         * ACC_ODR_1000H
         * ACC_ODR_500Hz
         * ACC_ODR_250Hz
         * ACC_ODR_125Hz
         * ACC_ODR_62_5Hz
         * ACC_ODR_31_25Hz
         * ACC_ODR_LOWPOWER_128Hz
         * ACC_ODR_LOWPOWER_21Hz
         * ACC_ODR_LOWPOWER_11Hz
         * ACC_ODR_LOWPOWER_3H
         * */
        SensorQMI8658::ACC_ODR_1000Hz,
        /*
         *  LPF_MODE_0     //2.66% of ODR
         *  LPF_MODE_1     //3.63% of ODR
         *  LPF_MODE_2     //5.39% of ODR
         *  LPF_MODE_3     //13.37% of ODR
         *  LPF_OFF        // OFF Low-Pass Fitter
         * */
        SensorQMI8658::LPF_MODE_0);

    qmi.configGyroscope(
        /*
         * GYR_RANGE_16DPS
         * GYR_RANGE_32DPS
         * GYR_RANGE_64DPS
         * GYR_RANGE_128DPS
         * GYR_RANGE_256DPS
         * GYR_RANGE_512DPS
         * GYR_RANGE_1024DPS
         * */
        SensorQMI8658::GYR_RANGE_64DPS,
        /*
         * GYR_ODR_7174_4Hz
         * GYR_ODR_3587_2Hz
         * GYR_ODR_1793_6Hz
         * GYR_ODR_896_8Hz
         * GYR_ODR_448_4Hz
         * GYR_ODR_224_2Hz
         * GYR_ODR_112_1Hz
         * GYR_ODR_56_05Hz
         * GYR_ODR_28_025H
         * */
        SensorQMI8658::GYR_ODR_896_8Hz,
        /*
         *  LPF_MODE_0     //2.66% of ODR
         *  LPF_MODE_1     //3.63% of ODR
         *  LPF_MODE_2     //5.39% of ODR
         *  LPF_MODE_3     //13.37% of ODR
         *  LPF_OFF        // OFF Low-Pass Fitter
         * */
        SensorQMI8658::LPF_MODE_3);

    /*
     * If both the accelerometer and gyroscope sensors are turned on at the same time,
     * the output frequency will be based on the gyroscope output frequency.
     * The example configuration is 896.8HZ output frequency,
     * so the acceleration output frequency is also limited to 896.8HZ
     * */
    qmi.enableGyroscope();
    qmi.enableAccelerometer();

    // Print register configuration information
    qmi.dumpCtrlRegister();

    IMUdata acc;
    IMUdata gyr;
    while (1)
    {
        if (qmi.getDataReady())
        {
            if (qmi.getAccelerometer(acc.x, acc.y, acc.z))
            {
                printf("ACCEL--x:%5.2f y:%5.2f z:%5.2f  ", acc.x, acc.y, acc.z);
            }

            if (qmi.getGyroscope(gyr.x, gyr.y, gyr.z))
            {
                printf("GYRO--x:%5.2f y:%5.2f z:%5.2f  ", gyr.x, gyr.y, gyr.z);
            }
            printf("Temperature:%5.2f degrees C", qmi.getTemperature_C());
        }
        printf("Timestamp:%ld \r\n", qmi.getTimestamp());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
