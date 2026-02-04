/*
 * LVGL Display Port Header
 * Using ESP-BSP esp_lvgl_port for flicker-free display
 */

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "esp_err.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"  /* Provides lvgl_port_lock/unlock */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL display and touch
 * 
 * @return ESP_OK on success
 */
esp_err_t lvgl_display_init(void);

/**
 * @brief Get LVGL display handle
 * 
 * @return lv_display_t* LVGL display handle
 */
lv_display_t *lvgl_display_get_display(void);

/**
 * @brief Get LVGL input device handle
 * 
 * @return lv_indev_t* LVGL input device handle
 */
lv_indev_t *lvgl_display_get_indev(void);

/* Note: lvgl_port_lock() and lvgl_port_unlock() are provided by esp_lvgl_port.h */

#ifdef __cplusplus
}
#endif

#endif // LVGL_PORT_H
