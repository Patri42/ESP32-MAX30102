#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

// Define master I2C settings
#define I2C_MASTER_NUM        I2C_NUM_0     // I2C port number
#define I2C_MASTER_FREQ_HZ    100000        // I2C master clock frequency
#define I2C_MASTER_SDA_IO     21            // GPIO number for I2C master data 
#define I2C_MASTER_SCL_IO     5             // GPIO number for I2C master clock

void i2c_init(void); // Initialize I2C communication as master
esp_err_t i2c_write(uint8_t addr, uint8_t reg, uint8_t data); // Write byte data to a specific I2C address and register
esp_err_t i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, size_t data_len); // Read byte data from a specific I2C address and register

#endif // I2C_DRIVER_H