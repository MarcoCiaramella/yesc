#ifndef STRATUM_CLIENT_H
#define STRATUM_CLIENT_H

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include "yespower-main/yespower.h"

#define MAX_URL_LEN 256
#define MAX_USER_LEN 128
#define MAX_PASS_LEN 128
#define MAX_BUFFER_SIZE 4096
#define MAX_EXTRANONCE_LEN 32
#define BLOCK_HEADER_SIZE 80
#define MAX_JOB_ID_LENGTH 64
#define MAX_COINBASE_SIZE 256
#define MAX_MERKLE_BRANCHES 32

typedef struct {
    char pool_url[MAX_URL_LEN];
    int pool_port;
    char username[MAX_USER_LEN];
    char password[MAX_PASS_LEN];
    yespower_params_t yespower_params;
} mining_config_t;

typedef struct {
    char job_id[MAX_JOB_ID_LENGTH];
    char prevhash[65];
    char coinb1[MAX_COINBASE_SIZE];
    char coinb2[MAX_COINBASE_SIZE];
    char merkle_branches[MAX_MERKLE_BRANCHES][65];
    int merkle_count;
    char version[9];
    char nbits[9];
    char ntime[9];
    char clean_jobs[2];
    char extranonce1[17];
    int extranonce2_size;
    char target[65];  // Manteniamo per compatibilità
    uint32_t target_array[8];  // Nuovo array per il target
} stratum_job_t;

typedef struct {
    int sockfd;
    mining_config_t config;
    stratum_job_t current_job;
    pthread_mutex_t job_mutex;
    bool connected;
    bool authorized;
    bool subscribed;
    uint32_t next_id;
    bool new_job;
    uint64_t shares_submitted;
    uint64_t shares_accepted;
    uint64_t shares_rejected;
    bool running;
    double current_difficulty;  // Aggiunto campo per memorizzare la difficoltà corrente
} stratum_client_t;

// Function prototypes
int stratum_connect(stratum_client_t* client);
void stratum_disconnect(stratum_client_t* client);
int stratum_subscribe(stratum_client_t* client);
int stratum_authorize(stratum_client_t* client);
int stratum_submit_share(stratum_client_t* client, uint32_t nonce, uint32_t extranonce2);
void* stratum_receiver_thread(void* arg);
void* mining_thread(void* arg);
int build_block_header(stratum_client_t* client, uint32_t nonce, uint32_t extranonce2, uint8_t* header);
bool check_target(const uint8_t* hash, const char* target_hex);
void hex_to_bin(const char* hex, uint8_t* bin, size_t bin_len);
void bin_to_hex(const uint8_t* bin, char* hex, size_t bin_len);
void print_target(const char* target_hex);
void print_job_details(const stratum_job_t* job);
int stratum_suggest_difficulty(stratum_client_t* client, double difficulty);

#endif
