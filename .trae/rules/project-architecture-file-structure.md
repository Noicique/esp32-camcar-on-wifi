Directory Structure
Adopt the following standard file tree structure. If the user only asks for a snippet, generate the code assuming it belongs in the corresponding directory.

.
├── src/
│ ├── main.cpp # Entry point, setup(), loop()
│ ├── config.h # Pin definitions, macros, global constants
│ ├── camera/ # Camera driver module
│ │ ├── camera_driver.cpp
│ │ └── camera_driver.h
│ ├── network/ # Wi-Fi, HTTP, MQTT modules
│ │ ├── wifi_manager.cpp
│ │ └── wifi_manager.h
│ ├── storage/ # SD Card and SPIFFS modules
│ │ ├── sd_card.cpp
│ │ └── sd_card.h
│ └── utils/ # Helpers (debug, converters)
│ └── debug_helpers.h
├── include/ # Shared headers
├── lib/ # External libraries (if not using platform.io library manager)
├── platformio.ini # Build configuration (MUST specify board_build.partitions and PSRAM options)
