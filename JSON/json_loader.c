#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_loader.h"

// Funzione per leggere il valore di una chiave in un file JSON (senza libreria esterna)
const char* readJson(const char* key, char *content_) {
const char *json_file_path = "./config.json";  // Percorso del file JSON

char *content;

    if(!content_) {

    	// Apri il file JSON
    	FILE *file = fopen(json_file_path, "r");
    	if (file == NULL) {
        	perror("Errore nell'aprire il file JSON");
        	return NULL;
    	}

    	// Leggi tutto il contenuto del file
    	fseek(file, 0, SEEK_END);
    	long length = ftell(file);
    	fseek(file, 0, SEEK_SET);
    
    	content = (char *)malloc(length + 1);
    	fread(content, 1, length, file);
    	content[length] = '\0';  // Aggiungi terminatore della stringa
    	fclose(file);
    } else {

	content = (char *)malloc(strlen(content_));
	strcpy(content, content_);	

    }


 
    // Cerca la chiave nel JSON
    char *key_start = strstr(content, key);  // Trova la posizione della chiave
    if (key_start != NULL) {
        // Spostati dopo il nome della chiave, quindi cerca il valore
        char *value_start = strchr(key_start, ':');
        if (value_start != NULL) {
            value_start++;  // Salta il ':' e lo spazio

            // Rimuovi gli spazi bianchi all'inizio del valore
            while (*value_start == ' ' || *value_start == '\"') value_start++;

            // Trova dove termina il valore (potrebbe essere una virgola o la fine della stringa)
            char *value_end = strchr(value_start, '\"');
            if (value_end != NULL) {
                // Termina la stringa con un '\0' per ottenere il valore
                *value_end = '\0';
                return value_start;  // Restituisci il valore
            }
        }
    }

    free(content);
    return NULL;  // Se la chiave non è trovata
}
