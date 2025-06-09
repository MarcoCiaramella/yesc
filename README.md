# YesPower Stratum Miner

Un client di mining Stratum che utilizza l'algoritmo YesPower per il mining di criptovalute.

## Caratteristiche

- Supporto completo per il protocollo Stratum
- Integrazione con l'algoritmo YesPower (versioni 0.5 e 1.0)
- Mining multi-threaded
- Statistiche in tempo reale
- Gestione automatica dei job e riconnessione

## Compilazione

```bash
# Clona o scarica il codice
cd yesc

# Compila il progetto
make

# Opzionale: testa yespower
make test
```

## Utilizzo

```bash
./yespower-miner <pool_url> <pool_port> <username> [password] [version] [N] [r]
```

Esempi:
```bash
# Utilizzo base
./yespower-miner stratum.pool.com 3333 your_wallet_address

# Utilizzo con password
./yespower-miner stratum.pool.com 3333 your_wallet_address yourpassword

# Utilizzo con parametri YesPower personalizzati
./yespower-miner stratum.pool.com 3333 your_wallet_address x 1.0 2048 8
```

### Parametri:
- `pool_url`: URL del mining pool
- `pool_port`: Porta del mining pool
- `username`: Il tuo indirizzo wallet o username
- `[password]`: La tua password (predefinito: x)
- `[version]`: Versione YesPower: 0.5 o 1.0 (predefinito: 1.0)
- `[N]`: Parametro N di YesPower (predefinito: 2048)
- `[r]`: Parametro r di YesPower (predefinito: 8)

## Configurazione YesPower

Il miner è configurato di default per YesPower 1.0 con parametri:
- N = 2048
- r = 8
- version = 1.0

Puoi modificare questi parametri direttamente dalla riga di comando come mostrato negli esempi di utilizzo.

## Pool Supportate

Il miner supporta qualsiasi pool che utilizza il protocollo Stratum standard con YesPower come algoritmo di mining.

## Requisiti

- GCC compiler
- pthread library
- Sistema operativo Linux/Unix

## Note

- Questo è un miner di base per scopi educativi
- Per uso in produzione, considera ottimizzazioni aggiuntive
- Il parser JSON è semplificato e potrebbe richiedere miglioramenti per pool specifiche

## License

Segue la stessa licenza di YesPower (BSD-style license)
