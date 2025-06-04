// Conceptual C++ Code Structure for 'cpm config' command (initial C-style port)

// =============================================================================
// File: include/cpm_types.h (or cpm_types.hpp) - Additions needed
// Description: Defines common data structures for CPM.
// We'll need to ensure this file (or its C++ equivalent) can represent
// servers, groups, and the overall configuration.
// =============================================================================
/*
// --- Additions/Modifications to cpm_types.h (conceptual) ---
#ifndef CPM_TYPES_H
#define CPM_TYPES_H

#include <stdbool.h>

typedef struct {
    char *key;
    char *value;
} KeyValuePair;

typedef struct {
    char *id;
    char *name;
    char *command;
    char **args;
    int arg_count;
    KeyValuePair *env_vars;
    int env_var_count;
    bool enabled;
    char *type;
} ServerConfig;

typedef struct {
    char *name;          // Name of the group
    char **server_names; // Array of server names belonging to this group
    int server_count;    // Number of servers in this group
} GroupConfig;

typedef struct {
    ServerConfig *servers;    // Array of server configurations
    int server_config_count;  // Number of server configurations
    GroupConfig *groups;      // Array of group configurations
    int group_config_count;   // Number of group configurations
    // Paths might be managed internally by config_manager or stored here
    // char* current_load_path;
    // char* current_save_path;
} FullConfig;

void free_server_config(ServerConfig *config); // Assumed to exist
void free_group_config(GroupConfig *config);   // Needs to be defined
void free_full_config(FullConfig *config);     // Needs to be defined

#endif // CPM_TYPES_H
