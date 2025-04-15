#include "littlefs.h"
#include <errno.h>
#include <string.h>
#include <dirent.h>

#define TAG "littlefs"

esp_vfs_littlefs_conf_t littlefs_conf;

void littlefs_init(esp_vfs_littlefs_conf_t *conf)
{
    ESP_LOGI(TAG, "Starting LittleFS initialization...");

    conf->base_path = "/littlefs";
    conf->partition_label = "storage";
    conf->format_if_mount_failed = true;
    conf->dont_mount = false;

    ESP_LOGI(TAG, "Configuration: base_path=%s, partition_label=%s", 
             conf->base_path, conf->partition_label);

    // 检查分区是否存在
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, conf->partition_label);
    
    if (partition == NULL) {
        ESP_LOGE(TAG, "Failed to find partition with label '%s'", conf->partition_label);
        return;
    }
    ESP_LOGI(TAG, "Found partition: type=%d, subtype=%d, address=0x%lx, size=0x%lx",
             partition->type, partition->subtype, partition->address, partition->size);

    // 配置结构体以初始化并挂载littlefs文件系统
    esp_err_t ret = esp_vfs_littlefs_register(conf);

    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) 
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } 
        else if (ret == ESP_ERR_NOT_FOUND) 
        {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } 
        else 
        {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    ESP_LOGI(TAG, "LittleFS mounted successfully");

    // 添加文件系统信息打印
    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf->partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Partition size: total=%d bytes, used=%d bytes", total, used);
    } else {
        ESP_LOGE(TAG, "Failed to get partition info: %s", esp_err_to_name(ret));
    }

    // 列出目录内容
    DIR* dir = opendir("/littlefs");
    if (dir != NULL) {
        ESP_LOGI(TAG, "Listing files in /littlefs:");
        struct dirent* entry;
        int file_count = 0;
        while ((entry = readdir(dir)) != NULL) {
            ESP_LOGI(TAG, "Found file: %s", entry->d_name);
            file_count++;
        }
        ESP_LOGI(TAG, "Total files found: %d", file_count);
        closedir(dir);
    } else {
        ESP_LOGE(TAG, "Failed to open directory: %s", strerror(errno));
    }

    // 尝试读取一个文件来验证文件系统
    FILE* f = fopen("/littlefs/tle_eph.txt", "r");
    if (f != NULL) {
        ESP_LOGI(TAG, "Successfully opened tle_eph.txt file");
        fclose(f);
    } else {
        ESP_LOGE(TAG, "Failed to open tle_eph.txt file: %s", strerror(errno));
    }
}

void get_littlefs_stat(esp_vfs_littlefs_conf_t *conf)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_littlefs_info(conf->partition_label, &total, &used);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_littlefs_format(conf->partition_label);
    } 
    else 
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void get_file_info(void)
{
    FILE *tle = NULL;
    char buffer[256];

    tle = fopen(FLASH_FILE_PATH, "r");
    if (tle == NULL) {
        ESP_LOGE(TAG, "Failed to open the TLE file: %s", strerror(errno));
        return;
    }
    printf("TLE file content:\n");
    while (fgets(buffer, sizeof(buffer), tle) != NULL) {
        printf("%s", buffer);
    }

    fclose(tle);
}

void sync_latest_time(void)
{
    char latest_time[128];

    // 下载结束之后同步当前时间并记录
    sntp_netif_sync_time();  
    struct timeval tv;
    struct tm time;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &time);
    sprintf(latest_time, "%d.%d.%d %d:%d:%d\n", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);

    FILE *fp = fopen(LATEST_TIME_PATH, "w");
    if (NULL == fp)
    {
        ESP_LOGE(TAG, "Failed to open the latest time file.\n");
        fclose(fp);
        return;
    }
    fprintf(fp, "%s", latest_time);
    printf("Latest update time: %s\n", latest_time);

    fclose(fp);
}