/*
 * LVGL Benchmark Demo
 * Running LVGL's built-in benchmark
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
#include "demos/lv_demos.h"

static const char *TAG = "benchmark_demo";

/* Callback when benchmark finishes */
static void benchmark_finished_cb(const lv_demo_benchmark_summary_t *summary)
{
    ESP_LOGI(TAG, "=== BENCHMARK FINISHED ===");
    ESP_LOGI(TAG, "Average FPS: %" PRId32, summary->total_avg_fps);
    ESP_LOGI(TAG, "Average CPU: %" PRId32 "%%", summary->total_avg_cpu);
    ESP_LOGI(TAG, "Average Render time: %" PRId32 " ms", summary->total_avg_render_time);
    ESP_LOGI(TAG, "Average Flush time: %" PRId32 " ms", summary->total_avg_flush_time);
    ESP_LOGI(TAG, "Valid scenes: %" PRId32, summary->valid_scene_cnt);
    
    /* Show summary on display */
    lv_demo_benchmark_summary_display(summary);
}

void app_main(void)
{
    printf("LVGL Benchmark Demo\n");

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

    // Initialize LVGL display
    ESP_LOGI(TAG, "Initializing LVGL...");
    ESP_ERROR_CHECK(lvgl_display_init());
    
    ESP_LOGI(TAG, "System ready");
    
    // Give LVGL time to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Start benchmark
    ESP_LOGI(TAG, "Starting LVGL Benchmark...");
    
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return;
    }
    
    /* Set callback for when benchmark finishes */
    lv_demo_benchmark_set_end_cb(benchmark_finished_cb);
    
    /* Start the benchmark */
    lv_demo_benchmark();
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Benchmark running...");
    
    // Keep the app running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    }
}
