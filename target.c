#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <gmp.h>

// Funzione per invertire l'ordine dei byte in un valore a 32 bit
uint32_t swap32(uint32_t val) {
    return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) | ((val << 24) & 0xff000000);
}

// Funzione per invertire l'ordine dei byte in un array di valori a 32 bit
void swap32Arr(uint32_t *arr, size_t length) {
    for (size_t i = 0; i < length; i++) {
        arr[i] = swap32(arr[i]);
    }
}

// Funzione per convertire stringa esadecimale in array di byte
void hex2Uint8Array(const char* hex, uint8_t* bytes) {
    size_t len = strlen(hex);
    for (size_t i = 0; i < len/2; i++) {
        sscanf(hex + i*2, "%2hhx", &bytes[i]);
    }
}

// Funzione per calcolare il target
char* calcTarget(double miningDiff) {
    mpf_t num, divisore, risultato;
    char* result = (char*)malloc(65); // 64 caratteri + terminatore
    
    // Inizializza le variabili GMP con precisione elevata (1024 bit)
    mpf_init2(num, 1024);
    mpf_init2(divisore, 1024);
    mpf_init2(risultato, 1024);
    
    // Inizializza il numero dalla stringa esadecimale
    mpz_t num_intero;
    mpz_init(num_intero);
    mpz_set_str(num_intero, "0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16);
    mpf_set_z(num, num_intero);
    
    // Converti il parametro di difficoltà in floating point
    mpf_set_d(divisore, miningDiff);
    
    // Esegui la divisione
    mpf_div(risultato, num, divisore);
    
    // Converti il risultato in un intero e poi in stringa esadecimale
    mpz_t result_int;
    mpz_init(result_int);
    mpz_set_f(result_int, risultato);
    
    // Ottieni la rappresentazione esadecimale
    char* hex_result = mpz_get_str(NULL, 16, result_int);
    
    // Formatta il risultato come stringa esadecimale di 64 caratteri con zero-padding
    size_t hex_len = strlen(hex_result);
    if (hex_len > 64) {
        // Se la stringa è troppo lunga, prendiamo solo gli ultimi 64 caratteri
        strcpy(result, hex_result + (hex_len - 64));
    } else {
        // Altrimenti aggiungiamo zeri all'inizio
        memset(result, '0', 64 - hex_len);
        strcpy(result + (64 - hex_len), hex_result);
    }
    result[64] = '\0';
    
    // Libera la memoria
    free(hex_result);
    mpz_clear(result_int);
    mpz_clear(num_intero);
    mpf_clear(num);
    mpf_clear(divisore);
    mpf_clear(risultato);
    
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Utilizzo: %s <difficoltà_mining>\n", argv[0]);
        return 1;
    }
    
    // Converti l'argomento della riga di comando in un double
    double miningDiff = atof(argv[1]);
    
    // Calcola il target
    char* targetHex = calcTarget(miningDiff);
    
    // Allocazione di un array di 32 byte (256 bit)
    uint8_t bytes[32] = {0};
    
    // Converti la stringa esadecimale in bytes
    hex2Uint8Array(targetHex, bytes);
    
    // Interpreta i bytes come un array di uint32_t e applica swap32 a ciascun elemento
    uint32_t* words = (uint32_t*)bytes;
    swap32Arr(words, 8); // 32 byte = 8 uint32_t
    
    // Rimossa la stampa del target
    
    // Libera la memoria
    free(targetHex);
    
    return 0;
}
    // Libera la memoria
    free(targetHex);
    
    return 0;
}
