/*
 * @Description: In User Settings Edit
 * @Author: your name
 * @Date: 2019-09-24 11:24:16
 * @LastEditTime: 2019-10-09 16:59:58
 * @LastEditors: Please set LastEditors
 */

/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#include "infra_config.h"

void HAL_Printf(const char *fmt, ...);
int HAL_Snprintf(char *str, const int len, const char *fmt, ...);

#ifdef DEPRECATED_LINKKIT
#include "solo.c"
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "infra_types.h"
#include "infra_defs.h"
#include "infra_compat.h"
#include "infra_compat.h"
#ifdef INFRA_MEM_STATS
#include "infra_mem_stats.h"
#endif
#include "dev_model_api.h"
#include "dm_wrapper.h"
#include "cJSON.h"
#ifdef ATM_ENABLED
#include "at_api.h"
#endif

#include "rgb.h"
#include "esp_log.h"

static const char *TAG = "linkkit_example_solo";

#define EXAMPLE_TRACE(...)                                      \
    do                                                          \
    {                                                           \
        HAL_Printf("\033[1;32;40m%s.%d: ", __func__, __LINE__); \
        HAL_Printf(__VA_ARGS__);                                \
        HAL_Printf("\033[0m\r\n");                              \
    } while (0)

#define EXAMPLE_MASTER_DEVID (0)
#define EXAMPLE_YIELD_TIMEOUT_MS (200)

typedef struct
{
    int master_devid;
    int cloud_connected;
    int master_initialized;
} user_example_ctx_t;

/**
 * These PRODUCT_KEY|PRODUCT_SECRET|DEVICE_NAME|DEVICE_SECRET are listed for demo only
 *
 * When you created your own devices on iot.console.com, you SHOULD replace them with what you got from console
 *
 */

char PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
char PRODUCT_SECRET[IOTX_PRODUCT_SECRET_LEN + 1] = {0};
char DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
char DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

static user_example_ctx_t g_user_example_ctx;

/** Awss Status event callback */
static int user_awss_status_event_handler(int status)
{
    EXAMPLE_TRACE("Awss Status %d", status);

    return SUCCESS_RETURN;
}

/** cloud connected event callback */
static int user_connected_event_handler(void)
{
    EXAMPLE_TRACE("Cloud Connected");
    g_user_example_ctx.cloud_connected = 1;

    return 0;
}

/** cloud connect fail event callback */
static int user_connect_fail_event_handler(void)
{
    EXAMPLE_TRACE("Cloud Connect Fail");

    return SUCCESS_RETURN;
}

/** cloud disconnected event callback */
static int user_disconnected_event_handler(void)
{
    EXAMPLE_TRACE("Cloud Disconnected");
    g_user_example_ctx.cloud_connected = 0;

    return 0;
}

/** cloud raw_data arrived event callback */
static int user_rawdata_arrived_event_handler(const int devid, const unsigned char *request, const int request_len)
{
    EXAMPLE_TRACE("Cloud Rawdata Arrived");

    return 0;
}

/* device initialized event callback */
static int user_initialized(const int devid)
{
    EXAMPLE_TRACE("Device Initialized");
    g_user_example_ctx.master_initialized = 1;

    return 0;
}

/** recv property post response message from cloud **/
static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
                                           const int reply_len)
{
    EXAMPLE_TRACE("Message Post Reply Received, Message ID: %d, Code: %d, Reply: %.*s", msgid, code,
                  reply_len,
                  (reply == NULL) ? ("NULL") : (reply));
    return 0;
}

/** recv event post response message from cloud **/
static int user_trigger_event_reply_event_handler(const int devid, const int msgid, const int code, const char *eventid,
                                                  const int eventid_len, const char *message, const int message_len)
{
    EXAMPLE_TRACE("Trigger Event Reply Received, Message ID: %d, Code: %d, EventID: %.*s, Message: %.*s",
                  msgid, code,
                  eventid_len,
                  eventid, message_len, message);

    return 0;
}

