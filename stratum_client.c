#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include "stratum_client.h"
#include "json_parser.h"

int stratum_connect(stratum_client_t* client) {
    struct hostent* host_entry;
    struct sockaddr_in server_addr;

    // Resolve hostname
    host_entry = gethostbyname(client->config.pool_url);
    if (host_entry == NULL) {
        printf("Error: Cannot resolve hostname %s\n", client->config.pool_url);
        return -1;
    }

    // Create socket
    client->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client->config.pool_port);
    memcpy(&server_addr.sin_addr.s_addr, host_entry->h_addr, host_entry->h_length);

    // Connect to server
    if (connect(client->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client->sockfd);
        return -1;
    }

    client->connected = true;
    printf("Connected to %s:%d\n", client->config.pool_url, client->config.pool_port);
    return 0;
}

void stratum_disconnect(stratum_client_t* client) {
    if (client->connected) {
        close(client->sockfd);
        client->connected = false;
        printf("Disconnected from pool\n");
    }
}

int stratum_subscribe(stratum_client_t* client) {
    char request[1024];
    char response[MAX_BUFFER_SIZE];
    
    snprintf(request, sizeof(request),
        "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"yespower-miner/1.0\"]}\n",
        client->next_id++);

    if (send(client->sockfd, request, strlen(request), 0) < 0) {
        perror("Failed to send subscribe request");
        return -1;
    }

    ssize_t received = recv(client->sockfd, response, sizeof(response) - 1, 0);
    if (received <= 0) {
        perror("Failed to receive subscribe response");
        return -1;
    }
    response[received] = '\0';

    // Parse JSON response and extract extranonce1, extranonce2_size
    if (parse_subscribe_response(response, &client->current_job) == 0) {
        client->subscribed = true;
        printf("Successfully subscribed to pool\n");
        return 0;
    }

    return -1;
}

int stratum_authorize(stratum_client_t* client) {
    char request[1024];
    char response[MAX_BUFFER_SIZE];
    
    snprintf(request, sizeof(request),
        "{\"id\": %u, \"method\": \"mining.authorize\", \"params\": [\"%s\", \"%s\"]}\n",
        client->next_id++, client->config.username, client->config.password);

    if (send(client->sockfd, request, strlen(request), 0) < 0) {
        perror("Failed to send authorize request");
        return -1;
    }

    ssize_t received = recv(client->sockfd, response, sizeof(response) - 1, 0);
    if (received <= 0) {
        perror("Failed to receive authorize response");
        return -1;
    }
    response[received] = '\0';

    // Parse JSON response
    if (parse_authorize_response(response)) {
        client->authorized = true;
        printf("Successfully authorized as %s\n", client->config.username);
        return 0;
    }

    printf("Authorization failed\n");
    return -1;
}

int stratum_submit_share(stratum_client_t* client, uint32_t nonce, uint32_t extranonce2) {
    char request[1024];
    char nonce_hex[9], extranonce2_hex[17], ntime_hex[9];
    
    // Convert values to hex
    snprintf(nonce_hex, sizeof(nonce_hex), "%08x", nonce);
    snprintf(extranonce2_hex, sizeof(extranonce2_hex), "%08x", extranonce2);
    snprintf(ntime_hex, sizeof(ntime_hex), "%s", client->current_job.ntime);

    // Aggiungo stampa dei dettagli dello share prima dell'invio
    printf("Preparazione share: job_id=%s, nonce=%s, extranonce2=%s, ntime=%s\n", 
           client->current_job.job_id, nonce_hex, extranonce2_hex, ntime_hex);

    snprintf(request, sizeof(request),
        "{\"id\": %u, \"method\": \"mining.submit\", \"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]}\n",
        client->next_id++,
        client->config.username,
        client->current_job.job_id,
        extranonce2_hex,
        ntime_hex,
        nonce_hex);

    if (send(client->sockfd, request, strlen(request), 0) < 0) {
        perror("Failed to send submit request");
        return -1;
    }

    client->shares_submitted++;
    printf("Share submitted: nonce=%s, extranonce2=%s\n", nonce_hex, extranonce2_hex);
    return 0;
}

