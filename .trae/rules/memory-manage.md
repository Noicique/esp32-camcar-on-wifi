# 1. Memory Management (CRITICAL for ESP32-S3-CAM)
## 1.1 PSRAM Utilization
- Assume ESP32-S3-CAM has 8MB PSRAM (OPI/QPI).
- **Rule**: All large buffers (Image frames, audio buffers, large JSON strings) **MUST** be allocated in PSRAM.
- **Implementation**: Use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` or `ps_malloc()` in Arduino framework. Standard `malloc()` may fail for large images.
## 1.2 Resource Release & Memory Leaks Prevention
- **Camera Buffer**: After `esp_camera_fb_get()`, you **MUST** call `esp_camera_fb_return(fb)` in all exit paths (success and error).
- **Pointers**: Check pointers for `nullptr` before use.
- **RAII**: Encourage RAII patterns where possible, or create `init()` / `deinit()` pairs for resources.
