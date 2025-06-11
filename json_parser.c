#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "json_parser.h"

int parse_subscribe_response(const char* json, stratum_job_t* job) {
    // Simplified JSON parsing for mining.subscribe response
    // Look for extranonce1 and extranonce2_size in the result
    
    char* extranonce1_start = strstr(json, "\"");
    if (extranonce1_start) {
        extranonce1_start++; // Skip opening quote
        char* extranonce1_end = strstr(extranonce1_start, "\"");
        if (extranonce1_end) {
            size_t len = extranonce1_end - extranonce1_start;
            if (len < MAX_EXTRANONCE_LEN) {
                strncpy(job->extranonce1, extranonce1_start, len);
                job->extranonce1[len] = '\0';
            }
        }
    }
    
    // Look for extranonce2_size (typically 4)
    job->extranonce2_size = 4;
    
    return 0;
}

int parse_authorize_response(const char* json) {
    // Look for "result":true
    return strstr(json, "\"result\":true") ? 1 : 0;
}

int parse_job_notification(const char* json, stratum_job_t* job) {
    // Parse JSON for mining.notify message
    char* params_start = strstr(json, "\"params\":[");
    if (!params_start) return -1;
    
    params_start += 9; // Skip "params":[ 

    // Extract job_id (first parameter)
    char* job_id_start = strchr(params_start, '"');
    if (!job_id_start) return -1;
    job_id_start++; // Skip opening quote
    
    char* job_id_end = strchr(job_id_start, '"');
    if (!job_id_end) return -1;
    
    size_t len = job_id_end - job_id_start;
    if (len >= sizeof(job->job_id)) return -1;
    strncpy(job->job_id, job_id_start, len);
    job->job_id[len] = '\0';
    
    // Move to next parameter (prevhash)
    params_start = job_id_end + 2; // Skip quote and comma
    char* prevhash_start = strchr(params_start, '"');
    if (!prevhash_start) return -1;
    prevhash_start++;
    
    char* prevhash_end = strchr(prevhash_start, '"');
    if (!prevhash_end) return -1;
    
    len = prevhash_end - prevhash_start;
    if (len >= sizeof(job->prevhash)) return -1;
    strncpy(job->prevhash, prevhash_start, len);
    job->prevhash[len] = '\0';
    
    // Extract coinb1
    params_start = prevhash_end + 2;
    char* coinb1_start = strchr(params_start, '"');
    if (!coinb1_start) return -1;
    coinb1_start++;
    
    char* coinb1_end = strchr(coinb1_start, '"');
    if (!coinb1_end) return -1;
    
    len = coinb1_end - coinb1_start;
    if (len >= sizeof(job->coinb1)) return -1;
    strncpy(job->coinb1, coinb1_start, len);
    job->coinb1[len] = '\0';
    
    // Extract coinb2
    params_start = coinb1_end + 2;
    char* coinb2_start = strchr(params_start, '"');
    if (!coinb2_start) return -1;
    coinb2_start++;
    
    char* coinb2_end = strchr(coinb2_start, '"');
    if (!coinb2_end) return -1;
    
    len = coinb2_end - coinb2_start;
    if (len >= sizeof(job->coinb2)) return -1;
    strncpy(job->coinb2, coinb2_start, len);
    job->coinb2[len] = '\0';
    
    // Extract merkle_branches (array of strings)
    params_start = coinb2_end + 2;
    char* merkle_start = strchr(params_start, '[');
    if (!merkle_start) return -1;
    merkle_start++;
    
    char* merkle_end = strchr(merkle_start, ']');
    if (!merkle_end) return -1;
    
    // Parse each merkle branch
    job->merkle_count = 0;
    char* branch_start = merkle_start;
    
    while (branch_start < merkle_end && job->merkle_count < 32) {
        branch_start = strchr(branch_start, '"');
        if (!branch_start || branch_start >= merkle_end) break;
        branch_start++;
        
        char* branch_end = strchr(branch_start, '"');
        if (!branch_end || branch_end >= merkle_end) break;
        
        len = branch_end - branch_start;
        if (len >= sizeof(job->merkle_branches[0])) return -1;
        strncpy(job->merkle_branches[job->merkle_count], branch_start, len);
        job->merkle_branches[job->merkle_count][len] = '\0';
        job->merkle_count++;
        
        branch_start = branch_end + 1;
    }
    
    // Extract version
    params_start = merkle_end + 2;
    char* version_start = strchr(params_start, '"');
    if (!version_start) return -1;
    version_start++;
    
    char* version_end = strchr(version_start, '"');
    if (!version_end) return -1;
    
    len = version_end - version_start;
    if (len >= sizeof(job->version)) return -1;
    strncpy(job->version, version_start, len);
    job->version[len] = '\0';
    
    // Extract nbits
    params_start = version_end + 2;
    char* nbits_start = strchr(params_start, '"');
    if (!nbits_start) return -1;
    nbits_start++;
    
    char* nbits_end = strchr(nbits_start, '"');
    if (!nbits_end) return -1;
    
    len = nbits_end - nbits_start;
    if (len >= sizeof(job->nbits)) return -1;
    strncpy(job->nbits, nbits_start, len);
    job->nbits[len] = '\0';
    
    // Extract ntime
    params_start = nbits_end + 2;
    char* ntime_start = strchr(params_start, '"');
    if (!ntime_start) return -1;
    ntime_start++;
    
    char* ntime_end = strchr(ntime_start, '"');
    if (!ntime_end) return -1;
    
    len = ntime_end - ntime_start;
    if (len >= sizeof(job->ntime)) return -1;
    strncpy(job->ntime, ntime_start, len);
    job->ntime[len] = '\0';
    
    // Extract clean_jobs (boolean)
    params_start = ntime_end + 2;
    if (strstr(params_start, "true")) {
        job->clean_jobs = true;
    } else {
        job->clean_jobs = false;
    }
    
    // Convert nbits to target (proper conversion)
    nbits_to_target(job->nbits, job->target, sizeof(job->target));
    
    return 0;
}

