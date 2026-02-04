/*
 * LVGL Display Port for ESP32-P4-WIFI6-Touch-LCD-4B (720x720 MIPI DSI)
 * Using ESP-BSP esp_lvgl_port component with avoid_tearing for flicker-free display
 */

#include <stdio.h>
#include "lvgl_port.h"
#include "display_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_st7703.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_ldo_regulator.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lcd_touch_gt911.h"

/* ESP-BSP LVGL Port - handles VSync, double-buffering, and avoid_tearing */
#include "esp_lvgl_port.h"

static const char *TAG = "lvgl_port";

static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;
static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;

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

// Configure backlight using simple GPIO
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

esp_err_t lvgl_display_init(void)
{
    ESP_LOGI(TAG, "Initialize LVGL using ESP-BSP esp_lvgl_port (avoid_tearing mode)");
    
    // ==========================================
    // Step 1: Initialize ESP-BSP LVGL Port
    // ==========================================
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = LVGL_TASK_PRIORITY,
        .task_stack = LVGL_TASK_STACK_SIZE,
        .task_affinity = 1,  // Pin to Core 1
        .task_max_sleep_ms = LVGL_TASK_MAX_DELAY_MS,
        .timer_period_ms = LVGL_TICK_PERIOD_MS,
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
    ESP_LOGI(TAG, "ESP-BSP LVGL port initialized");
    
    // ==========================================
    // Step 2: Configure backlight
    // ==========================================
    ESP_LOGI(TAG, "Initialize backlight");
    ESP_ERROR_CHECK(lvgl_backlight_init());
    
    // ==========================================
    // Step 3: Enable MIPI DSI PHY power
    // ==========================================
    ESP_LOGI(TAG, "Enable DSI PHY power");
    ESP_ERROR_CHECK(lvgl_enable_dsi_phy_power());
    
    // ==========================================
    // Step 4: Create MIPI DSI bus
    // ==========================================
    ESP_LOGI(TAG, "Install MIPI DSI bus");
    esp_lcd_dsi_bus_config_t bus_config = ST7703_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));
    
    // ==========================================
    // Step 5: Create DBI IO handle for DSI
    // ==========================================
    ESP_LOGI(TAG, "Install DBI panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_dbi_io_config_t dbi_config = ST7703_PANEL_IO_DBI_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &io_handle));
    
    // ==========================================
    // Step 6: Create ST7703 panel with double-buffering
    // ==========================================
    ESP_LOGI(TAG, "Install LCD driver of st7703 (double-buffering for avoid_tearing)");
    esp_lcd_dpi_panel_config_t dpi_config = ST7703_720_720_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
    
    // CRITICAL: num_fbs=2 required for avoid_tearing mode
    dpi_config.num_fbs = 2;
    
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
    
    // ==========================================
    // Step 7: Reset and initialize LCD panel
    // ==========================================
    ESP_LOGI(TAG, "Reset and initialize LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    // ==========================================
    // Step 8: Add display to ESP-BSP LVGL port (with avoid_tearing!)
    // ==========================================
    ESP_LOGI(TAG, "Add MIPI-DSI display to LVGL with avoid_tearing enabled");
    
    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .full_refresh = true,  // Redraw entire screen each frame (clears old pixels)
            .buff_dma = true,    // Use DMA-capable buffers
            .buff_spiram = true, // Allocate buffers in PSRAM
            .sw_rotate = true,   // Use software rotation
        },
    };
    
    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = {
            .avoid_tearing = true,  // KEY: Enable VSync-based tearing prevention!
        },
    };
    
    lvgl_disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    if (lvgl_disp == NULL) {
        ESP_LOGE(TAG, "Failed to add MIPI-DSI display to LVGL");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "LVGL display added with avoid_tearing mode");
    
    // ==========================================
    // Step 9: Initialize I2C for touch
    // ==========================================
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
    
    // ==========================================
    // Step 10: Initialize touch controller GT911
    // ==========================================
    ESP_LOGI(TAG, "Initialize touch controller GT911");
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = 400000;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &tp_io_handle));
    
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
    
    // ==========================================
    // Step 11: Add touch to ESP-BSP LVGL port
    // ==========================================
    ESP_LOGI(TAG, "Add touch input device to LVGL");
    
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (lvgl_touch_indev == NULL) {
        ESP_LOGE(TAG, "Failed to add touch input device");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "LVGL initialization complete!");
    ESP_LOGI(TAG, "  - Display: %dx%d MIPI-DSI", LCD_H_RES, LCD_V_RES);
    ESP_LOGI(TAG, "  - Mode: avoid_tearing (VSync synchronized)");
    ESP_LOGI(TAG, "  - Double buffering: enabled");
    ESP_LOGI(TAG, "  - Touch: GT911");
    ESP_LOGI(TAG, "========================================");
    
    return ESP_OK;
}

lv_display_t *lvgl_display_get_display(void)
{
    return lvgl_disp;
}

lv_indev_t *lvgl_display_get_indev(void)
{
    return lvgl_touch_indev;
}

/* Note: lvgl_port_lock and lvgl_port_unlock are now provided by esp_lvgl_port library */
