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
    // Simplified parsing of mining.notify message
    // This should parse: job_id, prevhash, coinb1, coinb2, merkle_branches,
    // version, nbits, ntime, clean_jobs
    
    // For demonstration, setting some default values
    strcpy(job->job_id, "test_job");
    strcpy(job->prevhash, "0000000000000000000000000000000000000000000000000000000000000000");
    strcpy(job->coinb1, "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff");
    strcpy(job->coinb2, "ffffffff0100f2052a01000000434104");
    strcpy(job->version, "00000001");
    strcpy(job->nbits, "1d00ffff");
    strcpy(job->ntime, "4f8b1c5a");
    strcpy(job->target, "0000ffff00000000000000000000000000000000000000000000000000000000");
    job->clean_jobs = true;
    job->merkle_count = 0;
    
    return 0;
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

int load_config_from_json(const char* filename, stratum_config_t* config) {
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
