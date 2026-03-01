# 5. ESP32-S3-CAM Hardware Specifics

## 5.1 Pin Definitions (Standard AI-Thinker Version)
Use these definitions unless specified otherwise. Note that SD Card and Camera share some pins or have specific power constraints.
- PWDN_GPIO_NUM: -1
- RESET_GPIO_NUM: -1
- XCLK_GPIO_NUM: 15
- SIOD_GPIO_NUM: 4
- SIOC_GPIO_NUM: 5
- Y9_GPIO_NUM: 16
- Y8_GPIO_NUM: 17
- Y7_GPIO_NUM: 18
- Y6_GPIO_NUM: 12
- Y5_GPIO_NUM: 10
- Y4_GPIO_NUM: 8
- Y3_GPIO_NUM: 9
- Y2_GPIO_NUM: 11
- VSYNC_GPIO_NUM: 6
- HREF_GPIO_NUM: 7
- PCLK_GPIO_NUM: 13

## 5.2 Hardware Conflict Warning
- **SD Card vs Camera**: The SD card uses `CS`, `SCK`, `MOSI`, `MISO`. Ensure these pins do not conflict with the camera pins defined above.
- If the project requires simultaneous usage, warn the user about potential GPIO conflicts or bus sharing overhead.
