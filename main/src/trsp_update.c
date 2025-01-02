/**
 * @file    trsp_update.c
 * @brief   更新转发器数据
 *          trsp is the address of the transponder
 * @author 
 * @date    2025-01-01
 */

#include "trsp_update.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "trsp_type.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#define CONFIG_BASE_PATH "/littlefs"
#define MAX_PATH_LENGTH 512

#define TAG "trsp_update"

#define TRSP_URL "https://db.satnogs.org/api/transmitters/?format=json"
#define TRSP_DIR_NAME         "trsp"
#define MODES_FILE_NAME       "modes.json"
#define TRSP_FILE_EXT         ".trsp"

// 释放模式数据
static void free_mode_value(void* value) {
    trsp_mode_t* mode = (trsp_mode_t*)value;
    if (mode) {
        free(mode->mode_name);
        free(mode);
    }
}

// 释放转发器数据
static void free_trsp_value(void* value) {
    free(value);
}

// 确保目录存在
static bool ensure_directory(const char* path) {
    if (access(path, F_OK) != 0) {
        if (mkdir(path, 0755) != 0) {
            printf("Failed to create directory: %s\n", path);
            return false;
        }
    }
    return true;
}

// HTTP事件处理函数
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer for storing HTTP response
    static int output_len;       // Length of the data read
    
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Handle chunked responses
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }
            break;
            
        case HTTP_EVENT_ON_FINISH:
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
            
        case HTTP_EVENT_DISCONNECTED:
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
            
        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief Download transponder data from SatNOGS database
 * @return true if successful, false otherwise
 */
bool download_trsp_data(void) {
    ESP_LOGI(TAG, "Downloading transponder data from SatNOGS");
    
    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = TRSP_URL,
        .cert_pem = NULL,
        .skip_cert_common_name_check = true,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = _http_event_handler,
    };
    
    // Initialize HTTP client
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }
    
    // Set HTTP method to GET
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    
    // Perform HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }
    
    // Check HTTP status
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status code %d", status_code);
        esp_http_client_cleanup(client);
        return false;
    }
    
    // Get response length
    int content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", status_code, content_length);
    
    // Allocate memory for response
    char *response_data = malloc(content_length + 1);
    if (response_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response data");
        esp_http_client_cleanup(client);
        return false;
    }
    
    // Read response data
    int read_len = esp_http_client_read(client, response_data, content_length);
    if (read_len <= 0) {
        ESP_LOGE(TAG, "Failed to read response data");
        free(response_data);
        esp_http_client_cleanup(client);
        return false;
    }
    response_data[read_len] = '\0';
    
    // Process the JSON data using existing trsp_update_files function
    bool result = trsp_update_files(response_data);
    
    // Cleanup
    free(response_data);
    esp_http_client_cleanup(client);
    
    return result;
}

/**
 *  @brief               更新转发器数据主体函数
 *  @param json_data     输入JSON数据
 */
