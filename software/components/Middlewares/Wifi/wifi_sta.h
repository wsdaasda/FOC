#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_STA_SSID_MAX_LEN       32
#define WIFI_STA_PASSWORD_MAX_LEN   64

typedef void (*wifi_sta_connected_cb_t)(const esp_netif_ip_info_t *ip_info,
                                        void *user_ctx);
typedef void (*wifi_sta_disconnected_cb_t)(uint8_t reason, void *user_ctx);

/*
 * Keep credentials and application callbacks in main.c, not in this component.
 * Wi-Fi credentials must not be committed to a reusable driver component.
 */
typedef struct {
    const char *ssid;
    const char *password;
    uint8_t max_retry; /* 0 means retry indefinitely. */
    wifi_sta_connected_cb_t on_connected;
    wifi_sta_disconnected_cb_t on_disconnected;
    void *user_ctx;
} foc_wifi_sta_config_t;

/**
 * Start Wi-Fi in station mode. NVS must have been initialized first.
 * This call returns after the Wi-Fi driver starts; use wifi_sta_wait_connected()
 * when an initial IP address is required before continuing.
 */
esp_err_t wifi_sta_start(const foc_wifi_sta_config_t *config);

/** Wait for an IPv4 address or for all configured retries to fail. */
esp_err_t wifi_sta_wait_connected(TickType_t timeout_ticks);

/** Clear retry state and request a new connection attempt. */
esp_err_t wifi_sta_reconnect(void);

bool wifi_sta_is_connected(void);
esp_netif_t *wifi_sta_get_netif(void);

#ifdef __cplusplus
}
#endif
