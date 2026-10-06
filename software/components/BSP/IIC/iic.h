#ifndef __IIC_H
#define __IIC_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"


/* IIC控制块：描述一条IIC总线的所有硬件信息，初始化完成后当作"总线句柄"传给外设驱动使用 */
typedef struct _i2c_obj_t {
    i2c_port_t port;      /* 使用哪个硬件IIC控制器：I2C_NUM_0 或 I2C_NUM_1 */
    gpio_num_t scl;       /* SCL时钟线接的GPIO编号，例如 GPIO_NUM_42 */
    gpio_num_t sda;       /* SDA数据线接的GPIO编号，例如 GPIO_NUM_41 */
    esp_err_t init_flag;  /* 初始化结果：ESP_OK=成功，ESP_FAIL=失败 */
} i2c_obj_t;

/* 数据段描述：描述一段连续数据的位置和长度，多个buf组合起来可以一次发送不连续的数据 */
typedef struct _i2c_buf_t {
    size_t len;     /* 这段数据有多少字节 */
    uint8_t *buf;   /* 指向这段数据的指针（存放地址，不是数据本身） */
} i2c_buf_t;

extern i2c_obj_t iic_master[I2C_NUM_MAX];

/* 读写标志位 */
#define I2C_FLAG_READ                   (0x01)                                                          /* 读标志 */
#define I2C_FLAG_STOP                   (0x02)                                                          /* 停止标志 */
#define I2C_FLAG_WRITE                  (0x04)                                                          /* 写标志 */

/* 总线参数 */
#define IIC_FREQ                        400000                                                          /* IIC通信频率 */
#define I2C_MASTER_TX_BUF_DISABLE       0                                                               /* I2C主机不需要缓冲区 */
#define I2C_MASTER_RX_BUF_DISABLE       0                                                               /* I2C主机不需要缓冲区 */
#define ACK_CHECK_EN                    0x1                                                             /* I2C master将从slave检查ACK */

/* 函数声明 */
i2c_obj_t iic_init(i2c_port_t iic_port, gpio_num_t sda, gpio_num_t scl);                                /* 初始化指定引脚的IIC */
esp_err_t i2c_transfer(i2c_obj_t *self, uint16_t addr, size_t n, i2c_buf_t *bufs, unsigned int flags);  /* IIC读写数据 */

#endif
