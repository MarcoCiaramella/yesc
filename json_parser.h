#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "stratum_client.h"

// Function prototypes for JSON parsing
int parse_subscribe_response(const char* json, stratum_job_t* job);
int parse_authorize_response(const char* json);
int parse_job_notification(const char* json, stratum_job_t* job);
char* extract_json_string(const char* json, const char* key);
int extract_json_int(const char* json, const char* key);
bool extract_json_bool(const char* json, const char* key);

// Configuration loading function
int load_config_from_json(const char* filename, mining_config_t* config);
void nbits_to_target(const char* nbits_hex, char* target_hex, size_t target_size);

#endif
