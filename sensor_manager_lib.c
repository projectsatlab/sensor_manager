// File sensor_main_lib.c con. Contiene le diverse funzioni di libreria  

/*
In order to use this file you need to install lib4curl.
When compiling this code, include the curl library with "-lcurl" param.
*/

#include "sensor_manager_lib.h"

// Open I2C communication on wiringPi lib
// Param: device address (SHT21 add = 0x40)
// Return: handle to device
int i2cCommOpen(uint8_t address) {

	wiringPiSetup();

	return wiringPiI2CSetup(address);

}

// Read SHT21 measure value
// Param: handle to device, command to read value (temp command = 0xE3, rh command = 0xE5)
// Return: measure register value
uint16_t sht21I2cRead(int fd, uint8_t command) {

uint16_t registerValue = 0;
        
        wiringPiI2CWrite(fd, command);				//Command to read 
        registerValue = wiringPiI2CRead(fd) << 8;		// Read MSB
        registerValue |= wiringPiI2CRead(fd) & 0x00FF;		// Read LSB

	return registerValue;
}

float sht21Rh2Real(uint16_t rawRh) {
        return(-6.0 + (125.0 * (rawRh / 65536.0f)));   //got an error when using 2^16 so converted it to 65536.0
}

float sht21Temp2Real(uint16_t rawTemp) {
        return(-46.85 + (172.72 * (rawTemp / 65536.0f)));
}

// Funzione di callback per scrivere la risposta
size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    strcat(data, ptr);
    return size * nmemb;
}

// Funzione client per le API esposte sul server sensori interno alla rete locale
// Param: sensor type: 1 = SHT21, puntatore ad array di caratteri che conterrà il response
// Return: void 
int remoteSensorGet(char *sensorType, char *response) {

  CURL *curl;
  CURLcode res;

  char url[1024];

	strcpy(url, "http://192.168.1.62:8000/GET?sensorTYPE=");
	strcat(url, sensorType);

  	curl = curl_easy_init();
  	if(curl) {
  		//curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.2.183:8000/GET?sensorTYPE=SHT21");
  		curl_easy_setopt(curl, CURLOPT_URL, url);

        	// Funzione di callback per gestire la risposta
        	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    		res = curl_easy_perform(curl);

        	// Controlliamo se c'e stato un errore
        	if(res != CURLE_OK) {
            		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        	} else {
            		//printf("Risposta del server: %s\n", response);  // Stampa la risposta JSON
        	}

    		/* always cleanup */
    		curl_easy_cleanup(curl);
  	}

  	return 0;
}

// Funzione client per le API esposte sul server 32connect.net
// Param: token (chiave), deviceID autorizzato, payload (8HEX = timestamp 2HEX = payload ID 22HEX misure esposte su 1 o 2 byte (2 o 4 HEX))
// Return: O = Ok, -1 = error
int apiPostServerWrite(const char *_token, const char *_deviceID, const char *_sensorData) {

#define URL "api.32connect.net/demetro_staging/streams.php"  // URL del server API

CURL *curl;
CURLcode res;
struct curl_slist *headers = NULL;
char response[4096] = "";  // Buffer per la risposta
char json_body[1024];

    // Corpo JSON da inviar con i parametri passati come stringhe
    snprintf(json_body, sizeof(json_body), "{\"token\": \"%s\", \"eventType\": \"ACK_MESSAGE\", \"deviceId\": \"%s\", \"payloads\":{\"sensorData\": [\"%s\"]}}", _token, _deviceID, _sensorData);

    // Inizializza libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        // Impostiamo l'URL di destinazione
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        // Aggiungiamo gli header per il contenuto JSON
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");

        // Impostiamo l'header
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Impostiamo il corpo della richiesta (POST)
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);

        // Funzione di callback per gestire la risposta
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        // Eseguiamo la richiesta
        res = curl_easy_perform(curl);

        // Controlliamo se c'e stato un errore
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            //printf("Risposta del server: %s\n", response);  // Stampa la risposta JSON
        }

        // Pulisci gli header
        curl_slist_free_all(headers);

        // Pulisci libcurl
        curl_easy_cleanup(curl);
    }

    // Finalizza libcurl
    curl_global_cleanup();

    if (!strcmp(response, "\"All data has been entered correctly\"")) {
        //printf("\nOK\n");
        return 0;
    }
    else {
        //printf("\nno OK\n");
        return -1;
        }

}

/**
int main() {

printf("\nOra chiamo la funzione del curl\n");
//apiPostServerWrite("HIDDEN-FOR-PRIVACY", "HIDDEN-FOR-PRIVACY", "HIDDEN-FOR-PRIVACY");
apiPostServerWrite("HIDDEN-FOR-PRIVACY", "HIDDEN-FOR-PRIVACY", "HIDDEN-FOR-PRIVACY");
return 0;
}
**/