bool trsp_update_files(const char* json_data) {
    FILE* mfp;
    size_t mfplen;
    long flen;
    int result;
    FILE* ffile;
    char* jsn_object;
    unsigned int idx;
    trsp_mode_t* nmode;
    trsp_t* ntrsp;
    int* key;

    char userconfdir[MAX_PATH_LENGTH] = {0};
    char trspfolder[MAX_PATH_LENGTH] = {0};
    char modesfile[MAX_PATH_LENGTH] = {0};
    char trspfile[MAX_PATH_LENGTH] = {0};

    // 使用 strlcpy 和 strlcat 来安全地构建路径
    if (snprintf(userconfdir, sizeof(userconfdir), "%s/conf", CONFIG_BASE_PATH) >= sizeof(userconfdir)) {
        ESP_LOGE(TAG, "Path too long for userconfdir");
        return false;
    }
    
    if (snprintf(trspfolder, sizeof(trspfolder), "%s/%s", userconfdir, TRSP_DIR_NAME) >= sizeof(trspfolder)) {
        ESP_LOGE(TAG, "Path too long for trspfolder");
        return false;
    }
    
    if (snprintf(modesfile, sizeof(modesfile), "%s/%s", trspfolder, MODES_FILE_NAME) >= sizeof(modesfile)) {
        ESP_LOGE(TAG, "Path too long for modesfile");
        return false;
    }

    // 确保目录存在
    if (!ensure_directory(userconfdir) || !ensure_directory(trspfolder)) {
        return false;
    }

    // 创建哈希表
    hash_table_t* modes_hash = hash_table_create(free_mode_value);
    hash_table_t* trsp_hash = hash_table_create(free_trsp_value);
    
    if (!modes_hash || !trsp_hash) {
        printf("Failed to create hash tables\n");
        return false;
    }

    // 读取模式文件
    mfp = fopen(modesfile, "r");
    if (mfp != NULL) {
        fseek(mfp, 0, SEEK_END);
        flen = ftell(mfp);

        if (flen > 1) {
            mfplen = flen;
            rewind(mfp);
            jsn_object = malloc(mfplen);
            
            if (jsn_object) {
                result = fread(jsn_object, mfplen, 1, mfp);

                if (result == 1) {
                    const nx_json* json = nx_json_parse(jsn_object, 0);
                    if (json) {
                        idx = 0;
                        while (1) {
                            const nx_json* json_obj = nx_json_item(json, idx++);
                            struct modes m_modes;

                            if (json_obj->type == NX_JSON_NULL)
                                break;

                            m_modes.id = nx_json_get(json_obj, "id")->int_value;
                            strncpy(m_modes.name,
                                    nx_json_get(json_obj, "name")->text_value, 79);
                            m_modes.name[79] = 0;

                            key = malloc(sizeof(int));
                            if (key) {
                                *key = m_modes.id;
                                nmode = malloc(sizeof(trsp_mode_t));
                                if (nmode) {
                                    nmode->mode_id = m_modes.id;
                                    nmode->mode_name = strdup(m_modes.name);
                                    hash_table_insert(modes_hash, key, nmode);
                                }
                            }
                        }
                        nx_json_free(json);
                    }
                }
                free(jsn_object);
            }
        }
        fclose(mfp);
    }

    // 处理转发器数据
    mfp = fopen(json_data, "r");
    if (mfp != NULL) {
        fseek(mfp, 0, SEEK_END);
        flen = ftell(mfp);

        if (flen > 1) {
            mfplen = flen;
            rewind(mfp);
            jsn_object = malloc(mfplen);
            
            if (jsn_object) {
                result = fread(jsn_object, mfplen, 1, mfp);

                if (result == 1) {
                    const nx_json* json = nx_json_parse(jsn_object, 0);
                    if (json) {
                        idx = 0;
                        while (1) {
                            const nx_json* json_obj = nx_json_item(json, idx++);
                            struct transponder m_trsp;

                            if (json_obj->type == NX_JSON_NULL)
                                break;

                            // 解析转发器数据
                            strncpy(m_trsp.description,
                                    nx_json_get(json_obj, "description")->text_value, 79);
                            m_trsp.description[79] = 0;
                            m_trsp.catnum = nx_json_get(json_obj, "norad_cat_id")->int_value;
                            m_trsp.uplink_low = nx_json_get(json_obj, "uplink_low")->int_value;
                            m_trsp.uplink_high = nx_json_get(json_obj, "uplink_high")->int_value;
                            m_trsp.downlink_low = nx_json_get(json_obj, "downlink_low")->int_value;
                            m_trsp.downlink_high = nx_json_get(json_obj, "downlink_high")->int_value;

                            // 获取模式信息
                            key = malloc(sizeof(int));
                            if (key) {
                                *key = nx_json_get(json_obj, "mode_id")->int_value;
                                nmode = hash_table_lookup(modes_hash, key);
                                if (nmode)
                                    strncpy(m_trsp.mode, nmode->mode_name, sizeof(m_trsp.mode) - 1);
                                else
                                    snprintf(m_trsp.mode, sizeof(m_trsp.mode), "%d", *key);
                                free(key);
                            }

                            m_trsp.invert = nx_json_get(json_obj, "invert")->int_value;
                            m_trsp.baud = nx_json_get(json_obj, "baud")->dbl_value;
                            m_trsp.alive = nx_json_get(json_obj, "alive")->int_value;

                            // 创建转发器文件
                            if (snprintf(trspfile, sizeof(trspfile), "%s/%d%s", 
                                    trspfolder, m_trsp.catnum, TRSP_FILE_EXT) >= sizeof(trspfile)) {
                                ESP_LOGE(TAG, "Path too long for trspfile");
                                return false;
                            }

                            key = malloc(sizeof(int));
                            if (key) {
                                *key = m_trsp.catnum;
                                ntrsp = hash_table_lookup(trsp_hash, key);

                                if (!ntrsp) {
                                    ntrsp = malloc(sizeof(trsp_t));
                                    if (ntrsp) {
                                        ntrsp->cat_num = m_trsp.catnum;
                                        ntrsp->num_trsp = 1;
                                        hash_table_insert(trsp_hash, key, ntrsp);
                                        remove(trspfile);
                                    }
                                }
                            }

                            // 写入转发器数据
                            ffile = fopen(trspfile, "a");
                            if (ffile != NULL) {
                                fprintf(ffile, "\n[%s]\n", m_trsp.description);
                                if (m_trsp.uplink_low > 0)
                                    fprintf(ffile, "UP_LOW=%lld\n", m_trsp.uplink_low);
                                if (m_trsp.uplink_high > 0)
                                    fprintf(ffile, "UP_HIGH=%lld\n", m_trsp.uplink_high);
                                if (m_trsp.downlink_low > 0)
                                    fprintf(ffile, "DOWN_LOW=%lld\n", m_trsp.downlink_low);
                                if (m_trsp.downlink_high > 0)
                                    fprintf(ffile, "DOWN_HIGH=%lld\n", m_trsp.downlink_high);
                                fprintf(ffile, "MODE=%s\n", m_trsp.mode);
                                if (m_trsp.baud > 0.0)
                                    fprintf(ffile, "BAUD=%.0f\n", m_trsp.baud);
                                if (m_trsp.invert)
                                    fprintf(ffile, "INVERT=true\n");
                                fclose(ffile);
                            }
                        }
                        nx_json_free(json);
                    }
                }
                free(jsn_object);
            }
        }
        fclose(mfp);
    }

    // 清理
    hash_table_destroy(modes_hash);
    hash_table_destroy(trsp_hash);

    return true;
}