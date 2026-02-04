/*
 * LVGL Lottie Animation Demo for ESP32-P4
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

/* Lottie animation data */
extern const uint8_t circle_lottie_data[];
extern const uint32_t circle_lottie_data_size;

static const char *TAG = "lottie";

void app_main(void)
{
    printf("LVGL Benchmark Demo\\n");

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
    
    // Create Lottie animation
    ESP_LOGI(TAG, "Creating Lottie animation...");
    
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return;
    }
    
    /* Create Lottie widget */
    lv_obj_t *lottie = lv_lottie_create(lv_screen_active());
    lv_lottie_set_src_data(lottie, circle_lottie_data, circle_lottie_data_size);
    lv_lottie_set_draw_buf(lottie, lv_draw_buf_create(300, 300, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO));
    lv_obj_set_size(lottie, 300, 300);
    lv_obj_center(lottie);
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Lottie animation running...");
    
    // Keep the app running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    }
}
