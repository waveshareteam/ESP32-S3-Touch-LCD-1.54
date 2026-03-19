#ifndef __BSP_POWER_MANAGER_H__
#define __BSP_POWER_MANAGER_H__ 

#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

void bsp_power_manager_init(void);
int bsp_get_battery_level(void);
float bsp_get_battery_voltage(void);
bool bsp_is_charging(void);
void bsp_power_off(void);
void bsp_power_on(void);

#ifdef __cplusplus
} // extern "C"
#endif


#endif