static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
    int res = 0, value = 0;
    cJSON *root = NULL, *LightSwitch = NULL, *LightColor = NULL;

    ESP_LOGE(TAG, "Property Set Received, Devid: %d, Request: %s", devid, request);

    if (!request)
    {
        return NULL_VALUE_ERROR;
    }

    /* Parse Root */
    root = cJSON_Parse(request);
    if (!root)
    {
        ESP_LOGI(TAG, "JSON Parse Error");
        return FAIL_RETURN;
    }

    /** Switch Lightbulb On/Off   */
    //{"powerstate":0}

    LightSwitch = cJSON_GetObjectItem(root, "powerstate");

    if (LightSwitch)
    {

        ESP_LOGE(TAG, "Property LightSwitch->valueint: %d", LightSwitch->valueint);

        if (LightSwitch->valueint == 1)
        {
            rgb_set_pwm(0, 100, 0);
        }
        else
        {
            rgb_set_pwm(0, 0, 0);
        }
    }

    LightColor = cJSON_GetObjectItem(root, "color");
    if (LightColor)
    {
        ESP_LOGE(TAG, "Property LightColor->valueint: %d", LightColor->valueint);
        value = LightColor->valueint;

        //蓝色：255 绿色：65280 青色：65535紫色：8388736红色：16711680橙色：16753920黄色：16776960
        switch (value)
        {

        case 16711680: //红色
            rgb_set_pwm(100, 0, 0);
            break;

        case 255: //蓝色：
            rgb_set_pwm(0, 0, 100);
            break;

        case 65280: //绿色
            rgb_set_pwm(0, 100, 0);
            break;

        case 65535: //青色
            rgb_set_pwm(0, 100, 100);
            break;

        case 8388736: //紫色
            rgb_set_pwm(50, 0, 50);
            break;

        case 16753920: //橙色
            rgb_set_pwm(100, 50, 0);
            break;

        case 16776960: //黄色
            rgb_set_pwm(100, 100, 99);
            break;

        default:
            break;
        }
    }

    cJSON_Delete(root);

    res = IOT_Linkkit_Report(devid, ITM_MSG_POST_PROPERTY, (unsigned char *)request, request_len);

    return SUCCESS_RETURN;
}

static int user_property_desired_get_reply_event_handler(const char *serviceid, const int serviceid_len)
{
    ESP_LOGI(TAG, "ITE_PROPERTY_DESIRED_GET_REPLY");

    return SUCCESS_RETURN;
}

static int user_property_get_event_handler(const int devid, const char *serviceid, const int serviceid_len, char **response, int *response_len)
{
    ESP_LOGI(TAG, "Get Property Message ID: %d", devid);

    return SUCCESS_RETURN;
}

static int user_service_request_event_handler(const int devid, const char *serviceid, const int serviceid_len,
                                              const char *request, const int request_len,
                                              char **response, int *response_len)
{
    int contrastratio = 0, to_cloud = 0;
    cJSON *root = NULL, *item_transparency = NULL, *item_from_cloud = NULL;
    ESP_LOGI(TAG, "Service Request Received, Devid: %d, Service ID: %.*s, Payload: %s", devid, serviceid_len,
             serviceid,
             request);

    /* Parse Root */
    root = cJSON_Parse(request);
    if (root == NULL || !cJSON_IsObject(root))
    {
        ESP_LOGI(TAG, "JSON Parse Error");
        return -1;
    }

    if (strlen("Custom") == serviceid_len && memcmp("Custom", serviceid, serviceid_len) == 0)
    {
        /* Parse Item */
        const char *response_fmt = "{\"Contrastratio\":%d}";
        item_transparency = cJSON_GetObjectItem(root, "transparency");
        if (item_transparency == NULL || !cJSON_IsNumber(item_transparency))
        {
            cJSON_Delete(root);
            return -1;
        }
        ESP_LOGI(TAG, "transparency: %d", item_transparency->valueint);
        contrastratio = item_transparency->valueint + 1;

        /* Send Service Response To Cloud */
        *response_len = strlen(response_fmt) + 10 + 1;
        *response = malloc(*response_len);
        if (*response == NULL)
        {
            ESP_LOGW(TAG, "Memory Not Enough");
            return -1;
        }
        memset(*response, 0, *response_len);
        snprintf(*response, *response_len, response_fmt, contrastratio);
        *response_len = strlen(*response);
    }
    else if (strlen("SyncService") == serviceid_len && memcmp("SyncService", serviceid, serviceid_len) == 0)
    {
        /* Parse Item */
        const char *response_fmt = "{\"ToCloud\":%d}";
        item_from_cloud = cJSON_GetObjectItem(root, "FromCloud");
        if (item_from_cloud == NULL || !cJSON_IsNumber(item_from_cloud))
        {
            cJSON_Delete(root);
            return -1;
        }
        ESP_LOGI(TAG, "FromCloud: %d", item_from_cloud->valueint);
        to_cloud = item_from_cloud->valueint + 1;

        /* Send Service Response To Cloud */
        *response_len = strlen(response_fmt) + 10 + 1;
        *response = malloc(*response_len);
        if (*response == NULL)
        {
            ESP_LOGW(TAG, "Memory Not Enough");
            return -1;
        }
        memset(*response, 0, *response_len);
        snprintf(*response, *response_len, response_fmt, to_cloud);
        *response_len = strlen(*response);
    }
    cJSON_Delete(root);

    return 0;
}

