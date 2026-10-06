#include "wifi_sta.h"

#include <string.h>

#include "freertos/event_groups.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"

#define WIFI_STA_CONNECTED_BIT BIT0
#define WIFI_STA_FAILED_BIT    BIT1

static const char *TAG = "wifi_sta";
static EventGroupHandle_t s_event_group;
static esp_netif_t *s_netif;
static foc_wifi_sta_config_t s_config;
static uint8_t s_retry_count;
static bool s_started;

static void wifi_sta_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *event = event_data;
        xEventGroupClearBits(s_event_group, WIFI_STA_CONNECTED_BIT);
        if (s_config.on_disconnected) {
            s_config.on_disconnected(event->reason, s_config.user_ctx);
        }

        if (s_config.max_retry == 0 || s_retry_count < s_config.max_retry) {
            s_retry_count++;
            ESP_LOGW(TAG, "disconnected (reason=%u), reconnecting (%u)",
                     event->reason, s_retry_count);
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
        } else {
            ESP_LOGE(TAG, "connection failed after %u retries", s_retry_count);
            xEventGroupSetBits(s_event_group, WIFI_STA_FAILED_BIT);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = event_data;
        s_retry_count = 0;
        xEventGroupClearBits(s_event_group, WIFI_STA_FAILED_BIT);
        xEventGroupSetBits(s_event_group, WIFI_STA_CONNECTED_BIT);
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        if (s_config.on_connected) {
            s_config.on_connected(&event->ip_info, s_config.user_ctx);
        }
    }
}

static esp_err_t init_network_stack(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    return ESP_OK;
}

esp_err_t wifi_sta_start(const foc_wifi_sta_config_t *config)
{
    if (!config || !config->ssid || !config->password ||
        strlen(config->ssid) > WIFI_STA_SSID_MAX_LEN ||
        strlen(config->password) > WIFI_STA_PASSWORD_MAX_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_started) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_RETURN_ON_ERROR(init_network_stack(), TAG, "network stack init failed");
    s_event_group = xEventGroupCreate();
    if (!s_event_group) {
        return ESP_ERR_NO_MEM;
    }

    s_config = *config;
    s_netif = esp_netif_create_default_wifi_sta();
    if (!s_netif) {
        vEventGroupDelete(s_event_group);
        s_event_group = NULL;
        return ESP_FAIL;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG, "Wi-Fi driver init failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   wifi_sta_event_handler, NULL), TAG,
                        "Wi-Fi event registration failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                   wifi_sta_event_handler, NULL), TAG,
                        "IP event registration failed");

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, config->ssid, strlen(config->ssid));
    memcpy(wifi_config.sta.password, config->password, strlen(config->password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "mode setup failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG,
                        "credential setup failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Wi-Fi start failed");
    s_started = true;
    ESP_LOGI(TAG, "station started for SSID: %s", config->ssid);
    return ESP_OK;
}

esp_err_t wifi_sta_wait_connected(TickType_t timeout_ticks)
{
    if (!s_started) {
        return ESP_ERR_INVALID_STATE;
    }
    EventBits_t bits = xEventGroupWaitBits(s_event_group,
        WIFI_STA_CONNECTED_BIT | WIFI_STA_FAILED_BIT, pdFALSE, pdFALSE, timeout_ticks);
    if (bits & WIFI_STA_CONNECTED_BIT) {
        return ESP_OK;
    }
    return (bits & WIFI_STA_FAILED_BIT) ? ESP_FAIL : ESP_ERR_TIMEOUT;
}

esp_err_t wifi_sta_reconnect(void)
{
    if (!s_started) {
        return ESP_ERR_INVALID_STATE;
    }
    s_retry_count = 0;
    xEventGroupClearBits(s_event_group, WIFI_STA_FAILED_BIT);
    return esp_wifi_connect();
}

bool wifi_sta_is_connected(void)
{
    return s_event_group && (xEventGroupGetBits(s_event_group) & WIFI_STA_CONNECTED_BIT);
}

esp_netif_t *wifi_sta_get_netif(void)
{
    return s_netif;
}
