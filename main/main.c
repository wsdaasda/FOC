#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "as5600.h"

static const char *TAG = "as5600_motor1_test";

void app_main(void)
{
    as5600_t motor1_encoder = {0};
    esp_err_t err = as5600_init_default(&motor1_encoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AS5600 init failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "AS5600 motor 1 ready (addr=0x%02X)", AS5600_I2C_ADDR);

    while (1) {
        as5600_status_t status;
        uint16_t raw_angle;
        uint16_t angle;

        err = as5600_read_status(&motor1_encoder, &status);
        if (err == ESP_OK) {
            err = as5600_read_raw_angle(&motor1_encoder, &raw_angle);
        }
        if (err == ESP_OK) {
            err = as5600_read_angle(&motor1_encoder, &angle);
        }

        if (err == ESP_OK) {
            ESP_LOGI(TAG,
                     "magnet=%s weak=%s strong=%s raw=%u (%.2f deg) angle=%u (%.2f deg)",
                     status.magnet_detected ? "yes" : "no",
                     status.magnet_too_weak ? "yes" : "no",
                     status.magnet_too_strong ? "yes" : "no",
                     raw_angle, as5600_angle_to_degrees(raw_angle),
                     angle, as5600_angle_to_degrees(angle));
        } else {
            ESP_LOGW(TAG, "AS5600 read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
