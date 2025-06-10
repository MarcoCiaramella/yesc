#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "stratum_client.h"
#include "json_parser.h"

static stratum_client_t* global_client = NULL;

void signal_handler(int sig) {
    if (global_client) {
        printf("\nShutting down...\n");
        global_client->running = false;
    }
}

void print_usage(const char* program) {
    printf("Usage: %s [config_file]\n", program);
    printf("Default config file: config.json\n");
    printf("\nExample config.json content:\n");
    printf("{\n");
    printf("  \"pool_url\": \"stratum.pool.com\",\n");
    printf("  \"pool_port\": 3333,\n");
    printf("  \"username\": \"your_wallet_address\",\n");
    printf("  \"password\": \"x\",\n");
    printf("  \"yespower_version\": 1.0,\n");
    printf("  \"yespower_N\": 2048,\n");
    printf("  \"yespower_r\": 8\n");
    printf("}\n");
}

void set_default_config(mining_config_t* config) {
    // Imposta la configurazione predefinita
    strncpy(config->pool_url, "yespower.eu.mine.zpool.ca", MAX_URL_LEN - 1);
    config->pool_port = 6234;
    strncpy(config->username, "bc1qvvecpthfawvdc4q8hr56n2fxmp83clh80dvkze", MAX_USER_LEN - 1);
    strncpy(config->password, "c=BTC", MAX_PASS_LEN - 1);
    
    // Imposta i parametri YesPower
    config->yespower_params.version = YESPOWER_1_0;
    config->yespower_params.N = 2048;
    config->yespower_params.r = 32;
    config->yespower_params.pers = NULL;
    config->yespower_params.perslen = 0;
    
    printf("Using default configuration:\n");
    printf("Pool: %s:%d\n", config->pool_url, config->pool_port);
    printf("User: %s, Pass: %s\n", config->username, config->password);
    printf("YesPower params: version=1.0, N=%u, r=%u\n", 
           config->yespower_params.N, config->yespower_params.r);
}

int main(int argc, char* argv[]) {
    // Initialize client
    stratum_client_t client;
    memset(&client, 0, sizeof(client));
    global_client = &client;
    
    // Rileva e stampa il numero di core CPU
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Numero di core CPU rilevati: %ld\n", num_cores);
    
    // Set default config file or use the one provided
    const char* config_file = "config.json";
    if (argc >= 2) {
        config_file = argv[1];
    }
    
    // Load configuration from JSON file or use default if it fails
    if (load_config_from_json(config_file, &client.config) != 0) {
        printf("Failed to load configuration from %s, using default settings\n", config_file);
        set_default_config(&client.config);
    } else {
        printf("Loaded configuration from %s\n", config_file);
        printf("Connecting to %s:%d with user %s, password %s\n", 
               client.config.pool_url, client.config.pool_port, 
               client.config.username, client.config.password);
        printf("YesPower params: version=%.1f, N=%u, r=%u\n",
               client.config.yespower_params.version * 0.1,
               client.config.yespower_params.N,
               client.config.yespower_params.r);
    }

    // Initialize mutex
    pthread_mutex_init(&client.job_mutex, NULL);
    client.next_id = 1;
    client.running = true;

    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("YesPower Stratum Miner v1.0\n");

    // Connect to pool
    if (stratum_connect(&client) != 0) {
        printf("Failed to connect to pool\n");
        return 1;
    }

    // Subscribe to pool
    if (stratum_subscribe(&client) != 0) {
        printf("Failed to subscribe to pool\n");
        stratum_disconnect(&client);
        return 1;
    }

    // Authorize with pool
    if (stratum_authorize(&client) != 0) {
        printf("Failed to authorize with pool\n");
        stratum_disconnect(&client);
        return 1;
    }

    // Start receiver thread
    pthread_t receiver_thread, mining_thread_handle;
    if (pthread_create(&receiver_thread, NULL, stratum_receiver_thread, &client) != 0) {
        printf("Failed to create receiver thread\n");
        stratum_disconnect(&client);
        return 1;
    }

    // Wait for initial job
    printf("Waiting for initial job...\n");
    while (client.running && !client.new_job) {
        usleep(100000);
    }

    if (!client.running) {
        printf("Interrupted before receiving job\n");
        stratum_disconnect(&client);
        return 1;
    }

    // Start mining thread
    if (pthread_create(&mining_thread_handle, NULL, mining_thread, &client) != 0) {
        printf("Failed to create mining thread\n");
        client.running = false;
        pthread_join(receiver_thread, NULL);
        stratum_disconnect(&client);
        return 1;
    }

    printf("Mining started! Press Ctrl+C to stop.\n");

    // Wait for threads to finish
    pthread_join(mining_thread_handle, NULL);
    pthread_join(receiver_thread, NULL);

    // Cleanup
    stratum_disconnect(&client);
    pthread_mutex_destroy(&client.job_mutex);

    printf("Final stats: Submitted=%llu, Accepted=%llu, Rejected=%llu\n",
           client.shares_submitted, client.shares_accepted, client.shares_rejected);

    return 0;
}
