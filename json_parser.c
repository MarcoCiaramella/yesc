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
