#include <string.h>
#include "max30102.h"
#include "i2c-driver.h"
#include "esp_log.h"

// Put the max30102_init and max30102_task functions here from your initial code

#define I2C_ADDR_MAX30102 0x57

float heartrate=99.2, pctspo2=99.2;   
int irpower = 0, rpower = 0, lirpower = 0, lrpower = 0;
float meastime;
int countedsamples = 0;
int startstop = 0, raworbp = 0;
char outStr[1500];

void max30102_init() {
    uint8_t data;
    data = ( 0x2 << 5);         // Set sample averaging 0=1,1=2,2=4,3=8,4=16,5+=32
    i2c_write(I2C_ADDR_MAX30102, 0x08, data);
    data = 0x03;                // Set mode to red and IR samples
    i2c_write(I2C_ADDR_MAX30102, 0x09, data);
    data = ( 0x3 << 5) + ( 0x3 << 2 ) + 0x3; // Set ADC range, sample rate, and pulse width
    i2c_write(I2C_ADDR_MAX30102, 0x0a, data);
    data = 0xd0;                // Set IR pulse power
    i2c_write(I2C_ADDR_MAX30102, 0x0c, data);
    data = 0xa0;                // Set red pulse power
    i2c_write(I2C_ADDR_MAX30102, 0x0d, data);
}

void max30102_task () {
    int cnt, samp, tcnt = 0;
    uint8_t rptr, wptr;
    uint8_t data;
    uint8_t regdata[256];
    //int irmeas, redmeas;
    float firxv[5], firyv[5], fredxv[5], fredyv[5];
    float lastmeastime = 0;
    float hrarray[5],spo2array[5];
    int hrarraycnt = 0;
    while(1){
        // Update LED pulse amplitude if needed
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

        // Reading FIFO data pointers
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

            // Compute and display heart rate and SpO2 using detected peaks
            if (-1.0 * firyv[4] >= 100 && -1.0 * firyv[2] > -1*firyv[0] && -1.0 * firyv[2] > -1*firyv[4] && meastime-lastmeastime > 0.5){
                hrarray[hrarraycnt % 5] = 60 / (meastime - lastmeastime);
                spo2array[hrarraycnt % 5] = 110 - 25 * ((fredyv[4]/fredxv[4]) / (firyv[4]/firxv[4]));

                // Limit SpO2 to 100
                if(spo2array[hrarraycnt % 5] > 100)spo2array[hrarraycnt % 5]=99.9;

                // Displaying the intermediate result
                printf ("%6.2f  %4.2f     hr= %5.1f     spo2= %5.1f\n", meastime, meastime - lastmeastime, heartrate, pctspo2);
                lastmeastime = meastime;
	            hrarraycnt++;

                // Averaging recent 5 HR and SpO2 values and ensuring they are within plausible range
	            heartrate = (hrarray[0]+hrarray[1]+hrarray[2]+hrarray[3]+hrarray[4]) / 5;              
	            if (heartrate < 40 || heartrate > 150) heartrate = 0;
	            pctspo2 = (spo2array[0]+spo2array[1]+spo2array[2]+spo2array[3]+spo2array[4]) / 5;

	            if (pctspo2 < 50 || pctspo2 > 101) pctspo2 = 0;
            }
                // Output string
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