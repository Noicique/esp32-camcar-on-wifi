# 4. Error Handling & Debugging

## 4.1 Mandatory Error Checking
- Do not assume hardware calls always succeed. Check return values of `esp_camera_init`, `wifi_connect`, `sd_card_mount`.
- If an error occurs, the system must:
  1. Print a detailed error message.
  2. Attempt recovery (e.g., retry, soft reset).
  3. Enter a safe state if recovery fails (e.g., blink LED rapidly).

## 4.2 Logging Strategy
- Use the ESP-IDF logging system (`ESP_LOGE`, `ESP_LOGW`, `ESP_LOGI`, `ESP_LOGD`) or `Serial.printf` with log levels.
- **Key Information to Log**:
  - Initialization status (Success/Fail).
  - Memory usage (Free heap, Free PSRAM) at startup and during critical operations.
  - Network disconnection events.
- **Format**: `[TAG] Action: Detail (Result)`
  - *Good*: `[CAMERA] Init: Allocating framebuffer... OK`
  - *Bad*: `Camera started.`
