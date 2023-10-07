# ESP32-MAX30102 Heart Rate and SpO2 Monitor

## Overview

This repository hosts an embedded C project developed for the ESP32 platform, utilizing the MAX30102 sensor for acquiring and monitoring heart rate and SpO2 (blood oxygen saturation) levels in real-time.

## Features

- **I2C Communication**: Initialize and communicate with the MAX30102 sensor over I2C.
- **Sensor Initialization**: Set the MAX30102 sensor with specific configurations suitable for obtaining PPG signals.
- **Real-Time Data Acquisition**: Continuously fetch data from the sensor and process it in real-time.
- **Data Processing**: Implement a filter for the IR and RED led values, derive the plethysmograph, and find peaks for calculating the heart rate and SpO2.
- **Health Monitoring**: Calculate and display heart rate and SpO2 levels continuously on the standard output.

## Code Structure

- **I2C Initialization and Handlers**: Handles the setup and communication over the I2C bus with the MAX30102 sensor.
- **Sensor Configuration**: Initializes the MAX30102 sensor with predetermined configurations to enable the measurement of IR and RED led values.
- **Data Processing and Health Parameter Calculation**: Fetches raw data from the MAX30102, applies filtering to the signals, identifies peaks, calculates heart rate and SpO2 levels, and prints them on the standard output.
- **Application Entry Point**: Initializes the necessary drivers and creates a FreeRTOS task to handle MAX30102 data processing and display.
