idf_component_register(
    SRCS "main.c"
         "motor_control/motor_control.c"
         "camera/camera_driver.c"
         "network/wifi_manager.c"
         "network/http_server.c"
    INCLUDE_DIRS "." "motor_control" "camera" "network"
)