static int user_timestamp_reply_event_handler(const char *timestamp)
{
    EXAMPLE_TRACE("Current Timestamp: %s", timestamp);

    return SUCCESS_RETURN;
}

static int user_toplist_reply_event_handler(const int devid, const int msgid, const int code, const char *eventid, const int eventid_len)
{
    EXAMPLE_TRACE("ITE_TOPOLIST_REPLY");

    return SUCCESS_RETURN;
}

static int user_permit_join_event_handler(const int devid, const int msgid, const int code, const char *eventid, const int eventid_len)
{
    EXAMPLE_TRACE("ITE_PERMIT_JOIN");

    return SUCCESS_RETURN;
}

/** fota event handler **/
static int user_fota_event_handler(int type, const char *version)
{
    char buffer[1024] = {0};
    int buffer_length = 1024;

    /* 0 - new firmware exist, query the new firmware */
    if (type == 0)
    {
        EXAMPLE_TRACE("New Firmware Version: %s", version);

        IOT_Linkkit_Query(EXAMPLE_MASTER_DEVID, ITM_MSG_QUERY_FOTA_DATA, (unsigned char *)buffer, buffer_length);
    }

    return 0;
}

/* cota event handler */
static int user_cota_event_handler(int type, const char *config_id, int config_size, const char *get_type,
                                   const char *sign, const char *sign_method, const char *url)
{
    char buffer[128] = {0};
    int buffer_length = 128;

    /* type = 0, new config exist, query the new config */
    if (type == 0)
    {
        EXAMPLE_TRACE("New Config ID: %s", config_id);
        EXAMPLE_TRACE("New Config Size: %d", config_size);
        EXAMPLE_TRACE("New Config Type: %s", get_type);
        EXAMPLE_TRACE("New Config Sign: %s", sign);
        EXAMPLE_TRACE("New Config Sign Method: %s", sign_method);
        EXAMPLE_TRACE("New Config URL: %s", url);

        IOT_Linkkit_Query(EXAMPLE_MASTER_DEVID, ITM_MSG_QUERY_COTA_DATA, (unsigned char *)buffer, buffer_length);
    }

    return 0;
}

static int user_mqtt_connect_succ_event_handler(void)
{
    EXAMPLE_TRACE("ITE_MQTT_CONNECT_SUCC");

    return SUCCESS_RETURN;
}

static void user_post_property(void)
{
    static int cnt = 0;
    int res = 0;

    char property_payload[30] = {0};
    HAL_Snprintf(property_payload, sizeof(property_payload), "{\"Counter\": %d}", cnt++);

    res = IOT_Linkkit_Report(EXAMPLE_MASTER_DEVID, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)property_payload, strlen(property_payload));

    EXAMPLE_TRACE("Post Property Message ID: %d", res);
}

static void user_post_event(void)
{
    int res = 0;
    char *event_id = "HardwareError";
    char *event_payload = "{\"ErrorCode\": 0}";

    res = IOT_Linkkit_TriggerEvent(EXAMPLE_MASTER_DEVID, event_id, strlen(event_id),
                                   event_payload, strlen(event_payload));
    EXAMPLE_TRACE("Post Event Message ID: %d", res);
}

