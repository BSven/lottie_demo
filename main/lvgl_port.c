/*
 * LVGL Display Port for ESP32-P4-WIFI6-Touch-LCD-4B (720x720 MIPI DSI)
 */

#include <stdio.h>
#include "lvgl_port.h"
#include "display_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_st7703.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_ldo_regulator.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "lvgl.h"
#include "esp_lcd_touch_gt911.h"

static const char *TAG = "lvgl_port";

static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;
static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;

static SemaphoreHandle_t lvgl_mux = NULL;

// LVGL Tick Timer Callback
static void lvgl_tick_timer_cb(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// LVGL Lock/Unlock
bool lvgl_port_lock(uint32_t timeout_ms)
{
    const TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

// LVGL Flush Callback - Must explicitly draw to MIPI DSI panel!
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = lv_display_get_user_data(disp);
    const int x1 = area->x1;
    const int y1 = area->y1;
    const int x2 = area->x2 + 1;  // Exclusive end coordinate
    const int y2 = area->y2 + 1;  // Exclusive end coordinate
    
    // Draw bitmap to the panel
    esp_lcd_panel_draw_bitmap(panel, x1, y1, x2, y2, px_map);
    
    lv_display_flush_ready(disp);
}

// Touch Read Callback
static void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t touch = lv_indev_get_user_data(indev);
    esp_lcd_touch_point_data_t touch_points[1];
    uint8_t touch_cnt = 0;
    
    esp_lcd_touch_read_data(touch);
    
    esp_err_t ret = esp_lcd_touch_get_data(touch, touch_points, &touch_cnt, 1);
    
    if (ret == ESP_OK && touch_cnt > 0) {
        data->point.x = touch_points[0].x;
        data->point.y = touch_points[0].y;
        data->state = LV_INDEV_STATE_PRESSED;
        
        // Debug logging for touch coordinates
        ESP_LOGI(TAG, "Touch input: X=%d, Y=%d", touch_points[0].x, touch_points[0].y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// LVGL Task
static void lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    
    while (1) {
        uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        
        if (lvgl_port_lock(0)) {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
        
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

// Enable LDO for DSI PHY power
static esp_err_t lvgl_enable_dsi_phy_power(void)
{
#if LCD_MIPI_DSI_PHY_PWR_LDO_CHAN > 0
    static esp_ldo_channel_handle_t phy_pwr_chan = NULL;
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = LCD_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = LCD_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan));
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
#endif
    return ESP_OK;
}

// Configure backlight using simple GPIO (for testing)
static esp_err_t lvgl_backlight_init(void)
{
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_NUM_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_NUM_BL, 0));  // Active low, so 0 = ON
    
    ESP_LOGI(TAG, "Backlight initialized (GPIO mode)");
    return ESP_OK;
}

esp_err_t lvgl_port_init(void)
{
    ESP_LOGI(TAG, "Initialize LVGL");
    
    // Create LVGL mutex
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    if (lvgl_mux == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return ESP_FAIL;
    }
    
    // Initialize LVGL
    lv_init();
    
    // Configure backlight (PWM)
    ESP_LOGI(TAG, "Initialize backlight");
    ESP_ERROR_CHECK(lvgl_backlight_init());
    
    // Enable MIPI DSI PHY power
    ESP_LOGI(TAG, "Enable DSI PHY power");
    ESP_ERROR_CHECK(lvgl_enable_dsi_phy_power());
    
    // Create MIPI DSI bus using official ST7703 macro
    ESP_LOGI(TAG, "Install MIPI DSI bus");
    esp_lcd_dsi_bus_config_t bus_config = ST7703_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));
    
    // Create DBI IO handle for DSI using official ST7703 macro
    ESP_LOGI(TAG, "Install DBI panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_dbi_io_config_t dbi_config = ST7703_PANEL_IO_DBI_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &io_handle));
    
    // Create ST7703 panel with DMA2D and double-buffering for tear-free display
    ESP_LOGI(TAG, "Install LCD driver of st7703 (DMA2D enabled, double-buffering)");
    esp_lcd_dpi_panel_config_t dpi_config = ST7703_720_720_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
    
    // Enable DMA2D-assisted framebuffer transfer with double buffering
    // num_fbs=2: Double buffering on hardware level prevents tearing
    // use_dma2d=true: Use 2D-DMA for efficient buffer transfers (already set in macro)
    dpi_config.num_fbs = 2;  // Override macro default (1) for tear-free operation
    
    st7703_vendor_config_t vendor_config = {
        .flags = {
            .use_mipi_interface = 1,
        },
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7703(io_handle, &panel_config, &panel_handle));
    
    ESP_LOGI(TAG, "Reset and initialize LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    // Initialize I2C for touch
    ESP_LOGI(TAG, "Initialize I2C for touch");
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = TOUCH_I2C_NUM,
        .sda_io_num = TOUCH_I2C_SDA,
        .scl_io_num = TOUCH_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    i2c_master_bus_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
    
    // Initialize touch controller using the official GT911 pattern
    ESP_LOGI(TAG, "Initialize touch controller GT911");
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = 400000;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &tp_io_handle));
    
    // GT911 driver configuration
    esp_lcd_touch_io_gt911_config_t gt911_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
    };
    
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = TOUCH_I2C_RST,
        .int_gpio_num = TOUCH_I2C_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .driver_data = &gt911_config,
    };
    
    ESP_LOGI(TAG, "Creating GT911 touch handle (addr: 0x%02X)", gt911_config.dev_addr);
    esp_err_t touch_ret = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
    if (touch_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create GT911 touch handle: %s", esp_err_to_name(touch_ret));
        return touch_ret;
    }
    ESP_LOGI(TAG, "GT911 touch controller initialized successfully");
    
    // Create LVGL display (720x720)
    ESP_LOGI(TAG, "Create LVGL display");
    lvgl_disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(lvgl_disp, lvgl_flush_cb);
    lv_display_set_user_data(lvgl_disp, panel_handle);
    
    // Use full-screen buffers in PSRAM for FULL rendering mode (no partial updates)
    // Full screen: 720x720 pixels * 2 bytes = 1,036,800 bytes per buffer
    size_t buffer_size = LCD_H_RES * LCD_V_RES * sizeof(lv_color16_t);
    ESP_LOGI(TAG, "Allocating LVGL full-screen buffers: %zu bytes per buffer (%.2f MB total)", 
             buffer_size, (buffer_size * 2) / (1024.0f * 1024.0f));
    
    // CRITICAL: Use aligned allocation! LV_DRAW_BUF_ALIGN is 64 bytes.
    // heap_caps_malloc does NOT guarantee alignment, causing LVGL assertion loops.
    void *buf1 = heap_caps_aligned_alloc(64, buffer_size, MALLOC_CAP_SPIRAM);
    assert(buf1);
    void *buf2 = heap_caps_aligned_alloc(64, buffer_size, MALLOC_CAP_SPIRAM);
    assert(buf2);
    
    ESP_LOGI(TAG, "LVGL buffers allocated in PSRAM (64-byte aligned): buf1=%p, buf2=%p", buf1, buf2);
    
    // Use the simple API that doesn't do memset - just pass the buffers directly
    lv_display_set_buffers(lvgl_disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_FULL);
    
    ESP_LOGI(TAG, "LVGL display buffers initialized");
    
    // Create LVGL input device (touch)
    ESP_LOGI(TAG, "Create LVGL input device");
    lvgl_touch_indev = lv_indev_create();
    lv_indev_set_type(lvgl_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvgl_touch_indev, lvgl_touch_cb);
    lv_indev_set_user_data(lvgl_touch_indev, touch_handle);
    
    // Create and start LVGL tick timer
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_timer_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    
    // Create LVGL task
    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
    
    ESP_LOGI(TAG, "LVGL initialization complete");
    return ESP_OK;
}

lv_display_t *lvgl_port_get_display(void)
{
    return lvgl_disp;
}

lv_indev_t *lvgl_port_get_indev(void)
{
    return lvgl_touch_indev;
}
