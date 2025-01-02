idf_component_register(SRCS "src/main.c"
                            "src/tcp_server.c"
                            "src/stepper_motor_encoder.c"
                            "src/wifi_sta.c"
                            "src/littlefs.c"
                            "src/get_tle.c"
                            "src/sgp4sdp4.c"
                            "src/sgp_in.c"
                            "src/sgp_math.c"
                            "src/sgp_obs.c"
                            "src/sgp_time.c"
                            "src/solar.c"
                            "src/uart.c"
                            "src/lvgl_display.c"
                            "src/nxjson.c"
                            "src/trsp_update.c"
                            "src/hash_table.c"
                    INCLUDE_DIRS "include")

include_directories(${CMAKE_SOURCE_DIR}/build/config)

