#include <Arduino_GFX_Library.h>

#define GFX_BL 46 // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus* bus = new Arduino_ESP32SPI(45 /* DC */, 21 /* CS */, 38 /* SCK */, 39 /* MOSI */, -1 /* MISO */);
Arduino_GFX* gfx = new Arduino_ST7789(
  bus, 40 /* RST */, 0 /* rotation */, true, 240, 240);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

void setup(void)
{
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX Hello World example");

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif
  
  gfx->fillScreen(RGB565_RED);
  delay(1000);
  gfx->fillScreen(RGB565_BLUE);
  delay(1000);
  gfx->fillScreen(RGB565_GREEN);
  delay(1000);
  gfx->fillScreen(RGB565_BLACK);

  gfx->setCursor(10, 10);
  gfx->setTextColor(RGB565_RED);
  gfx->println("Hello World!");

  delay(5000); // 5 seconds
}

void loop()
{
  gfx->setCursor(random(gfx->width()), random(gfx->height()));
  gfx->setTextColor(random(0xffff), random(0xffff));
  gfx->setTextSize(random(6) /* x scale */, random(6) /* y scale */, random(2) /* pixel_margin */);
  gfx->println("Hello World!");

  delay(1000); // 1 second
}
