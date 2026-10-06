#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "iic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AS5600_I2C_ADDR             0x36
#define AS5600_REG_STATUS           0x0B
#define AS5600_REG_RAW_ANGLE_H      0x0C
#define AS5600_REG_ANGLE_H          0x0E

typedef struct {
    bool magnet_detected;
    bool magnet_too_weak;
    bool magnet_too_strong;
} as5600_status_t;

typedef struct {
    i2c_obj_t *i2c;
    uint8_t address;
} as5600_t;

/** Bind an AS5600 to an already initialized I2C bus and verify communication. */
esp_err_t as5600_init(as5600_t *sensor, i2c_obj_t *i2c);

/** Initialize the board's motor-1 I2C bus, then bind and verify the AS5600. */
esp_err_t as5600_init_default(as5600_t *sensor);

/** Read the STATUS register and decode magnet-detection flags. */
esp_err_t as5600_read_status(as5600_t *sensor, as5600_status_t *status);

/** Read the unmodified 12-bit RAW ANGLE value (0..4095). */
esp_err_t as5600_read_raw_angle(as5600_t *sensor, uint16_t *raw_angle);

/** Read the scaled ANGLE value (0..4095). */
esp_err_t as5600_read_angle(as5600_t *sensor, uint16_t *angle);

/** Convert a 12-bit angle value to degrees. */
float as5600_angle_to_degrees(uint16_t angle);

#ifdef __cplusplus
}
#endif
