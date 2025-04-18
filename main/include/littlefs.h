#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_littlefs.h"
#include "esp_log.h"

#include "get_tle.h"
#include "globals.h"
#include "sgp4sdp4.h"

extern esp_vfs_littlefs_conf_t littlefs_conf;

void littlefs_init(esp_vfs_littlefs_conf_t *conf);

void get_littlefs_stat(esp_vfs_littlefs_conf_t *conf);

void get_file_info(void);

void sync_latest_time(void);