void* stratum_receiver_thread(void* arg) {
    stratum_client_t* client = (stratum_client_t*)arg;
    char buffer[MAX_BUFFER_SIZE];
    
    while (client->running && client->connected) {
        ssize_t received = recv(client->sockfd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            if (received == 0) {
                printf("Server disconnected\n");
            } else {
                perror("Receive error");
            }
            client->connected = false;
            break;
        }
        
        buffer[received] = '\0';
        
        // Parse incoming messages (job notifications, submit responses)
        if (strstr(buffer, "mining.notify")) {
            pthread_mutex_lock(&client->job_mutex);
            if (parse_job_notification(buffer, &client->current_job) == 0) {
                client->new_job = true;
                printf("New job received: %s\n", client->current_job.job_id);
                print_job_details(&client->current_job);
                print_target(client->current_job.target);
            }
            pthread_mutex_unlock(&client->job_mutex);
        }
        // Gestione migliorata per mining.set_difficulty
        else if (strstr(buffer, "mining.set_difficulty") || strstr(buffer, "\"method\":\"mining.set_difficulty\"")) {
            // Cerchiamo prima il parametro params in formato JSON
            char* diff_str = NULL;
            
            // Cerca in formato "params":[numero]
            diff_str = strstr(buffer, "\"params\":");
            if (diff_str) {
                diff_str += 9; // Salta "params":
                
                // Trova l'inizio dell'array
                while (*diff_str && *diff_str != '[') diff_str++;
                if (*diff_str == '[') diff_str++; // Salta '['
                
                // Estrai il valore di difficoltà
                double new_diff = strtod(diff_str, NULL);
                if (new_diff > 0) {
                    client->current_difficulty = new_diff;
                    printf("\033[36mDifficoltà impostata dal pool: %.6f\033[0m\n", client->current_difficulty);
                } else {
                    printf("\033[33mWarning: Ricevuto messaggio set_difficulty con valore non valido\033[0m\n");
                }
            } else {
                printf("\033[33mWarning: Messaggio mining.set_difficulty malformattato\033[0m\n");
            }
        }
        else if (strstr(buffer, "\"result\":true")) {
            client->shares_accepted++;
            
            // Estrai eventuali informazioni aggiuntive come la difficoltà
            char* difficulty_str = NULL;
            if (strstr(buffer, "\"difficulty\":")) {
                difficulty_str = strstr(buffer, "\"difficulty\":");
                if (difficulty_str) {
                    difficulty_str += 12; // Skip "difficulty":
                }
            }
            
            printf("\033[32mShare ACCETTATO! (%llu/%llu) - %.2f%%\033[0m", 
                   client->shares_accepted, 
                   client->shares_submitted,
                   (client->shares_submitted > 0) ? 
                       (100.0 * client->shares_accepted / client->shares_submitted) : 0);
            
            if (difficulty_str) {
                // Estraiamo il valore di difficoltà fino alla virgola o chiusura di parentesi
                char diff_value[32] = {0};
                int i = 0;
                while (i < 31 && difficulty_str[i] && difficulty_str[i] != ',' && difficulty_str[i] != '}') {
                    diff_value[i] = difficulty_str[i];
                    i++;
                }
                diff_value[i] = '\0';
                printf(" - Difficoltà: %s", diff_value);
            }
            printf("\n");
        }
        else if (strstr(buffer, "\"result\":false")) {
            client->shares_rejected++;
            
            // Estrai il messaggio di errore se presente
            char* error_msg = strstr(buffer, "\"error\":");
            if (error_msg) {
                error_msg = strstr(error_msg, "\"message\":");
                if (error_msg) {
                    error_msg += 11; // Skip "message":
                    if (*error_msg == '"') error_msg++; // Skip quote
                    
                    // Estrai il messaggio fino alla virgola o chiusura di parentesi
                    char msg_buffer[128] = {0};
                    int i = 0;
                    while (i < 127 && error_msg[i] && error_msg[i] != '"' && error_msg[i] != '}') {
                        msg_buffer[i] = error_msg[i];
                        i++;
                    }
                    msg_buffer[i] = '\0';
                    
                    printf("\033[31mShare RIFIUTATO! (%llu/%llu) - Errore: %s\033[0m\n",
                           client->shares_rejected, client->shares_submitted, msg_buffer);
                } else {
                    printf("\033[31mShare RIFIUTATO! (%llu/%llu)\033[0m\n", 
                           client->shares_rejected, client->shares_submitted);
                }
            } else {
                printf("\033[31mShare RIFIUTATO! (%llu/%llu)\033[0m\n", 
                       client->shares_rejected, client->shares_submitted);
            }
        }
    }
    
    return NULL;
}

void* mining_thread(void* arg) {
    stratum_client_t* client = (stratum_client_t*)arg;
    uint8_t block_header[BLOCK_HEADER_SIZE];
    yespower_binary_t hash;
    uint32_t nonce = 0;
    uint32_t extranonce2 = 0;
    time_t start_time = time(NULL);
    uint64_t hashes = 0;
    
    printf("Mining thread started\n");
    
    while (client->running) {
        if (!client->new_job) {
            usleep(100000); // Wait 100ms
            continue;
        }
        
        pthread_mutex_lock(&client->job_mutex);
        stratum_job_t job_copy = client->current_job;
        client->new_job = false;
        pthread_mutex_unlock(&client->job_mutex);
        
        printf("Mining job %s...\n", job_copy.job_id);
        
        // Mining loop for current job
        for (nonce = 0; nonce < 0xFFFFFFFF && client->running && !client->new_job; nonce++) {
            // Build block header
            if (build_block_header(client, nonce, extranonce2, block_header) != 0) {
                continue;
            }
            
            // Calculate yespower hash
            if (yespower_tls(block_header, BLOCK_HEADER_SIZE, &client->config.yespower_params, &hash) != 0) {
                printf("Error calculating yespower hash\n");
                continue;
            }
            
            hashes++;
            
            // Check if hash meets target
            if (check_target(hash.uc, job_copy.target)) {
                printf("Found share! Submitting...\n");
                stratum_submit_share(client, nonce, extranonce2);
            }
            
            // Print hashrate every 10 seconds
            if (hashes % 10000 == 0) {
                time_t current_time = time(NULL);
                if (current_time - start_time >= 10) {
                    double hashrate = (double)hashes / (current_time - start_time);
                    printf("Hashrate: %.2f H/s\n", hashrate);
                    start_time = current_time;
                    hashes = 0;
                }
            }
        }
        
        // Try next extranonce2 if nonce space exhausted
        extranonce2++;
        nonce = 0;
    }
    
    printf("Mining thread stopped\n");
    return NULL;
}

