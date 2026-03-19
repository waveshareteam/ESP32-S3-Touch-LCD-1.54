/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include "esp_ota_ops.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "bsp/esp-bsp.h"
#include "esp_brookesia.hpp"
#include "boost/thread.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Main"
#include "esp_lib_utils.h"

#include "./dark/stylesheet.hpp"
#include "lv_demos.h"

#include "esp_brookesia_app_squareline_demo.hpp"

#include "driver/gpio.h"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;
using namespace esp_brookesia::apps;

#define TAG   "main"

constexpr bool EXAMPLE_SHOW_MEM_INFO = true;
static SemaphoreHandle_t xSemaphore = NULL;

static void initialize_sntp_and_sync(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config); 

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
        printf("Failed to update system time within 10s timeout\n");
    } else {
        setenv("TZ", "CST-8", 1);
        tzset(); 
        printf("Timezone set to China Standard Time.\n");
    }
}

static void power_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_2);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    /*power on*/
    gpio_set_level(GPIO_NUM_2, 1);
}


extern "C" void app_main(void)
{
    power_init();

    const esp_partition_t *update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    ESP_LOGI(TAG, "Switch to partition factory");
    esp_ota_set_boot_partition(update_partition);
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    xSemaphore = xSemaphoreCreateBinary();
    //bsp_io_expander_init();
    ESP_UTILS_LOGI("Display ESP-Brookesia phone demo");
    lv_display_t *disp = bsp_display_start();
    lv_indev_t *tp = bsp_display_get_input_dev();
    ESP_UTILS_CHECK_ERROR_EXIT(bsp_display_backlight_on(), "Turn on display backlight failed");
    bsp_sdcard_mount();
    // bsp_display_lock(-1);
    // lv_demo_stress();
    // bsp_display_unlock();
    /* Configure GUI lock */
    LvLock::registerCallbacks([](int timeout_ms) {
        esp_err_t ret = bsp_display_lock(timeout_ms);
        ESP_UTILS_CHECK_FALSE_RETURN(ret == ESP_OK, false, "Lock failed (timeout_ms: %d)", timeout_ms);

        return true;
    }, []() {
        bsp_display_unlock();
        return true;
    });

    /* Create a phone object */
    ESP_LOGI(TAG, "Create phone object");
    systems::phone::Phone *phone = new systems::phone::Phone(disp);
    phone->setTouchDevice(tp);
    
    Stylesheet *stylesheet = new systems::phone::Stylesheet(STYLESHEET_240_240_DARK);
    ESP_UTILS_CHECK_NULL_EXIT(stylesheet, "Create stylesheet failed");

    ESP_UTILS_LOGI("Using stylesheet (%s)", stylesheet->core.name);
    ESP_UTILS_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet), "Add stylesheet failed");
    ESP_UTILS_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet), "Activate stylesheet failed");
    delete stylesheet;

    {
        // When operating on non-GUI tasks, should acquire a lock before operating on LVGL
        LvLockGuard gui_guard;

        /* Begin the phone */
        ESP_UTILS_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
        // assert(phone->getDisplay().showContainerBorder() && "Show container border failed");

        /* Init and install apps from registry */
        std::vector<systems::base::Manager::RegistryAppInfo> inited_apps;
        ESP_UTILS_CHECK_FALSE_EXIT(phone->initAppFromRegistry(inited_apps), "Init app registry failed");
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installAppFromRegistry(inited_apps), "Install app registry failed");

        auto app1 = esp_brookesia::apps::SquarelineDemo::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app1),"start Drawpanel failed");

        /* Create a timer to update the clock */
        lv_timer_create([](lv_timer_t *t) {
            time_t now;
            struct tm timeinfo;
            Phone *phone = (Phone *)t->user_data;


            ESP_UTILS_CHECK_NULL_EXIT(phone, "Invalid phone");

            time(&now);
            localtime_r(&now, &timeinfo);

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min),
                "Refresh status bar failed"
            );
            
            wifi_ap_record_t ap_info;
            if(esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
            {
                phone->getDisplay().getStatusBar()->setWifiIconState(2);
            }
            else
            {
                phone->getDisplay().getStatusBar()->setWifiIconState(0);
            }
        }, 1000, phone);
    }

    if constexpr (EXAMPLE_SHOW_MEM_INFO) {
        esp_utils::thread_config_guard thread_config({
            .name = "mem_info",
            .stack_size = 4096,
        });
        boost::thread([ = ]() {
            char buffer[128];    /* Make sure buffer is enough for `sprintf` */
            size_t internal_free = 0;
            size_t internal_total = 0;
            size_t external_free = 0;
            size_t external_total = 0;

            while (1) 
            {
                if (xSemaphoreTake(xSemaphore, 0) == pdTRUE) {
                    printf("Receiver: Semaphore obtained!\n");
                    initialize_sntp_and_sync();
                }

                internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
                external_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                external_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
                sprintf(buffer,
                        "\t           Biggest /     Free /    Total\n"
                        "\t  SRAM : [%8d / %8d / %8d]\n"
                        "\t PSRAM : [%8d / %8d / %8d]",
                        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), internal_free, internal_total,
                        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), external_free, external_total);
                //ESP_UTILS_LOGI("\n%s", buffer);
                
                {
                    LvLockGuard gui_guard;
                    ESP_UTILS_CHECK_FALSE_EXIT(
                        phone->getDisplay().getRecentsScreen()->setMemoryLabel(
                            internal_free / 1024, internal_total / 1024, external_free / 1024, external_total / 1024
                        ), "Set memory label failed"
                    );
                }

                boost::this_thread::sleep_for(boost::chrono::seconds(2));
            }
        }).detach();
    }

}


