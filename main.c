#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "stratum_client.h"

static stratum_client_t* global_client = NULL;

void signal_handler(int sig) {
    if (global_client) {
        printf("\nShutting down...\n");
        global_client->running = false;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <pool_url> <pool_port> <username>\n", argv[0]);
        printf("Example: %s stratum.pool.com 3333 your_wallet_address\n", argv[0]);
        return 1;
    }

    // Initialize client
    stratum_client_t client;
    memset(&client, 0, sizeof(client));
    global_client = &client;

    // Set up configuration
    strncpy(client.config.pool_url, argv[1], MAX_URL_LEN - 1);
    client.config.pool_port = atoi(argv[2]);
    strncpy(client.config.username, argv[3], MAX_USER_LEN - 1);
    strcpy(client.config.password, "x"); // Default password

    // Configure yespower parameters (example for yespower 1.0)
    client.config.yespower_params.version = YESPOWER_1_0;
    client.config.yespower_params.N = 2048;
    client.config.yespower_params.r = 8;
    client.config.yespower_params.pers = NULL;
    client.config.yespower_params.perslen = 0;

    // Initialize mutex
    pthread_mutex_init(&client.job_mutex, NULL);
    client.next_id = 1;
    client.running = true;

    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("YesPower Stratum Miner v1.0\n");
    printf("Connecting to %s:%d with user %s\n", 
           client.config.pool_url, client.config.pool_port, client.config.username);
    printf("YesPower params: version=%.1f, N=%u, r=%u\n",
           client.config.yespower_params.version * 0.1,
           client.config.yespower_params.N,
           client.config.yespower_params.r);

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
