#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "i2c-driver.h"
#include "max30102.h"


void app_main(void)
{

ESP_ERROR_CHECK( nvs_flash_init() );

i2c_init();

max30102_init();

xTaskCreate(max30102_task, "max30102_task", 4096, NULL, 5, NULL);

}