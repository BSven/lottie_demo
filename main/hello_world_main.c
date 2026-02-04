/*
 * LVGL Animation - Matching Lottie circle_lottie.json behavior
 * Pulsing circle (scale 50% → 100% → 50%) - identical to Lottie
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

static const char *TAG = "lottie_anim";

/* Circle object */
static lv_obj_t *circle = NULL;

/*
 * Lottie animation analysis:
 * - Canvas: 300x300
 * - Circle: 100x100 ellipse at center (150, 150)
 * - Scale keyframes:
 *   - t=0:  50% (circle is 50x50)
 *   - t=30: 100% (circle is 100x100)  
 *   - t=60: 50% (circle is 50x50)
 * - Duration: 60 frames @ 30fps = 2000ms
 * - Easing: ease-in-out (cubic bezier)
 *
 * For 720x720 display, scale factor = 720/300 = 2.4
 * Base circle size: 100 * 2.4 = 240px at 100%
 * At 50%: 120px, at 100%: 240px
 */

#define CIRCLE_SIZE_MIN  120   /* 50% of base (240 * 0.5) */
#define CIRCLE_SIZE_MAX  240   /* 100% of base */
#define ANIM_DURATION_MS 2000  /* Full cycle: 2 seconds */

/* Animation callback - directly set circle size and re-center */
static void anim_size_cb(void *var, int32_t size)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_size(obj, size, size);
    lv_obj_center(obj);
}

/* Create animated circle matching Lottie behavior exactly */
static void create_lottie_circle(void)
{
    /* Create circle on active screen */
    circle = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(circle);
    
    /* Lottie color: [0.2, 0.7, 1, 1] = RGB(51, 179, 255) */
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle, lv_color_make(51, 179, 255), 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    
    /* Start at minimum size (50%) */
    lv_obj_set_size(circle, CIRCLE_SIZE_MIN, CIRCLE_SIZE_MIN);
    lv_obj_center(circle);
    
    /* 
     * Scale animation: 50% → 100% → 50%
     * - First half (0-1000ms): grow from 120px to 240px
     * - Second half (1000-2000ms): shrink from 240px to 120px
     * - Easing: ease-in-out (matches Lottie cubic bezier)
     */
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, circle);
    lv_anim_set_exec_cb(&anim, anim_size_cb);
    lv_anim_set_values(&anim, CIRCLE_SIZE_MIN, CIRCLE_SIZE_MAX);
    lv_anim_set_duration(&anim, ANIM_DURATION_MS / 2);  /* 1000ms to grow */
    lv_anim_set_playback_duration(&anim, ANIM_DURATION_MS / 2);  /* 1000ms to shrink */
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
    
    ESP_LOGI(TAG, "Lottie-identical circle animation started");
    ESP_LOGI(TAG, "  Size: %dpx (50%%) to %dpx (100%%)", CIRCLE_SIZE_MIN, CIRCLE_SIZE_MAX);
    ESP_LOGI(TAG, "  Duration: %dms cycle", ANIM_DURATION_MS);
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
    
    // Create Lottie-style animation
    ESP_LOGI(TAG, "Creating Lottie-identical circle animation...");
    
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return;
    }
    
    /* Create pulsing circle - identical to Lottie */
    create_lottie_circle();
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Animation running...");
    
    // Keep the app running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    }
}
