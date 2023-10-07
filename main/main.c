#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_FREQ_HZ       100000
#define I2C_MASTER_SDA_IO        21  // Choose your SDA pin
#define I2C_MASTER_SCL_IO        5   // Choose your SCL pin
#define I2C_ADDR_MAX30102 0x57

float heartrate=99.2, pctspo2=99.2;   
int irpower = 0, rpower = 0, lirpower = 0, lrpower = 0;
float meastime;
int countedsamples = 0;
int startstop = 0, raworbp = 0;
char outStr[1500];

void i2c_init() {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

esp_err_t i2c_write(uint8_t addr, uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, size_t data_len) {
    if (data_len == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if (data_len > 1) {
        i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void max30102_init() {
    uint8_t data;
    data = ( 0x2 << 5);  //sample averaging 0=1,1=2,2=4,3=8,4=16,5+=32
    i2c_write(I2C_ADDR_MAX30102, 0x08, data);
    data = 0x03;                //mode = red and ir samples
    i2c_write(I2C_ADDR_MAX30102, 0x09, data);
    data = ( 0x3 << 5) + ( 0x3 << 2 ) + 0x3; //first and last 0x3, middle smap rate 0=50,1=100,etc 
    i2c_write(I2C_ADDR_MAX30102, 0x0a, data);
    data = 0xd0;                //ir pulse power
    i2c_write(I2C_ADDR_MAX30102, 0x0c, data);
    data = 0xa0;                //red pulse power
    i2c_write(I2C_ADDR_MAX30102, 0x0d, data);
}

void max30102_task () {
    int cnt, samp, tcnt = 0, cntr = 0;
    uint8_t rptr, wptr;
    uint8_t data;
    uint8_t regdata[256];
    //int irmeas, redmeas;
    float firxv[5], firyv[5], fredxv[5], fredyv[5];
    float lastmeastime = 0;
    float hrarray[5],spo2array[5];
    int hrarraycnt = 0;
    while(1){
        if(lirpower!=irpower){
            data = (uint8_t) irpower;
            i2c_write(I2C_ADDR_MAX30102, 0x0d,  data); 
            lirpower=irpower;
        }
        if(lrpower!=rpower){
            data = (uint8_t) rpower;
            i2c_write(I2C_ADDR_MAX30102, 0x0c,  data); 
            lrpower=rpower;
        }
        i2c_read(I2C_ADDR_MAX30102, 0x04, &wptr, 1);
        i2c_read(I2C_ADDR_MAX30102, 0x06, &rptr, 1);
        
        samp = ((32+wptr)-rptr)%32;
        i2c_read(I2C_ADDR_MAX30102, 0x07, regdata, 6*samp);
        //ESP_LOGI("samp","----  %d %d %d %d",  adc_read_ptr,samp,wptr,rptr);

        for(cnt = 0; cnt < samp; cnt++){
            meastime =  0.01 * tcnt++;
            firxv[0] = firxv[1]; firxv[1] = firxv[2]; firxv[2] = firxv[3]; firxv[3] = firxv[4];
            firxv[4] = (1/3.48311) * (256*256*(regdata[6*cnt+3]%4)+ 256*regdata[6*cnt+4]+regdata[6*cnt+5]);
            firyv[0] = firyv[1]; firyv[1] = firyv[2]; firyv[2] = firyv[3]; firyv[3] = firyv[4];
            firyv[4] = (firxv[0] + firxv[4]) - 2 * firxv[2]
                    + ( -0.1718123813 * firyv[0]) + (  0.3686645260 * firyv[1])
                    + ( -1.1718123813 * firyv[2]) + (  1.9738037992 * firyv[3]);

            fredxv[0] = fredxv[1]; fredxv[1] = fredxv[2]; fredxv[2] = fredxv[3]; fredxv[3] = fredxv[4];
            fredxv[4] = (1/3.48311) * (256*256*(regdata[6*cnt+0]%4)+ 256*regdata[6*cnt+1]+regdata[6*cnt+2]);
            fredyv[0] = fredyv[1]; fredyv[1] = fredyv[2]; fredyv[2] = fredyv[3]; fredyv[3] = fredyv[4];
            fredyv[4] = (fredxv[0] + fredxv[4]) - 2 * fredxv[2]
                    + ( -0.1718123813 * fredyv[0]) + (  0.3686645260 * fredyv[1])
                    + ( -1.1718123813 * fredyv[2]) + (  1.9738037992 * fredyv[3]);

            //if (-1.0 * firyv[4] >= 100 && -1.0 * firyv[3] < 100){
            //   heartrate = 60 / (meastime - lastmeastime);
            //   pctspo2 = 110 - 25 * ((fredyv[4]/fredxv[4]) / (firyv[4]/firxv[4]));
            //   printf ("%6.2f  %4.2f     hr= %5.1f     spo2= %5.1f\n", meastime, meastime - lastmeastime, heartrate, pctspo2);
            //   lastmeastime = meastime;
            //}

            //printf("%8.1f %8.1f %8.1f %8.3f\n", -1.0 * firyv[0], -1.0 * firyv[2], -1.0 * firyv[4], meastime-lastmeastime); 
            if (-1.0 * firyv[4] >= 100 && -1.0 * firyv[2] > -1*firyv[0] && -1.0 * firyv[2] > -1*firyv[4] && meastime-lastmeastime > 0.5){
               hrarray[hrarraycnt % 5] = 60 / (meastime - lastmeastime);
               spo2array[hrarraycnt % 5] = 110 - 25 * ((fredyv[4]/fredxv[4]) / (firyv[4]/firxv[4]));
               if(spo2array[hrarraycnt % 5] > 100)spo2array[hrarraycnt % 5]=99.9;
               printf ("%6.2f  %4.2f     hr= %5.1f     spo2= %5.1f\n", meastime, meastime - lastmeastime, heartrate, pctspo2);
               lastmeastime = meastime;
	           hrarraycnt++;
	           heartrate = (hrarray[0]+hrarray[1]+hrarray[2]+hrarray[3]+hrarray[4]) / 5;
	           if (heartrate < 40 || heartrate > 150) heartrate = 0;
	           pctspo2 = (spo2array[0]+spo2array[1]+spo2array[2]+spo2array[3]+spo2array[4]) / 5;
	           if (pctspo2 < 50 || pctspo2 > 101) pctspo2 = 0;
            }
            
            char tmp[32];
            countedsamples++;
            if(countedsamples < 100){
                if(raworbp == 0){
                    snprintf(tmp, sizeof tmp, "%5.1f,%5.1f,", -1* fredyv[4], -1* firyv[4] ); 
                } else {
                    snprintf(tmp, sizeof tmp, "%5.1f,%5.1f,", fredxv[4], firxv[4] ); 
                }
                strcat (outStr, tmp);  
            }
        }
    }
}



void app_main(void)
{

ESP_ERROR_CHECK( nvs_flash_init() );

i2c_init();
//i2cdetect();

max30102_init();

xTaskCreate(max30102_task, "max30102_task", 4096, NULL, 5, NULL);

}