static void user_deviceinfo_update(void)
{
    int res = 0;
    char *device_info_update = "[{\"attrKey\":\"abc\",\"attrValue\":\"hello,world\"}]";

    res = IOT_Linkkit_Report(EXAMPLE_MASTER_DEVID, ITM_MSG_DEVICEINFO_UPDATE,
                             (unsigned char *)device_info_update, strlen(device_info_update));
    EXAMPLE_TRACE("Device Info Update Message ID: %d", res);
}

static void user_deviceinfo_delete(void)
{
    int res = 0;
    char *device_info_delete = "[{\"attrKey\":\"abc\"}]";

    res = IOT_Linkkit_Report(EXAMPLE_MASTER_DEVID, ITM_MSG_DEVICEINFO_DELETE,
                             (unsigned char *)device_info_delete, strlen(device_info_delete));
    EXAMPLE_TRACE("Device Info Delete Message ID: %d", res);
}

static int linkkit_thread(void *paras)
{
    int res = 0;
    iotx_linkkit_dev_meta_info_t master_meta_info;
    int domain_type = 0, dynamic_register = 0, post_reply_need = 0;

#ifdef ATM_ENABLED
    if (IOT_ATM_Init() < 0)
    {
        EXAMPLE_TRACE("IOT ATM init failed!\n");
        return -1;
    }
#endif

    memset(&g_user_example_ctx, 0, sizeof(user_example_ctx_t));

    HAL_GetProductKey(PRODUCT_KEY);
    HAL_GetProductSecret(PRODUCT_SECRET);
    HAL_GetDeviceName(DEVICE_NAME);
    HAL_GetDeviceSecret(DEVICE_SECRET);
    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    memcpy(master_meta_info.product_key, PRODUCT_KEY, strlen(PRODUCT_KEY));
    memcpy(master_meta_info.product_secret, PRODUCT_SECRET, strlen(PRODUCT_SECRET));
    memcpy(master_meta_info.device_name, DEVICE_NAME, strlen(DEVICE_NAME));
    memcpy(master_meta_info.device_secret, DEVICE_SECRET, strlen(DEVICE_SECRET));

    /* Register Callback */
    IOT_RegisterCallback(ITE_AWSS_STATUS, user_awss_status_event_handler);
    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
    IOT_RegisterCallback(ITE_CONNECT_FAIL, user_connect_fail_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);
    IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, user_rawdata_arrived_event_handler);
    IOT_RegisterCallback(ITE_SERVICE_REQUEST, user_service_request_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_GET, user_property_get_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_DESIRED_GET_REPLY, user_property_desired_get_reply_event_handler);
    IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);
    IOT_RegisterCallback(ITE_TRIGGER_EVENT_REPLY, user_trigger_event_reply_event_handler);
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_TOPOLIST_REPLY, user_toplist_reply_event_handler);
    IOT_RegisterCallback(ITE_PERMIT_JOIN, user_permit_join_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);
    IOT_RegisterCallback(ITE_FOTA, user_fota_event_handler);
    IOT_RegisterCallback(ITE_COTA, user_cota_event_handler);
    IOT_RegisterCallback(ITE_MQTT_CONNECT_SUCC, user_mqtt_connect_succ_event_handler);

    domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Choose Login Method */
    dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* post reply doesn't need */
    post_reply_need = 1;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_reply_need);

    /* Create Master Device Resources */
    g_user_example_ctx.master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
    if (g_user_example_ctx.master_devid < 0)
    {
        EXAMPLE_TRACE("IOT_Linkkit_Open Failed\n");
        return -1;
    }

    /* Start Connect Aliyun Server */
    res = IOT_Linkkit_Connect(g_user_example_ctx.master_devid);
    if (res < 0)
    {
        EXAMPLE_TRACE("IOT_Linkkit_Connect Failed\n");
        IOT_Linkkit_Close(g_user_example_ctx.master_devid);
        return -1;
    }

    while (1)
    {
        IOT_Linkkit_Yield(EXAMPLE_YIELD_TIMEOUT_MS);
    }

    IOT_Linkkit_Close(g_user_example_ctx.master_devid);

    IOT_DumpMemoryStats(IOT_LOG_DEBUG);
    IOT_SetLogLevel(IOT_LOG_NONE);
    return 0;
}

void linkkit_main(void *paras)
{
    while (1)
    {
        linkkit_thread(NULL);
    }
}
#endif
