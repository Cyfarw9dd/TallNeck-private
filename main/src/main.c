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
#include "protocol_examples_common.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "tcp_server.h"
#include "stepper_motor_encoder.h"
#include "esp_task_wdt.h"
#include "wifi_sta.h"
#include "esp_littlefs.h"
#include "esp_http_client.h"
#include "http_app.h"
#include "esp_crt_bundle.h"
#include "esp_sntp.h"

#include "wifi_manager.h"
#include "littlefs.h"
#include "get_tle.h"
#include "sgp4sdp4.h"
#include "uart.h"
#include "globals.h"
#include "lvgl_display.h"


#define NOTCONN_PERIOD          pdMS_TO_TICKS(500)
#define CONN_PERIOD             pdMS_TO_TICKS(10000)
#define RECV_PERIOD             pdMS_TO_TICKS(50)

// 预编译宏
// #define ORBIT_TRKING            true
#define DOWNLOAD_TLE            true   
#define TAG                     "main"

char test_buffer[128];

TimerHandle_t LedTimerHandle; 
gpio_num_t gpio_led_num = GPIO_NUM_2;       
int LedCounter = 0;     
unsigned char LedStatus = NOTCONNECTED;     
BaseType_t LedTimerStarted;

TaskHandle_t tle_download_handler;
TaskHandle_t orbit_trking_handler;
TaskHandle_t tcp_server_handler;
TaskHandle_t stepper_motor_handler;
TaskHandle_t uart_handler;
TaskHandle_t gui_handler;

QueueHandle_t SatnameQueueHandler = NULL;
QueueHandle_t SatelliteParamsQueueHandler = NULL;

static void Led_Init(void)
{
    gpio_set_direction(gpio_led_num, GPIO_MODE_OUTPUT);     
}

static void led_timer_callback(TimerHandle_t xTimer)
{
    LedCounter++;
    gpio_set_level(gpio_led_num, LedCounter % 2);
    if (LedStatus == CONNECTED)
    {
        LedCounter = 2;
    }
    else if (LedStatus == RECVIVING)
    {
        xTimerChangePeriod(xTimer, RECV_PERIOD, 0);
    }
    else
    {
        ;
    }
}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ok(void *pvParameter)
{
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
    ESP_LOGI(TAG, "Using the keywords through the uart to activate certain function.\n");
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // 延时一段事件再开启sntp同步
    // download_tle_task();
}

void app_main(void)
{
    Led_Init();  // LED初始化
    littlefs_init(&littlefs_conf);  // LittleFS文件系统初始化
    setenv("TZ", "CST-8", 1);  // 将时区设置为中国标准时间
    tzset();
    sntp_netif_sync_time_init();  // sntp时间同步初始化

    // esp_err_t err = nvs_flash_erase();  // 用于擦除nvs部分
    LedTimerHandle = xTimerCreate("led_controller", NOTCONN_PERIOD, pdTRUE, 0, led_timer_callback);  // 创建LED定时器
    RotQueueHandler = xQueueCreate(5, sizeof(Tcp_Sentence *));  // 创建用于传输俯仰角数据的消息队列
    SatnameQueueHandler = xQueueCreate(5, SAT_NMAE_LENGTH);
    SatelliteParamsQueueHandler = xQueueCreate(1, sizeof(satellite_params_t));
    // 检查定时器和消息队列是否创建完成
    if (NULL == RotQueueHandler)
    {
        ESP_LOGE(TAG, "Rotator message queue create failed.\n");    
    }
    if (NULL == SatnameQueueHandler)
    {
        ESP_LOGE(TAG, "Satellite name message queue create failed.\n");
    }
    if (LedTimerHandle != NULL)
    {
        LedTimerStarted = xTimerStart(LedTimerHandle, 0);  // 启动LED定时器
    }
    if (SatelliteParamsQueueHandler == NULL) {
        ESP_LOGE("QUEUE", "Failed to create satellite parameters queue");
    }

    // wifi manager IP address: 10.10.0.1
	// wifi_manager_start();
    // 回调函数，用于返回IP 
	// wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
    // 旋转器控制任务，调用rmt生成精确波形，后期考虑移至ISR的回调函数中
    // xTaskCreatePinnedToCore(rotator_controller, "rotator_control", 4096, (void *)RotQueueHandler, 3, &stepper_motor_handler, 1);
    // TCP server任务
    // xTaskCreatePinnedToCore(tcp_server_task, "tcp_server", 4096, (void *)RotQueueHandler, 5, &tcp_server_handler, 0);
    // TLE下载任务，属于wifi协议栈，位于核心0
    // xTaskCreatePinnedToCore(download_tle_task, "download_tle", 8192, NULL, 7, &tle_download_handler, 0);
    // sgp4sdp4轨道预测任务，位于核心1
    xTaskCreatePinnedToCore(orbit_trking_task, "orbit_trking", 8192, NULL, 5, &orbit_trking_handler, 1);
    // uart前台交互任务，高优先级，位于核心0
    xTaskCreatePinnedToCore(echo_task, "uart_echo", 8192, NULL, 10, &uart_handler, 0);
    // gui任务，高优先级，位于核心1，如果处于核心0，会导致堆栈溢出
    lvgl_display_init();
    xTaskCreatePinnedToCore(gui_task, "gui_task", 8192, NULL, 9, &gui_handler, 1);
    LedStatus = NOTCONNECTED;
}