int build_block_header(stratum_client_t* client, uint32_t nonce, uint32_t extranonce2, uint8_t* header) {
    // ...existing code for building block header from stratum job...
    // This involves concatenating coinb1 + extranonce1 + extranonce2 + coinb2
    // Then building merkle root and assembling the 80-byte block header
    
    // For now, simplified implementation
    memset(header, 0, BLOCK_HEADER_SIZE);
    
    // Copy version, prev_hash, merkle_root, timestamp, bits, nonce
    hex_to_bin(client->current_job.version, header, 4);
    hex_to_bin(client->current_job.prevhash, header + 4, 32);
    // merkle_root calculation needed here
    hex_to_bin(client->current_job.ntime, header + 68, 4);
    hex_to_bin(client->current_job.nbits, header + 72, 4);
    memcpy(header + 76, &nonce, 4);
    
    return 0;
}

bool check_target(const uint8_t* hash, const char* target_hex) {
    uint8_t target[32];
    hex_to_bin(target_hex, target, 32);
    
    // Compare hash with target (little-endian)
    for (int i = 31; i >= 0; i--) {
        if (hash[i] < target[i]) return true;
        if (hash[i] > target[i]) return false;
    }
    return false;
}

void hex_to_bin(const char* hex, uint8_t* bin, size_t bin_len) {
    for (size_t i = 0; i < bin_len; i++) {
        sscanf(hex + i * 2, "%2hhx", &bin[i]);
    }
}

void bin_to_hex(const uint8_t* bin, char* hex, size_t bin_len) {
    for (size_t i = 0; i < bin_len; i++) {
        sprintf(hex + i * 2, "%02x", bin[i]);
    }
    hex[bin_len * 2] = '\0';
}

void print_target(const char* target_hex) {
    // Print target in human-readable format
    printf("Target: ");
    
    // Find first non-zero character to make output more readable
    int i = 0;
    while (target_hex[i] == '0' && i < 64 - 8) {
        i++;
    }
    
    // Print with colors for better visibility
    printf("\033[36m");  // Cyan color
    
    // Print the first non-zero part in bold
    if (i < 64) {
        printf("\033[1m");  // Bold
        printf("%c", target_hex[i]);
        printf("\033[0m\033[36m");  // Reset bold but keep cyan
        i++;
    }
    
    // Print the rest of the significant digits
    for (; i < 64; i++) {
        printf("%c", target_hex[i]);
    }
    
    printf("\033[0m\n");  // Reset color
}

void print_job_details(const stratum_job_t* job) {
    printf("==== Job Details ====\n");
    printf("Job ID: %s\n", job->job_id);
    printf("Previous Hash: %s\n", job->prevhash);
    printf("Version: %s\n", job->version);
    printf("nBits: %s\n", job->nbits);
    printf("nTime: %s\n", job->ntime);
    printf("Clean Jobs: %s\n", job->clean_jobs ? "true" : "false");
    
    printf("Merkle Branches: %d branches\n", job->merkle_count);
    for (int i = 0; i < job->merkle_count && i < 5; i++) {
        printf("  [%d]: %s\n", i, job->merkle_branches[i]);
    }
    if (job->merkle_count > 5) {
        printf("  ... and %d more branches\n", job->merkle_count - 5);
    }
    
    printf("Coinb1 (first %d chars): %.40s%s\n", 
           job->coinb1[0] ? 40 : 0, 
           job->coinb1[0] ? job->coinb1 : "(empty)", 
           strlen(job->coinb1) > 40 ? "..." : "");
    
    printf("Coinb2 (first %d chars): %.40s%s\n", 
           job->coinb2[0] ? 40 : 0, 
           job->coinb2[0] ? job->coinb2 : "(empty)",
           strlen(job->coinb2) > 40 ? "..." : "");
    
    printf("Extranonce1: %s\n", job->extranonce1);
    printf("Extranonce2 Size: %d\n", job->extranonce2_size);
    printf("====================\n");
}
