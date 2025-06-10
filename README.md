# YesPower Stratum Client

Un client per il mining di criptovalute basate su YesPower utilizzando il protocollo Stratum.

## Requisiti

- GCC o altro compilatore C
- Librerie di supporto: pthread
- Sistema operativo compatibile con POSIX

## Compilazione

```
gcc main.c stratum_client.c json_parser.c -o yesc -lpthread
```

## Uso

```
./yesc [config_file]
```

Se non viene specificato un file di configurazione, verrà utilizzato per default `config.json`.

## Configurazione

Il file di configurazione deve essere in formato JSON e contenere i seguenti parametri:

### Parametri obbligatori:

- `pool_url`: URL o indirizzo IP del pool di mining
- `pool_port`: Porta del server pool
- `username`: Username o indirizzo wallet per il mining

### Parametri opzionali:

- `password`: Password per il pool (default: "x")
- `yespower_version`: Versione di YesPower (0.5 o 1.0, default: 1.0)
- `yespower_N`: Parametro N per YesPower (default: 2048)
- `yespower_r`: Parametro r per YesPower (default: 8)

## Esempio di file di configurazione

```json
{
  "pool_url": "stratum.pool.com",
  "pool_port": 3333,
  "username": "your_wallet_address",
  "password": "x",
  "yespower_version": 1.0,
  "yespower_N": 2048,
  "yespower_r": 8
}
```

## Operazioni

Una volta avviato, il miner si connetterà al pool specificato e inizierà a minare.
Le statistiche di mining saranno mostrate periodicamente nella console.
Per interrompere il mining, premere Ctrl+C.
