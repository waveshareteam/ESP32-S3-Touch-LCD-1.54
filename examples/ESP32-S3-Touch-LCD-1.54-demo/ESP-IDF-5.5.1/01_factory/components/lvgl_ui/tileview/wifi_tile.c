
#include "wifi_tile.h"

#include "bsp_wifi.h"
#include "esp_lvgl_port.h"

#define LIST_BTN_LEN_MAX 20

static lv_obj_t *list;
lv_obj_t *lable_wifi_ip;

lv_obj_t *list_btns[LIST_BTN_LEN_MAX];
uint16_t list_item_count = 0;
SemaphoreHandle_t wifi_scanf_semaphore;

// typedef struct
// {
//     char *name;
//     char *rssi;
// } wifi_info_t;

// wifi_info_t wifi_infos[4] = {{.name = "wifi1", .rssi = "40db"}, {.name = "wifi2", .rssi = "76db"}, {.name = "wifi3", .rssi = "70db"}, {.name = "wifi4", .rssi = "70db"}};

static void btn_wifi_scan_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        for (int i = 0; i < list_item_count; i++)
        {
            if (list_btns[i])
            {
                lv_obj_del(list_btns[i]);
            }
        }
        list_item_count = 0;
        list_btns[list_item_count++] = lv_list_add_btn(list, NULL, "WiFi scanning underway!");

        xSemaphoreGive(wifi_scanf_semaphore);
    }
}

static void wifi_time_cb(lv_timer_t *timer)
{
    char str[50] = {0};
    char str_wifi_ip[32] = {0};
    bsp_wifi_get_ip(str_wifi_ip);
    sprintf(str, "IP: %s", str_wifi_ip);
    lv_label_set_text(lable_wifi_ip, str);
}

static void lvgl_wifi_task(void *arg)
{
    lv_obj_t *label;
    wifi_ap_record_t ap_info[LIST_BTN_LEN_MAX];
    while (1)
    {
        // vTaskDelay(pd);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        if (lvgl_port_lock(0))
        {
            for (int i = 0; i < list_item_count; i++)
            {
                if (list_btns[i])
                {
                    lv_obj_del(list_btns[i]);
                }
            }
            list_item_count = 0;
            list_btns[list_item_count++] = lv_list_add_btn(list, NULL, "WiFi scanning underway!");
            lvgl_port_unlock();
        }

        // xSemaphoreTake(wifi_scanf_semaphore, portMAX_DELAY);
        printf("wifi_scanf!!\r\n");
        memset(ap_info, 0, sizeof(ap_info));
        if (bsp_wifi_scan(ap_info, &list_item_count, LIST_BTN_LEN_MAX))
        {

            if (lvgl_port_lock(0))
            {
                lv_obj_del(list_btns[0]);
                for (int i = 0; i < list_item_count && i < LIST_BTN_LEN_MAX; i++)
                {
                    list_btns[i] = lv_list_add_btn(list, NULL, (char *)ap_info[i].ssid);
                    label = lv_label_create(list_btns[i]);
                    lv_label_set_text_fmt(label, "%d db", ap_info[i].rssi);
                }
                lvgl_port_unlock();
            }
        }
        else
        {
            if (lvgl_port_lock(0))
            {
                if (list_btns[0] != NULL)
                {
                    lv_obj_del(list_btns[0]);
                }
                lvgl_port_unlock();
            }
        }
    }
}

void wifi_tile_init(lv_obj_t *parent)
{
    /*Create a list*/
    list = lv_list_create(parent);

    lv_obj_t *lable = lv_label_create(parent);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(lable, "WiFi");
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 3);

    lable_wifi_ip = lv_label_create(parent);
    lv_label_set_text(lable_wifi_ip, "IP: 0.0.0.0");
    lv_obj_align(lable_wifi_ip, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_set_size(list, lv_pct(100), lv_pct(85));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);

    // lv_obj_t *btn = lv_btn_create(parent);
    // lv_obj_set_size(btn, lv_pct(80), 40);
    // lable = lv_label_create(btn);
    // lv_label_set_text(lable, "scan");
    // lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
    // lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    // lv_obj_add_event_cb(btn, btn_wifi_scan_event_handler, LV_EVENT_CLICKED, NULL);

    // wifi_scanf_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(lvgl_wifi_task, "lvgl_wifi_task", 1024 * 10, NULL, 0, NULL);

    lv_timer_create(wifi_time_cb, 1000, NULL);
}