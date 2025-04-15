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
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_littlefs.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_crt_bundle.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

#include "littlefs.h"
#include "globals.h"
// #include "uart.h"

#define ISS_URL                 "https://celestrak.org/NORAD/elements/gp.php?CATNR=25544&FORMAT=tle"
#define AMATEUR_URL             "https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle"

#define FLASH_FILE_PATH         "/littlefs/tle_eph.txt"  // 从本地编辑文件并烧录  
#define FILE_PATH               "/littlefs/tle_data.txt"  // 通过下载功能下载文件，推荐一周更新一次
#define LATEST_TIME_PATH        "/littlefs/latest_time.txt"  // 保存着上次下载数据的更新时间
#define WIFI_CONNECTED_BIT      BIT0


void sntp_netif_sync_time_init(void);

void sntp_netif_sync_time(void);

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

void download_tle_task(void);