// New function to convert nbits to target properly
void nbits_to_target(const char* nbits_hex, char* target_hex, size_t target_size) {
    unsigned int nbits;
    sscanf(nbits_hex, "%x", &nbits);
    
    // Extract exponent and mantissa from nbits
    unsigned int exponent = nbits >> 24;
    unsigned int mantissa = nbits & 0x00FFFFFF;
    
    // Calculate difficulty based on nbits
    // Formula: difficulty = (max_target) / (current_target)
    // Where current_target = mantissa * 256^(exponent-3)
    double difficulty = 1.0;
    
    if (exponent > 3) {
        difficulty = mantissa * (1 << (8 * (exponent - 3)));
    } else if (exponent < 3) {
        difficulty = (double)mantissa / (1 << (8 * (3 - exponent)));
    } else {
        difficulty = mantissa;
    }
    
    // Set initial max_target (all bytes 0xFF except first two)
    unsigned char target_bytes[32] = {0};
    for (int i = 2; i < 32; i++) {
        target_bytes[i] = 0xFF;
    }
    
    // Limit difficulty to avoid division by zero
    if (difficulty <= 0.0) difficulty = 1.0;
    
    // Calculate target = max_target / difficulty
    double factor = 1.0 / difficulty;
    
    // Multiply each byte by factor, starting from most significant
    double carry = 0.0;
    for (int i = 2; i < 32; i++) {
        double temp = target_bytes[i] * factor + carry;
        target_bytes[i] = (unsigned char)temp;
        carry = (temp - (unsigned char)temp) * 256.0;
    }
    
    // Convert to hex string
    char* ptr = target_hex;
    for (int i = 0; i < 32; i++) {
        sprintf(ptr, "%02x", target_bytes[i]);
        ptr += 2;
    }
    *ptr = '\0';
}

