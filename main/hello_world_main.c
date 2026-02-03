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
#include "lottie_animation.h"

static const char *TAG = "main";

static void lottie_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Creating Lottie animation demo");
    
    // Give LVGL time to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        vTaskDelete(NULL);
        return;
    }
    
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x003a57), LV_PART_MAIN);
    
    // Create Lottie widget (300x300 as in JSON)
    lv_obj_t *lottie_obj = lv_lottie_create(scr);
    lv_obj_center(lottie_obj);
    
    // Allocate buffer for Lottie animation (ARGB8888 = 4 bytes per pixel)
    size_t buf_size = 300 * 300 * 4;
    void *lottie_buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!lottie_buf) {
        ESP_LOGE(TAG, "Failed to allocate Lottie buffer");
        lvgl_port_unlock();
        vTaskDelete(NULL);
        return;
    }
    
    lv_lottie_set_buffer(lottie_obj, 300, 300, lottie_buf);
    
    // Load Lottie animation from embedded data (not from file)
    lv_lottie_set_src_data(lottie_obj, lottie_animation_json, lottie_animation_json_size);
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Lottie animation created successfully");
    
    // Task completed, delete self
    vTaskDelete(NULL);
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
    
    // Create Lottie animation demo in a dedicated task with larger stack
    // ThorVG vector rendering requires significant stack space (recursive algorithms)
    xTaskCreate(lottie_task, "lottie_task", 32768, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "System ready");
    
    // Keep the app running and display memory info
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    }
}
