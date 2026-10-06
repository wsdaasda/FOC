/**
 ****************************************************************************************************
 * @file        iic.c
 * @author      正点原子团队(ALIENTEK) and  CXH 
 * @version     V1.0
 * @date        2026-03-12
 * @brief       IIC驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 ESP32-S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 写操作流程 (flags = WRITE | STOP)

    时序：START → [地址+W] → ACK → [寄存器地址] → ACK → [数据] → ACK → STOP

读操作流程（flags = WRITE | READ | STOP）

    时序：START → [地址+W] → ACK → [寄存器地址] → ACK
          → ReSTART → [地址+R] → ACK → [数据] → NACK → STOP

写+读（flags = WRITE | READ | STOP）
第一步 if(WRITE) 执行：
   START → [addr+W] → [bufs[0]寄存器地址]
   --n; ++bufs

第二步 无条件执行：
   ReSTART → [addr+R]  ← 最低位=1（因为有READ标志）

第三步 for循环：
   走 if 分支 → i2c_master_read → 读数据存入 bufs[1]

第四步 STOP


#define I2C_FLAG_READ                   (0x01)                                                         
#define I2C_FLAG_STOP                   (0x02)                                                          
#define I2C_FLAG_WRITE                  (0x04)                                                          


纯写    ：i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs,  I2C_FLAG_STOP)
写加读：i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, I2C_FLAG_WRITE | I2C_FLAG_READ | I2C_FLAG_STOP)
纯读：  i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs,  I2C_FLAG_READ | I2C_FLAG_STOP)

 */

#include "iic.h"


i2c_obj_t iic_master[I2C_NUM_MAX];  /* 为IIC0和IIC1分别定义IIC控制块结构体 */

/**
 * @brief       初始化IIC
 * @param       iic_port：I2C编号(I2C_NUM_0 / I2C_NUM_1)
 * @retval      IIC控制块0 / IIC控制块1
 */
i2c_obj_t iic_init(i2c_port_t iic_port, gpio_num_t sda, gpio_num_t scl)
{
    i2c_obj_t result = {
        .port = iic_port,
        .sda = sda,
        .scl = scl,
        .init_flag = ESP_ERR_INVALID_ARG,
    };
    i2c_config_t config = {0};

    if (iic_port < I2C_NUM_0 || iic_port >= I2C_NUM_MAX ||
        sda < 0 || scl < 0 || sda == scl) {
        return result;
    }

    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = sda;
    config.scl_io_num = scl;
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = IIC_FREQ;

    result.init_flag = i2c_param_config(iic_port, &config);
    if (result.init_flag == ESP_OK) {
        result.init_flag = i2c_driver_install(iic_port, config.mode,
                                              I2C_MASTER_RX_BUF_DISABLE,
                                              I2C_MASTER_TX_BUF_DISABLE, 0);
    }

    if (result.init_flag == ESP_OK) {
        iic_master[iic_port] = result;
    }
    return result;
}

/**
 * @brief       IIC读写数据
 * @param       self：设备控制块
 * @param       addr：设备地址
 * @param       n   ：数据大小
 * @param       bufs：要发送的数据或者是读取的存储区
 * @param       flags：读写标志位
 * @retval      无
 */
esp_err_t i2c_transfer(i2c_obj_t *self, uint16_t addr, size_t n, i2c_buf_t *bufs, unsigned int flags)
{
    int data_len = 0;
    esp_err_t ret = ESP_FAIL;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                                                       /* 创建一个命令链接,将一系列待发送给从机的数据填充命令链接 */

    /* 根据器件通信时序去决定flags参数,进而选择如下代码不同的执行情况 */

    if (flags & I2C_FLAG_WRITE)
    {

           /* START → [从机地址+W] → ACK → [寄存器地址] → ACK → STOP
                1         2                   3
             */

        i2c_master_start(cmd);                                                                          /* 1 塞入 START  启动位 */
        i2c_master_write_byte(cmd, addr << 1, ACK_CHECK_EN);                                            /* 2 塞入 [从机地址+W]   */
        i2c_master_write(cmd, bufs->buf, bufs->len, ACK_CHECK_EN);                                      /* 3 塞入 bufs[0] 的数据 */
        data_len += bufs->len; 
        --n;
        ++bufs;
    }

    i2c_master_start(cmd);                                                                              /* 启动位 */
    i2c_master_write_byte(cmd, addr << 1 | (flags & I2C_FLAG_READ), ACK_CHECK_EN);                      /* 从机地址 + 读/写操作位 */

    for (; n--; ++bufs)
    {
        if (flags & I2C_FLAG_READ)
        {
            i2c_master_read(cmd, bufs->buf, bufs->len, n == 0 ? I2C_MASTER_LAST_NACK : I2C_MASTER_ACK); /* 读取数据 */
        }
        else
        {
            if (bufs->len != 0)
            {
                i2c_master_write(cmd, bufs->buf, bufs->len, ACK_CHECK_EN);                              /* len个数据 */
            }
        }
        data_len += bufs->len;
    }

    if (flags & I2C_FLAG_STOP)
    {
        i2c_master_stop(cmd);                                                                           /* 停止位 */
    }

    ret = i2c_master_cmd_begin(self->port, cmd, 100 * (1 + data_len) / portTICK_PERIOD_MS);             /* 触发I2C控制器执行命令链接,即命令发送 */
    i2c_cmd_link_delete(cmd);                                                                           /* 释放命令链接使用的资源 */

    return ret;
}