char* extract_json_string(const char* json, const char* key) {
    // Simplified string extraction
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", key);
    
    char* start = strstr(json, search_pattern);
    if (!start) return NULL;
    
    start += strlen(search_pattern);
    char* end = strstr(start, "\"");
    if (!end) return NULL;
    
    size_t len = end - start;
    char* result = malloc(len + 1);
    strncpy(result, start, len);
    result[len] = '\0';
    
    return result;
}

int extract_json_int(const char* json, const char* key) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    
    char* start = strstr(json, search_pattern);
    if (!start) return 0;
    
    start += strlen(search_pattern);
    return atoi(start);
}

bool extract_json_bool(const char* json, const char* key) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    
    char* start = strstr(json, search_pattern);
    if (!start) return false;
    
    start += strlen(search_pattern);
    return strstr(start, "true") == start;
}

int load_config_from_json(const char* filename, mining_config_t* config) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open config file %s\n", filename);
        return -1;
    }
    
    // Find file size and read entire file into buffer
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        printf("Error: Memory allocation failed\n");
        return -1;
    }
    
    size_t read_size = fread(buffer, 1, file_size, file);
    fclose(file);
    buffer[read_size] = '\0';
    
    // Parse JSON values
    char* temp;
    
    // Pool URL (required)
    temp = extract_json_string(buffer, "pool_url");
    if (temp) {
        strncpy(config->pool_url, temp, MAX_URL_LEN - 1);
        config->pool_url[MAX_URL_LEN - 1] = '\0';
        free(temp);
    } else {
        printf("Error: pool_url not found in config file\n");
        free(buffer);
        return -1;
    }
    
    // Pool port (required)
    config->pool_port = extract_json_int(buffer, "pool_port");
    if (config->pool_port <= 0) {
        printf("Error: Invalid or missing pool_port in config file\n");
        free(buffer);
        return -1;
    }
    
    // Username (required)
    temp = extract_json_string(buffer, "username");
    if (temp) {
        strncpy(config->username, temp, MAX_USER_LEN - 1);
        config->username[MAX_USER_LEN - 1] = '\0';
        free(temp);
    } else {
        printf("Error: username not found in config file\n");
        free(buffer);
        return -1;
    }
    
    // Password (optional, default "x")
    temp = extract_json_string(buffer, "password");
    if (temp) {
        strncpy(config->password, temp, MAX_PASS_LEN - 1);
        config->password[MAX_PASS_LEN - 1] = '\0';
        free(temp);
    } else {
        strcpy(config->password, "x"); // Default password
    }
    
    // YesPower version (optional, default 1.0)
    float version = 1.0;
    char* version_str = extract_json_string(buffer, "yespower_version");
    if (version_str) {
        version = atof(version_str);
        free(version_str);
    }
    
    if (version == 0.5) {
        config->yespower_params.version = YESPOWER_0_5;
    } else if (version == 1.0) {
        config->yespower_params.version = YESPOWER_1_0;
    } else {
        printf("Warning: Unrecognized version %.1f, using default 1.0\n", version);
        config->yespower_params.version = YESPOWER_1_0;
    }
    
    // YesPower N parameter (optional, default 2048)
    int N = extract_json_int(buffer, "yespower_N");
    if (N > 0) {
        config->yespower_params.N = N;
        if (config->yespower_params.N < 1024) {
            printf("Warning: N value too small, setting to 1024\n");
            config->yespower_params.N = 1024;
        }
    } else {
        config->yespower_params.N = 2048; // Default N value
    }
    
    // YesPower r parameter (optional, default 8)
    int r = extract_json_int(buffer, "yespower_r");
    if (r > 0) {
        config->yespower_params.r = r;
        if (config->yespower_params.r < 8) {
            printf("Warning: r value too small, setting to 8\n");
            config->yespower_params.r = 8;
        }
    } else {
        config->yespower_params.r = 8; // Default r value
    }
    
    // Default NULL settings for pers
    config->yespower_params.pers = NULL;
    config->yespower_params.perslen = 0;
    
    free(buffer);
    return 0;
}
