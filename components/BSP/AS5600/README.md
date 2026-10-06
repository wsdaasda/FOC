# AS5600 motor-1 bring-up

The driver uses the existing legacy I2C master driver. `as5600_init_default()` configures motor 1 on I2C0, SDA GPIO7, SCL GPIO15, then binds and verifies the AS5600 at 7-bit address `0x36`. For a separately managed bus, `as5600_init()` accepts an initialized `i2c_obj_t`.

The application prints STATUS magnet flags, 12-bit RAW ANGLE, and ANGLE every 250 ms. It does not write configuration or OTP registers.

Before powering the circuit, confirm SDA/SCL have pull-ups to a voltage safe for ESP32-S3 GPIOs. `iic_init()` also enables the ESP32 internal pull-ups, but external pull-ups on the board are recommended for reliable I2C.
