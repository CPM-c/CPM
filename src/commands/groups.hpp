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

// Represents a single group configuration
typedef struct {
    char *name;          // Name of the group
    char **server_names; // Array of server names (strings) belonging to this group
    int server_count;    // Number of servers in this group
} GroupConfig;

// Represents the entire configuration file content
typedef struct {
    ServerConfig *servers;    // Array of server configurations
    int server_config_count;  // Number of server configurations
    GroupConfig *groups;      // Array of group configurations
    int group_config_count;   // Number of group configurations
} FullConfig;

// Memory management functions
void free_server_config(ServerConfig *config); // Assumed to exist from 'add' command
void free_key_value_pair_array(KeyValuePair *pairs, int count); // If KeyValuePair arrays are managed
void free_string_array(char **arr, int count); // For ServerConfig->args and GroupConfig->server_names

void free_group_config_members(GroupConfig *group_config); // Frees members of a GroupConfig
void free_group_config(GroupConfig *group_config);         // Frees GroupConfig struct and its members

void free_full_config_members(FullConfig *full_config);    // Frees members of a FullConfig
void free_full_config(FullConfig *full_config);            // Frees FullConfig struct and its members

#endif // CPM_TYPES_H
*/

// =============================================================================
// File: src/utils/config_manager.h (or config_manager.hpp) - New functions for groups
// Description: Header for configuration management utilities.
// =============================================================================
/*
// --- New function signatures needed in config_manager.h (conceptual) ---
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "../../include/cpm_types.h" // For FullConfig, ServerConfig, GroupConfig etc.
#include <stdbool.h>

// ... existing functions ...
// void set_custom_load_path_in_config_manager(const char* path);
// void set_custom_save_path_in_config_manager(const char* path);
// FullConfig* read_full_config_from_manager();
// bool write_full_config_to_manager(const FullConfig* config);
// bool ensure_config_initialized();
// ServerConfig* get_server_by_name_from_config(const char *name);
// bool add_or_update_server_in_config(const ServerConfig *server);
// char* generate_uuid_string();


// --- Group specific functions ---

// Adds a new group or updates an existing one.
// name: Name of the group.
// servers: Array of server name strings.
// server_count: Number of servers in the array.
// Returns true on success, false on failure.
bool add_or_update_group_in_config(const char* group_name, const char** server_names, int server_count);

// Removes a group by its name.
// Returns true if the group was found and removed, false otherwise.
bool remove_group_from_config(const char* group_name);

// Retrieves all groups.
// Returns a dynamically allocated array of GroupConfig.
// Sets 'group_count_out' to the number of groups retrieved.
// Caller is responsible for freeing the array and its contents.
// This might be part of read_full_config_from_manager() if FullConfig is used.
// For direct access, a separate function could be:
// GroupConfig* get_all_groups(int* group_count_out); 
// Or, more likely, groups are read as part of FullConfig.

#endif // CONFIG_MANAGER_H


// =============================================================================
// File: src/commands/groups_command.hpp
// Description: Header for the 'groups' command handler.
// =============================================================================
#ifndef GROUPS_COMMAND_HPP
#define GROUPS_COMMAND_HPP

/**
 * Handles the "cpm groups" subcommands (add, remove, list).
 * Parses arguments from argv.
 * Expected:
 * cpm groups add <group-name> --servers "server1,server2"
 * cpm groups remove <group-name>
 * cpm groups list
 * argc and argv are relative to the "groups" command itself.
 * (e.g., argv[0] is "add", "remove", or "list")
 * Returns 0 on success, non-zero on failure.
 */
int handle_groups_command(int argc, char *argv[]);

#endif // GROUPS_COMMAND_HPP
