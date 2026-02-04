# LVGL Benchmark - ESP32-P4

720×720 MIPI-DSI Display (ST7703) mit LVGL 9.4 auf ESP32-P4.

## Hardware

- **Board:** ESP32-P4-WIFI6-Touch-LCD-4B (Waveshare)
- **Display:** 4" IPS 720×720, MIPI-DSI 2-Lane
- **Touch:** GT911 (I2C)

## Build & Flash

```bash
idf.py build flash monitor
```

## Konfiguration

| Parameter | Wert |
|-----------|------|
| CPU | 360 MHz (Dual-Core) |
| PSRAM | 32 MB @ 200 MHz |
| Farbformat | RGB565 |
| Render Mode | FULL (Double-Buffered) |
| LVGL Task | Core 1 |
* For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-idf/issues)

We will get back to you as soon as possible.
