idf_component_register(SRCS "hello_world_main.c" "lvgl_port.c" "assets/circle_lottie.c" "assets/cute_bird.c"
                       PRIV_REQUIRES spi_flash esp_lcd esp_driver_gpio esp_driver_i2c esp_timer lvgl__lvgl
                       INCLUDE_DIRS ".")
