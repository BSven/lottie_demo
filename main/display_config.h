/*
 * ESP32-P4-WIFI6-Touch-LCD-4B Display Configuration
 * 
 * Display: 4.0" IPS LCD 720x720 (MIPI DSI)
 * Touch: GT911 capacitive touch controller
 * Interface: MIPI DSI 2-lane
 */

#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

// LCD Settings (MIPI DSI - 720x720)
#define LCD_H_RES              720
#define LCD_V_RES              720
#define LCD_BIT_PER_PIXEL      16
#define LCD_COLOR_PIXEL_FORMAT RGB565
#define LCD_RGB_ELEMENT_ORDER  LCD_RGB_ELEMENT_ORDER_RGB

// MIPI DSI Settings
#define LCD_MIPI_DSI_LANE_NUM           2
#define LCD_MIPI_DSI_LANE_BITRATE_MBPS  480

// LDO and Power Settings (matching test program exactly!)
#define LCD_MIPI_DSI_PHY_PWR_LDO_CHAN         3
#define LCD_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV   2500

// Display Buffer
#define LCD_NUM_FBS            2
#define LVGL_BUFFER_HEIGHT     100

// LCD GPIO Pins for ESP32-P4 (matching test program exactly!)
#define LCD_PIN_NUM_RST        27  // Reset pin
#define LCD_PIN_NUM_BL         26  // Backlight pin

// LEDC PWM Configuration for Backlight
#define LCD_LEDC_CH            0

// Touch I2C Settings
#define TOUCH_I2C_NUM          0
#define TOUCH_I2C_SCL          8
#define TOUCH_I2C_SDA          7
#define TOUCH_I2C_INT          4
#define TOUCH_I2C_RST          5  // Shared with LCD reset

// LVGL Settings
#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE   (32 * 1024)  // Increased for ThorVG Lottie rendering (was 4KB, now 32KB)
#define LVGL_TASK_PRIORITY     2

#endif // DISPLAY_CONFIG_H
