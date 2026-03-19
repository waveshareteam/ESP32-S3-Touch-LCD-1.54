#include "qmi8658_tile.h"
#include "bsp_qmi8658.h"

static lv_obj_t *list;
lv_obj_t *label_accel_x;
lv_obj_t *label_accel_y;
lv_obj_t *label_accel_z;

lv_obj_t *label_gyro_x;
lv_obj_t *label_gyro_y;
lv_obj_t *label_gyro_z;

lv_obj_t *label_angle_x;
lv_obj_t *label_angle_y;
lv_obj_t *label_angle_z;

static void qmi8658_time_cb(lv_timer_t *timer)
{
    qmi8658_data_t qmi8658_data;
    char str[20];
    if (bsp_qmi8658_read_data(&qmi8658_data))
    {
        lv_label_set_text_fmt(label_accel_x, "%d", qmi8658_data.acc_x);
        lv_label_set_text_fmt(label_accel_y, "%d", qmi8658_data.acc_y);
        lv_label_set_text_fmt(label_accel_z, "%d", qmi8658_data.acc_z);
        lv_label_set_text_fmt(label_gyro_x, "%d", qmi8658_data.gyr_x);
        lv_label_set_text_fmt(label_gyro_y, "%d", qmi8658_data.gyr_y);
        lv_label_set_text_fmt(label_gyro_z, "%d", qmi8658_data.gyr_z);
        
        sprintf(str, "%4.1f", qmi8658_data.AngleX);
        lv_label_set_text_fmt(label_angle_x, str);
        sprintf(str, "%4.1f", qmi8658_data.AngleY);
        lv_label_set_text_fmt(label_angle_y, str);
        sprintf(str, "%4.1f", qmi8658_data.AngleZ);
        lv_label_set_text_fmt(label_angle_z, str);
    }
}

void qmi8658_tile_init(lv_obj_t *parent)
{
    /*Create a list*/
    list = lv_list_create(parent);
    lv_obj_t *lable =  lv_label_create(parent);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(lable, "QMI8658");
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 3);

    lv_obj_set_size(list, lv_pct(100), lv_pct(85));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *list_item;

    list_item = lv_list_add_btn(list, NULL, "Accel_x");
    label_accel_x = lv_label_create(list_item);
    lv_label_set_text(label_accel_x, "----");

    list_item = lv_list_add_btn(list, NULL, "Accel_y");
    label_accel_y = lv_label_create(list_item);
    lv_label_set_text(label_accel_y, "----");

    list_item = lv_list_add_btn(list, NULL, "Accel_z");
    label_accel_z = lv_label_create(list_item);
    lv_label_set_text(label_accel_z, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro_x");
    label_gyro_x = lv_label_create(list_item);
    lv_label_set_text(label_gyro_x, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro_y");
    label_gyro_y = lv_label_create(list_item);
    lv_label_set_text(label_gyro_y, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro_z");
    label_gyro_z = lv_label_create(list_item);
    lv_label_set_text(label_gyro_z, "----");

    list_item = lv_list_add_btn(list, NULL, "Angle_x");
    label_angle_x = lv_label_create(list_item);
    lv_label_set_text(label_angle_x, "----");

    list_item = lv_list_add_btn(list, NULL, "Angle_y");
    label_angle_y = lv_label_create(list_item);
    lv_label_set_text(label_angle_y, "----");

    list_item = lv_list_add_btn(list, NULL, "Angle_z");
    label_angle_z = lv_label_create(list_item);
    lv_label_set_text(label_angle_z, "----");

    lv_timer_create(qmi8658_time_cb, 100, NULL);
}