#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Utilizzo: %s <divisore>\n", argv[0]);
        return 1;
    }
    
    mpf_t num, divisore, risultato;
    
    // Inizializza le variabili GMP con precisione elevata (1024 bit)
    mpf_init2(num, 1024);
    mpf_init2(divisore, 1024);
    mpf_init2(risultato, 1024);
    
    // Inizializza il numero dalla stringa esadecimale
    mpz_t num_intero;
    mpz_init(num_intero);
    mpz_set_str(num_intero, "0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16);
    mpf_set_z(num, num_intero);
    
    // Converti l'argomento della riga di comando in floating point
    mpf_set_str(divisore, argv[1], 10);
    
    // Esegui la divisione
    mpf_div(risultato, num, divisore);
    
    // Stampa il risultato con alta precisione
    gmp_printf("res: %.60Ff\n", risultato);
    
    // Libera la memoria
    mpz_clear(num_intero);
    mpf_clear(num);
    mpf_clear(divisore);
    mpf_clear(risultato);
    
    return 0;
}
