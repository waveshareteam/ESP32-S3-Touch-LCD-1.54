#ifndef __BSP_CODEC_H__
#define __BSP_CODEC_H__ 


#include "esp_codec_dev.h"
#include "audio_codec_ctrl_if.h"
#include "audio_codec_gpio_if.h"

#ifdef __cplusplus
extern "C" {
#endif
void bsp_codec_init(i2c_master_bus_handle_t bus_handle);


void bsp_codec_set_out_volume(float volume);
void bsp_codec_set_in_volume(float volume);

void bsp_codec_recording(uint8_t *data, size_t limit_size);
void bsp_codec_playing(uint8_t *data, size_t limit_size);

void bsp_codec_test(void);

#ifdef __cplusplus
}
#endif

#endif