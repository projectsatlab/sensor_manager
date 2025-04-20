/*
=========================================
   Filename:	meteo-sensor-main.c
   Author:	Luca Olearo	
   Version:	1.0	
   Last edit:	12/04/2025
   Description:
Read temperature and Humidity data
from SHT21 sensor and save it in raw
then report data in real numbers.
Sampling and Showing rate are passed
through command line.
==========================================
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "./JSON/json_loader.h" //permette di leggere i JSON semplici

#include "./sensor_manager_lib.h"

//#define REMOTE_SENSOR

// usare json_loader: readjson("nomechiave", file/buffer)
// compilare con: cc sensor_manager_main.c sensor_manager_lib.c ./JSON/json_loader.c -lwiringPi -lcurl 

int fd;					//made it global so i can use it in every function
uint8_t elements = 0;			//made it global so i can use it in every function

//Data structure named samples
struct samples {
	uint32_t timestamp;
	uint16_t rawTempValue;
	uint16_t rawHumidityValue;
};

void sht21GetValues(time_t callTime, struct samples *itemPtr) {

	//append data to pointed data structure
	itemPtr->timestamp = callTime;
	itemPtr->rawTempValue = sht21I2cRead(fd, 0xE3);
	itemPtr->rawHumidityValue = sht21I2cRead(fd, 0xE5);
}

void sht21DisplayValues(struct samples *itemPtr, const char *_token, const char *_deviceId) {

	printf("\n\n****************************\n   Riepilogo campionamenti  \n*****************************");

	float rhMean = 0;
	float tempMean = 0;

	uint16_t rawTempMean = 0;
	uint16_t rawHumMean = 0;

	float realRh = 0;
	float realTemp = 0;

	char payloadBuffer[33];

	//converting from Unix Time to normal date
	char dateBuffer[80]; //buffer to be able to convert

		for(uint8_t i = 0; i < elements; i++) {

			time_t epoch_time = (time_t)itemPtr->timestamp;
			struct tm *local_time = localtime(&epoch_time);
			strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y - %H:%M:%S", local_time); //converting to string in giorno-mese-anno/ore-minuti-secondi


			rawTempMean += itemPtr->rawTempValue;
			rawHumMean += itemPtr->rawHumidityValue;

			realTemp = sht21Temp2Real(itemPtr->rawTempValue);
			tempMean += realTemp;

			realRh = sht21Rh2Real(itemPtr->rawHumidityValue);
			rhMean += realRh;

			printf("\n===[Campionamento: %d]===\nTimestamp: %s", i + 1, dateBuffer);
			printf("\nTemperatura registrata: %.2f°C", realTemp);
			printf("\nUmidità relativa registrata: %.2f%%", realRh);
			fflush(stdout);		//print out any buffered text
			itemPtr++;
		}
	itemPtr--;
	sprintf(payloadBuffer, "%08lX80%04X%04X000000000000000", itemPtr->timestamp, rawTempMean / elements, rawHumMean / elements);

	printf("\n\nTemperatura media: %.2f°C", tempMean / elements);
	printf("\nUmidità relativa media: %.2f%%", rhMean / elements);
	printf("\nPayload effettivo in HEX: %s\n\n", payloadBuffer);
	fflush(stdout);
	apiPostServerWrite(_token, _deviceId, payloadBuffer);
}

int sht21RemoteSensorDecode(char *buffer, struct samples *itemPtr) {

	if (!strcmp(readJson("type", buffer), "SHT21")) {
		itemPtr->rawTempValue = atoi(readJson("temp", buffer));
		itemPtr->rawHumidityValue = atoi(readJson("rh", buffer));
	}

}

int main (int argc, char* argv[]) {

const char *writeTkn = readJson("API_TOKEN", NULL);
const char *deviceId = readJson("API_DEVICE_ID", NULL);

char buffer[1024];

printf("\nSensor manager APP\n");
printf("\nIl tuo token e': %s\n", writeTkn);
printf("Il tuo deviceId e': %s\n", deviceId);

	if (argc != 3) {
		printf("\nERRORE! | Non sono stati correttamente inseriti i parametri\n");
		return(0);
	}

uint16_t samplingRate = atoi(argv[1]);
uint16_t readingRate = atoi(argv[2]);

	if(samplingRate > readingRate) {
		printf("\nERRORE! | I valori inseriti non sono nell'ordine corretto\n");
		return(0);
	}

fd = i2cCommOpen(0x40);

	if (fd == -1) {
        	printf ("\nFailed to init I2C communication.\n");
        	return -1;
    	}


elements = readingRate / samplingRate;		//global

struct samples *samplesMainPtr;
samplesMainPtr = malloc(sizeof(*samplesMainPtr) * elements);
struct samples *samplesStartPtr;
samplesStartPtr = samplesMainPtr;		//i have a pointer that is static to the start of the structure

time_t samplingStartingTime;
samplingStartingTime = time(NULL);
uint8_t doubleCheck = 0;

	while(true) {

	time_t currentTime = time(NULL);	//Instead of checking time in each IF statement,
						//i just set it for each while iteration

		if(currentTime >= (samplingStartingTime + samplingRate)) {
			#if defined (REMOTE_SENSOR)

				memset(buffer, 0, 1024);

				remoteSensorGet("SHT21", buffer);
				sht21RemoteSensorDecode(buffer, samplesMainPtr);
			#else
				sht21GetValues(currentTime, samplesMainPtr);
			#endif
			samplingStartingTime = time(NULL);
			samplesMainPtr++;
			doubleCheck++;
		}

		if(doubleCheck == elements) {
			samplesMainPtr = samplesStartPtr;			//replacing the pointer to the start of the
			sht21DisplayValues(samplesMainPtr, writeTkn, deviceId);		//structure so i can read values
			doubleCheck = 0;
		}

	sleep(1);	//1s pause to limit CPU usage
        }

free(samplesStartPtr);

return(0);
}
