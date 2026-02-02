/**
 * LVGL Configuration for ESP32-P4
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Color depth: 16 bit for RGB565 */
#define LV_COLOR_DEPTH 16

/* Memory settings */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (128 * 1024U)

/* Display refresh period */
#define LV_DEF_REFR_PERIOD 33

/* Enable features */
#define LV_USE_PERF_MONITOR 1
#define LV_USE_MEM_MONITOR 1
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO

/* Font support */
#define LV_FONT_MONTSERRAT_8 1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_38 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_42 1
#define LV_FONT_MONTSERRAT_44 1
#define LV_FONT_MONTSERRAT_46 1
#define LV_FONT_MONTSERRAT_48 1

/* Enable all widgets */
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_ARC 1
#define LV_USE_ANIMIMG 1
#define LV_USE_CANVAS 1

/* Vector graphics and Lottie support */
#define LV_USE_VECTOR_GRAPHIC 1
#define LV_USE_THORVG_INTERNAL 1
#define LV_USE_THORVG 1

/* Lottie animation support */
#define LV_USE_LOTTIE 1

#endif /* LV_CONF_H */
