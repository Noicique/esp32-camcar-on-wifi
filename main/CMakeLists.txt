idf_component_register(
    SRCS "main.c"
         "hal/hal_motor.c"
         "hal/hal_camera.c"
         "motor_control/motor_control.c"
         "camera/camera_driver.c"
         "camera/camera_stream.c"
         "network/wifi_manager.c"
         "network/http_server.c"
         "network/command_handler.c"
    INCLUDE_DIRS "." "hal" "motor_control" "camera" "network"
)
