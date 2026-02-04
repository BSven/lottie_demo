/*
 * LVGL Lottie Animation Demo
 * Playing cute_bird.json using LVGL's built-in Lottie support
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
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "lvgl_port.h"

/* Include the Lottie animation data */
#include "assets/cute_bird.c"

static const char *TAG = "lottie_demo";

/* Lottie animation object */
static lv_obj_t *lottie_obj = NULL;

/* Buffer for Lottie rendering (ARGB8888 format, 4 bytes per pixel) */
/* Lottie canvas: 300x300 - allocated in PSRAM due to size */
#define LOTTIE_WIDTH  300
#define LOTTIE_HEIGHT 300
static uint8_t *lottie_buf = NULL;

/* Create Lottie animation */
static void create_lottie_animation(void)
{
    /* Allocate buffer in PSRAM (SPIRAM) due to large size */
    size_t buf_size = LOTTIE_WIDTH * LOTTIE_HEIGHT * 4;
    lottie_buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (lottie_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate Lottie buffer in PSRAM");
        return;
    }
    ESP_LOGI(TAG, "Lottie buffer allocated: %d bytes in PSRAM", buf_size);
    
    /* Create Lottie widget */
    lottie_obj = lv_lottie_create(lv_screen_active());
    
    /* Set buffer for rendering */
    lv_lottie_set_buffer(lottie_obj, LOTTIE_WIDTH, LOTTIE_HEIGHT, lottie_buf);
    
    /* Set the animation data */
    lv_lottie_set_src_data(lottie_obj, cute_bird_lottie_data, cute_bird_lottie_data_size);
    
    /* Center on screen */
    lv_obj_center(lottie_obj);
    
    /* Get the animation and configure it */
    lv_anim_t *anim = lv_lottie_get_anim(lottie_obj);
    if (anim) {
        lv_anim_set_repeat_count(anim, LV_ANIM_REPEAT_INFINITE);
    }
    
    ESP_LOGI(TAG, "Lottie animation created");
    ESP_LOGI(TAG, "  Size: %dx%d", LOTTIE_WIDTH, LOTTIE_HEIGHT);
    ESP_LOGI(TAG, "  Data size: %" PRIu32 " bytes", cute_bird_lottie_data_size);
}

void app_main(void)
{
    printf("LVGL Pure Animation Demo\\n");

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
    
    // Create Lottie animation
    ESP_LOGI(TAG, "Creating Lottie animation...");
    
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return;
    }
    
    /* Create Lottie animation from JSON data */
    create_lottie_animation();
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Lottie animation running...");
    
    // Keep the app running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    }
}
