# 可移植 Wi-Fi STA 组件

来源：ESP-IDF 5.4 官方 `examples/wifi/getting_started/station`。

本目录把官方示例的 Wi-Fi STA 初始化、事件处理、自动重连抽成组件；它不包含 `app_main()`、不持有固定 SSID/密码，也不操作 IIC、PWM、ADC 或电机驱动。

## 加入新工程

复制整个 `components/Middlewares` 目录，并在项目根 `CMakeLists.txt` 中保留：

```cmake
set(EXTRA_COMPONENT_DIRS components/Middlewares)
```

应用必须先完成 NVS 初始化。最小调用：

```c
#include "nvs_flash.h"
#include "wifi_sta.h"

static void on_wifi_connected(const esp_netif_ip_info_t *ip, void *ctx)
{
    /* 在这里启动 HTTP、MQTT 或 UDP；不要在此调用 FOC 电流环。 */
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    const foc_wifi_sta_config_t wifi = {
        .ssid = "your-ssid",
        .password = "your-password",
        .max_retry = 5,
        .on_connected = on_wifi_connected,
    };
    ESP_ERROR_CHECK(wifi_sta_start(&wifi));
}
```

`wifi_sta_start()` 不阻塞。若开机必须联网再启动某业务，调用
`wifi_sta_wait_connected(pdMS_TO_TICKS(15000))`。

不要把密码写进可提交的源文件。开发阶段可放在未跟踪的 `wifi_secret.h`；产品阶段应使用 BLE/SoftAP 配网，把凭据保存到 NVS。
