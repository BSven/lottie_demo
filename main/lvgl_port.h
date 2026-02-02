/*
 * LVGL Display Port Header
 */

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL display port
 * 
 * @return ESP_OK on success
 */
esp_err_t lvgl_port_init(void);

/**
 * @brief Get LVGL display handle
 * 
 * @return lv_display_t* LVGL display handle
 */
lv_display_t *lvgl_port_get_display(void);

/**
 * @brief Get LVGL input device handle
 * 
 * @return lv_indev_t* LVGL input device handle
 */
lv_indev_t *lvgl_port_get_indev(void);

/**
 * @brief Lock LVGL
 * 
 * @param timeout_ms Timeout in milliseconds
 * @return true if locked successfully
 */
bool lvgl_port_lock(uint32_t timeout_ms);

/**
 * @brief Unlock LVGL
 */
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_PORT_H
