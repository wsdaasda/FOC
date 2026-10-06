#include "as5600.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

#define AS5600_I2C_TIMEOUT_TICKS pdMS_TO_TICKS(100)

/* Motor 1 board wiring belongs to the AS5600 device module, not app_main(). */
#define AS5600_DEFAULT_I2C_PORT I2C_NUM_0
#define AS5600_DEFAULT_I2C_SDA  GPIO_NUM_7
#define AS5600_DEFAULT_I2C_SCL  GPIO_NUM_15

static i2c_obj_t s_default_i2c;
static bool s_default_i2c_initialized;

static esp_err_t read_registers(as5600_t *sensor, uint8_t reg,
                                uint8_t *data, size_t len)
{
    if (!sensor || !sensor->i2c || sensor->i2c->init_flag != ESP_OK ||
        !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_write_read_device(sensor->i2c->port, sensor->address,
                                        &reg, 1, data, len,
                                        AS5600_I2C_TIMEOUT_TICKS);
}

esp_err_t as5600_init(as5600_t *sensor, i2c_obj_t *i2c)
{
    if (!sensor || !i2c || i2c->init_flag != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    sensor->i2c = i2c;
    sensor->address = AS5600_I2C_ADDR;

    uint8_t status;
    return read_registers(sensor, AS5600_REG_STATUS, &status, sizeof(status));
}

esp_err_t as5600_init_default(as5600_t *sensor)
{
    if (!sensor) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_default_i2c_initialized) {
        s_default_i2c = iic_init(AS5600_DEFAULT_I2C_PORT,
                                 AS5600_DEFAULT_I2C_SDA,
                                 AS5600_DEFAULT_I2C_SCL);
        s_default_i2c_initialized = true;
    }

    if (s_default_i2c.init_flag != ESP_OK) {
        return s_default_i2c.init_flag;
    }

    return as5600_init(sensor, &s_default_i2c);
}

esp_err_t as5600_read_status(as5600_t *sensor, as5600_status_t *status)
{
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t value;
    esp_err_t err = read_registers(sensor, AS5600_REG_STATUS, &value, sizeof(value));
    if (err != ESP_OK) {
        return err;
    }

    status->magnet_detected = (value & (1U << 5)) != 0;
    status->magnet_too_weak = (value & (1U << 4)) != 0;
    status->magnet_too_strong = (value & (1U << 3)) != 0;
    return ESP_OK;
}

static esp_err_t read_angle_register(as5600_t *sensor, uint8_t reg, uint16_t *angle)
{
    if (!angle) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t bytes[2];
    esp_err_t err = read_registers(sensor, reg, bytes, sizeof(bytes));
    if (err != ESP_OK) {
        return err;
    }

    *angle = (((uint16_t)bytes[0] << 8) | bytes[1]) & 0x0FFF;
    return ESP_OK;
}

esp_err_t as5600_read_raw_angle(as5600_t *sensor, uint16_t *raw_angle)
{
    return read_angle_register(sensor, AS5600_REG_RAW_ANGLE_H, raw_angle);
}

esp_err_t as5600_read_angle(as5600_t *sensor, uint16_t *angle)
{
    return read_angle_register(sensor, AS5600_REG_ANGLE_H, angle);
}

float as5600_angle_to_degrees(uint16_t angle)
{
    return ((float)(angle & 0x0FFF) * 360.0f) / 4096.0f;
}
