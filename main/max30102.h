#ifndef MAX30102_H
#define MAX30102_H

// Define the I2C address for MAX30102 sensor
#define I2C_ADDR_MAX30102 0x57

void max30102_init(void); // Declare function for initializing MAX30102 sensor
void max30102_task(void); // Declare function to handle MAX30102 task


#endif // MAX30102_H