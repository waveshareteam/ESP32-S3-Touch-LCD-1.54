/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>

/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 as the examples and demos are now part of the main LVGL library. */

// #include <examples/lv_examples.h>
// #include <demos/lv_demos.h>

// #define DIRECT_MODE // Uncomment to enable full frame buffer

#include <Arduino_GFX_Library.h>

#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>

#include "TouchDrvCSTXXX.hpp"

#ifndef SENSOR_SDA
#define SENSOR_SDA 42
#endif

#ifndef SENSOR_SCL
#define SENSOR_SCL 41
#endif

#ifndef SENSOR_IRQ
#define SENSOR_IRQ 48
#endif

#ifndef SENSOR_RST
#define SENSOR_RST 47
#endif

TouchDrvCSTXXX touch;

bool isPressed = false;
bool isTouch = false;

#define GFX_BL 46  // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
Arduino_DataBus *bus = new Arduino_ESP32SPI(45 /* DC */, 21 /* CS */, 38 /* SCK */, 39 /* MOSI */, -1 /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 40 /* RST */, 0 /* rotation */, true, 240, 240);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

/*******************************************************************************
 * Please config the touch panel in touch.h
 ******************************************************************************/

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
  lv_disp_flush_ready(disp_drv);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  int16_t x[5], y[5];
  if (isPressed) {
    isPressed = false;
    uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());
    if (touched) {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = x[0];
      data->point.y = y[0];
    } else {
      data->state = LV_INDEV_STATE_REL;
    }

  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}


void setup() {
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX LVGL_Arduino_v8 example ");
  String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  touch.setPins(SENSOR_RST, SENSOR_IRQ);
  Wire.begin(SENSOR_SDA, SENSOR_SCL);
  isTouch = touch.begin(Wire, CST816_SLAVE_ADDRESS, SENSOR_SDA, SENSOR_SCL);
  if (isTouch == true) {
    attachInterrupt(
      SENSOR_IRQ, []() {
        isPressed = true;
      },
      FALLING);
  }



  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef DIRECT_MODE
  bufSize = screenWidth * screenHeight;
#else
  bufSize = screenWidth * 40;
#endif

#if defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
  disp_draw_buf = (lv_color_t *)gfx->getFramebuffer();
#else   // !(defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)))
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
#endif  // !(defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)))

  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
#ifdef DIRECT_MODE
    disp_drv.direct_mode = true;
#endif
    lv_disp_drv_register(&disp_drv);

    /* Initialize the (dummy) input device driver */
    if (isTouch == true) {
      static lv_indev_drv_t indev_drv;
      lv_indev_drv_init(&indev_drv);
      indev_drv.type = LV_INDEV_TYPE_POINTER;
      indev_drv.read_cb = my_touchpad_read;
      lv_indev_drv_register(&indev_drv);
    }
    /* Option 1: Create simple label */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello Arduino! (V" GFX_STR(LVGL_VERSION_MAJOR) "." GFX_STR(LVGL_VERSION_MINOR) "." GFX_STR(LVGL_VERSION_PATCH) ")");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    if (isTouch == true) {
      lv_obj_t *sw = lv_switch_create(lv_scr_act());
      lv_obj_set_size(sw, 80, 40);
      lv_obj_align(sw, LV_ALIGN_TOP_MID, 0, 30);

      sw = lv_switch_create(lv_scr_act());
      lv_obj_set_size(sw, 80, 40);
      lv_obj_align(sw, LV_ALIGN_BOTTOM_MID, 0, -30);
    }
    /* Option 2: Try an example. See all the examples
     * online: https://docs.lvgl.io/master/examples.html
     * source codes: https://github.com/lvgl/lvgl/tree/e7f88efa5853128bf871dde335c0ca8da9eb7731/examples */
    // lv_example_btn_1();

    /* Option 3: Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMOS_WIDGETS*/
    // lv_demo_widgets();
    // lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    // lv_demo_music();
    // lv_demo_stress();
  }

  Serial.println("Setup done");
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
