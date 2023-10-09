#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_FREQ_HZ    100000
#define I2C_MASTER_SDA_IO     21
#define I2C_MASTER_SCL_IO     5

void i2c_init(void);
esp_err_t i2c_write(uint8_t addr, uint8_t reg, uint8_t data);
esp_err_t i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, size_t data_len);

#endif // I2C_DRIVER_H