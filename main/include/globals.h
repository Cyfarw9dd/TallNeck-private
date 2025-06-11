#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

extern TaskHandle_t tle_download_handler;
extern TaskHandle_t orbit_trking_handler;
extern TaskHandle_t tcp_server_handler;
extern TaskHandle_t stepper_motor_handler;
extern TaskHandle_t uart_handler;
extern TaskHandle_t gui_handler;

extern QueueHandle_t SatnameQueueHandler;
extern QueueHandle_t SatelliteParamsQueueHandler;   

// 任务通知传递掩码
typedef enum
{
    NO_EVENT = 0X00,
    UPDATE_TLE = 0X01,
    START_ORB_TRKING = 0x02,
    END_ORB_TRKING = 0X03,
    LAUNCH_TCP = 0x04,
    LAUNCH_ROT = 0x05
}EVENT_BITS;

typedef struct {
    char sat_name[128];          // 卫星名称
    double sat_azi;              // 方位角
    double sat_ele;              // 仰角
    double sat_range;            // 距离
    double sat_range_rate;       // 距离变化率
    double sat_lat;              // 纬度
    double sat_lon;              // 经度
    double sat_alt;              // 高度
    double sat_vel;              // 速度
    char sat_status[12];         // 卫星状态
    double eclipse_depth;        // 阴影深度
    struct tm utc;               // UTC 时间
    char ephem[5];               // 星历类型
} satellite_params_t;

