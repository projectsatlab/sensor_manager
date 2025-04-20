// File header file meteo_sensor_lib.c con i prototipi delle diverse funzioni

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <curl/curl.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>

int i2cCommOpen(uint8_t address);
uint16_t sht21I2cRead(int fd, uint8_t command);
float sht21Rh2Real(uint16_t rawRh);
float sht21Temp2Real(uint16_t rawTemp);
int remoteSensorGet(char *sensorType, char *response);
int apiPostServerWrite(const char *_token, const char *_deviceID, const char *_sensorData);
