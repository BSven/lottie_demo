/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "lvgl__lvgl/src/widgets/lottie/lv_lottie.h"

// External Lottie data
extern const uint8_t circle_lottie_data[];
extern const uint32_t circle_lottie_data_size;
extern const uint8_t cute_bird_lottie_data[];
extern const uint32_t cute_bird_lottie_data_size;

#if CONFIG_LV_USE_SYSMON
#include "lvgl__lvgl/src/others/sysmon/lv_sysmon.h"
#endif

static const char *TAG = "main";

// Lottie buffer dimensions - change here to adjust size
static const size_t LOTTIE_BUFFER_DIM = 300;

// Global state for animation switching
static lv_obj_t *g_lottie_obj = NULL;
static void *g_lottie_buf = NULL;
static bool g_is_cute_bird = false;  // false = circle, true = cute_bird

// Touch event callback to switch animations
static void touch_event_cb(lv_event_t *e)
{
    if (g_lottie_obj == NULL || g_lottie_buf == NULL) {
        ESP_LOGE(TAG, "Lottie object or buffer not initialized");
        return;
    }
    
    // Delete old Lottie object
    lv_obj_delete(g_lottie_obj);
    
    // Toggle animation
    g_is_cute_bird = !g_is_cute_bird;
    
    // Create new Lottie object
    lv_obj_t *scr = lv_screen_active();
    g_lottie_obj = lv_lottie_create(scr);
    lv_obj_set_size(g_lottie_obj, LOTTIE_BUFFER_DIM, LOTTIE_BUFFER_DIM);
    lv_obj_center(g_lottie_obj);
    
    // Load animation data
    if (g_is_cute_bird) {
        ESP_LOGI(TAG, "Switching to cute_bird animation");
        lv_lottie_set_buffer(g_lottie_obj, LOTTIE_BUFFER_DIM, LOTTIE_BUFFER_DIM, g_lottie_buf);
        lv_lottie_set_src_data(g_lottie_obj, (const char*)cute_bird_lottie_data, cute_bird_lottie_data_size);
    } else {
        ESP_LOGI(TAG, "Switching to circle animation");
        lv_lottie_set_buffer(g_lottie_obj, LOTTIE_BUFFER_DIM, LOTTIE_BUFFER_DIM, g_lottie_buf);
        lv_lottie_set_src_data(g_lottie_obj, (const char*)circle_lottie_data, circle_lottie_data_size);
    }
}

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    // Initialize LVGL
    ESP_LOGI(TAG, "Initializing LVGL...");
    ESP_ERROR_CHECK(lvgl_port_init());
    
    ESP_LOGI(TAG, "System ready");
    
    // Give LVGL time to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Create Lottie animation demo
    ESP_LOGI(TAG, "Creating Lottie animation demo");
    
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return;
    }
    
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x003a57), LV_PART_MAIN);
    
    // Create Lottie object
    g_lottie_obj = lv_lottie_create(scr);
    lv_obj_set_size(g_lottie_obj, LOTTIE_BUFFER_DIM, LOTTIE_BUFFER_DIM);
    lv_obj_center(g_lottie_obj);
    
    // Add touch event handler to screen
    lv_obj_add_event_cb(scr, touch_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    
    size_t buf_size = LOTTIE_BUFFER_DIM * LOTTIE_BUFFER_DIM * 4;  // RGBA format
    g_lottie_buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!g_lottie_buf) {
        ESP_LOGE(TAG, "Failed to allocate Lottie buffer");
        lvgl_port_unlock();
        return;
    }
    
    ESP_LOGI(TAG, "Allocated %zu bytes for Lottie buffer in PSRAM", buf_size);
    
    // Set buffer and load circle Lottie animation (initial)
    lv_lottie_set_buffer(g_lottie_obj, LOTTIE_BUFFER_DIM, LOTTIE_BUFFER_DIM, g_lottie_buf);
    lv_lottie_set_src_data(g_lottie_obj, (const char*)circle_lottie_data, circle_lottie_data_size);
    g_is_cute_bird = false;  // Start with circle
    
    ESP_LOGI(TAG, "Touch the screen to switch between animations!");
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Lottie animation created successfully");
    
    // Keep the app running and display memory info
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    }
}
