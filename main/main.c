#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "i2c-driver.h"
#include "max30102.h"


void app_main(void)
{

// Initialize NVS for storing data across reboots
ESP_ERROR_CHECK( nvs_flash_init() );

i2c_init();

// Initialize MAX30102 Pulse Oximeter sensor
max30102_init();

// Create a FreeRTOS task for the MAX30102 sensor
xTaskCreate(max30102_task, "max30102_task", 4096, NULL, 5, NULL);

}