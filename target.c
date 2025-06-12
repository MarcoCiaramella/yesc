#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint32_t swap32(uint32_t val) {
    return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) | ((val << 24) & 0xff000000);
}

void swap32Arr(uint32_t* arr) {
    for (int i = 0; i < 8; i++) {
        arr[i] = swap32(arr[i]);
    }
}

uint8_t* hex2Uint8Array(const char* hex, size_t* length) {
    size_t len = strlen(hex);
    *length = len / 2;
    uint8_t* bytes = (uint8_t*)malloc(*length);
    
    for (size_t c = 0; c < len; c += 2) {
        char temp[3] = {hex[c], hex[c+1], '\0'};
        bytes[c/2] = (uint8_t)strtol(temp, NULL, 16);
    }
    
    return bytes;
}

char* calcTarget(double miningDiff) {
    char* target = (char*)malloc(65); // 64 hex chars + null terminator
    const char* maxTarget = "0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
    
    // Una semplificazione per la divisione di un numero esadecimale grande
    // Questa è un'approssimazione che potrebbe non essere precisa per tutti i valori
    sprintf(target, "%064llx", (unsigned long long)(0xFFFFFFFFFFFFFFFFULL / miningDiff));
    
    // Assicura che il target sia lungo 64 caratteri
    size_t len = strlen(target);
    if (len < 64) {
        memmove(target + (64 - len), target, len + 1);
        memset(target, '0', 64 - len);
    }
    
    return target;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mining_difficulty>\n", argv[0]);
        return 1;
    }
    
    double miningDiff = atof(argv[1]);
    char* targetHex = calcTarget(miningDiff);
    
    size_t byteLength;
    uint8_t* bytes = hex2Uint8Array(targetHex, &byteLength);
    
    uint32_t* words = (uint32_t*)bytes;
    swap32Arr(words);
    
    printf("Target: ");
    for (size_t i = 0; i < byteLength; i++) {
        printf("%02x", bytes[i]);
    }
    printf("\n");
    
    free(targetHex);
    free(bytes);
    
    return 0;
}
