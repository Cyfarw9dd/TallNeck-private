#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "nxjson.h"
#include "hash_table.h"

// 转发器数据结构
struct transponder {
    char uuid[30];
    int catnum;
    char description[80];
    int64_t uplink_low;
    int64_t uplink_high;
    int64_t downlink_low;
    int64_t downlink_high;
    char mode[20];
    int invert;
    double baud;
    int alive;
};

// 模式数据结构
struct modes {
    int id;
    char name[80];
};

// 模式数据
typedef struct {
    int mode_id;
    char* mode_name;
} trsp_mode_t;

// 转发器数据
typedef struct {
    int cat_num;
    int num_trsp;
} trsp_t;



bool trsp_update_files(const char* input_file);
