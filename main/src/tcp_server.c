/*
 * Copyright 2025 Cyfarwydd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
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
#include "driver/rmt_tx.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "tcp_server.h"
#include "stepper_motor_encoder.h"

extern unsigned char LedStatus;

static const char *TAG = "TCP SERVER";

void do_retransmit(const int sock)
{
    Tcp_Sentence *Tcp_Sentence_p;
    BaseType_t TXStatus;                    
    int len;
    char rx_buffer[128];                    

    float prev_az = 0, prev_el = 0;
    float curr_az = 0, curr_el = 0;

    float delta_az, delta_el;

    do
    {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);      
        if (len < 0)
        {
            LedStatus = CONNECTED;
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        }
        else if (len == 0)
        {
            LedStatus = CONNECTED;
            ESP_LOGW(TAG, "Connection closed");
        }
        else
        {
            if (uxQueueMessagesWaiting(RotQueueHandler) == 0)
            {
                rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string

                sscanf(rx_buffer, "\\P %f %f", &curr_az, &curr_el);  // 接受数据并转义至对应的变量中
                
                delta_az = curr_az > prev_az ? (curr_az - prev_az) : (prev_az - curr_az);
                delta_el = curr_el > prev_el ? (curr_el - prev_el) : (prev_el - curr_el);

                // 确定卫星此时是否位于地球的背面
#if (NOT_TRACKING_FARSIDE)
                if (curr_el >= 0 && prev_el >= 0)
                {
                    if (delta_az > DELTA_VALUE || delta_el > DELTA_VALUE)
                    {
#endif
                        Tcp_Sentence_p = (Tcp_Sentence *)malloc(sizeof(Tcp_Sentence));
                        if (Tcp_Sentence_p != NULL)
                        {
                            Tcp_Sentence_p->Azimuth = curr_az;
                            Tcp_Sentence_p->Elevation = curr_el;
                            
                            TXStatus = xQueueSend(RotQueueHandler, &Tcp_Sentence_p, 0);  // 向队列发送消息
                            if (TXStatus == pdPASS)
                            {
                                ESP_LOGI(TAG, "send done");
                                prev_az = curr_az;
                                prev_el = curr_el;
                            }
                            else
                            {
                                ESP_LOGI(TAG, "send failed");
                                free(Tcp_Sentence_p);  // 发送失败，释放内存
                            }
                        }
                        else
                            ESP_LOGI(TAG, "alloc failed.\n");
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
#if (NOT_TRACKING_FARSIDE)
                    }
                }
#endif
            }
        }

    } while (len > 0);
}

void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

#ifdef CONFIG_EXAMPLE_IPV4
    if (addr_family == AF_INET)
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    if (addr_family == AF_INET6)
    {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif
    // 创建一个socket，listen_sock为文件描述符
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    // 设置socket选项，该API类似于基础IO中的fcntl
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");
    // 连接
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);
    // 监听
    err = listen(listen_sock, 1);
    // 监听失败，意味着服务器无法接受客户端连接，这是一个严重的错误，后续代码没有任何意义
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Socket listening");
        
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);  // 创建一个新的socket，用于和服务器进行数据传输
        /**
         * @brief   accept返回失败通常代表严重的系统级错误，可能是由于资源耗尽，网络故障或者其他严重错误，多错误处理应该不继续执行任何接下来
         *          的代码并且跳出
         */
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
#ifdef CONFIG_EXAMPLE_IPV4
        if (source_addr.ss_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
        if (source_addr.ss_family == PF_INET6)
        {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);
        // After transmision is over, close the socket and free the memory.
        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}
