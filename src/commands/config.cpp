*/

// =============================================================================
// File: src/utils/config_manager.h (or config_manager.hpp) - New functions needed
// Description: Header for configuration management utilities.
// =============================================================================
/*
// --- New function signatures needed in config_manager.h (conceptual) ---
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "../../include/cpm_types.h" // For FullConfig, ServerConfig etc.
#include <stdbool.h>

// ... existing functions like get_server_by_name_from_config, etc. ...

// Sets the custom path from which to load configurations.
// This path should be persisted or used for subsequent reads.
void set_custom_load_path_in_config_manager(const char* path);

// Sets the custom path to which configurations will be saved.
// This path should be persisted or used for subsequent writes.
void set_custom_save_path_in_config_manager(const char* path);

// Reads the entire current configuration (servers, groups).
// The caller is responsible for freeing the returned FullConfig struct
// using free_full_config().
FullConfig* read_full_config_from_manager();

// Writes the given FullConfig to the currently set save path (or default).
// Returns true on success, false on failure.
bool write_full_config_to_manager(const FullConfig* config);

// (Optional) Functions to get current effective paths
// const char* get_effective_load_path();
// const char* get_effective_save_path();

#endif // CONFIG_MANAGER_H
*/


// =============================================================================
// File: src/commands/config_command.hpp
// Description: Header for the 'config' command handler.
// =============================================================================
#ifndef CONFIG_COMMAND_HPP
#define CONFIG_COMMAND_HPP

/**
 * Handles the "cpm config" command.
 * Parses arguments from argv.
 * Expected:
 * cpm config
 * cpm config --load <path>
 * cpm config --save <path>
 * argc and argv are relative to the "config" command itself.
 * Returns 0 on success, non-zero on failure.
 */
int handle_config_command(int argc, char *argv[]);

#endif // CONFIG_COMMAND_HPP


// =============================================================================
// File: src/commands/config_command.cpp
// Description: Implementation of the 'cpm config' command.
// =============================================================================
#include "config_command.hpp"
#include "../utils/config_manager.h"  // Assuming C or C++ compatible
#include "../utils/output_utils.h"    // Assuming C or C++ compatible
#include "../../include/cpm_types.h"  // For FullConfig, ServerConfig, GroupConfig

#include <cstdio>    // For printf, file operations (from stdio.h)
#include <cstdlib>   // For realpath, free (from stdlib.h)
#include <cstring>   // For strcmp (from string.h)
#include <getopt.h>  // For getopt_long
#include <sys/stat.h> // For stat to check file existence
#include <limits.h>   // For PATH_MAX (though realpath handles allocation if NULL)


// Helper function to display the configuration
// This function assumes `output_utils.h` provides print_info, print_success etc.
// and that `FullConfig` and its members are defined in `cpm_types.h`.
static void display_current_configuration() {
    print_info("Current configuration:"); // chalk.blue
    
    FullConfig* config = read_full_config_from_manager(); // Assumes this function exists
    if (!config) {
        print_error("Could not read configuration.");
        return;
    }

    // Display Servers
    print_info("Servers:"); // chalk.bold
    if (config->servers && config->server_config_count > 0) {
        for (int i = 0; i < config->server_config_count; ++i) {
            ServerConfig* server = &config->servers[i];
            // Using print_info for sub-details, adjust formatting as needed
            // Mimicking the chalk.green(server.name) style might require more complex output_utils
            char server_details[1024]; // Buffer for formatted string

            snprintf(server_details, sizeof(server_details), "  Name: %s", server->name /*, server->enabled ? "(enabled)" : "(disabled)"*/);
            print_info(server_details); // Would be better if print_success could be used for name

            snprintf(server_details, sizeof(server_details), "    Command: %s", server->command ? server->command : "none");
            print_info(server_details);

            // Format args
            char args_str[512] = "none";
            if (server->args && server->arg_count > 0) {
                args_str[0] = '\0'; // Clear "none"
                for (int j = 0; j < server->arg_count; ++j) {
                    strncat(args_str, server->args[j], sizeof(args_str) - strlen(args_str) - 1);
                    if (j < server->arg_count - 1) {
                        strncat(args_str, ", ", sizeof(args_str) - strlen(args_str) - 1);
                    }
                }
            }
            snprintf(server_details, sizeof(server_details), "    Args: %s", args_str);
            print_info(server_details);

            // Format env (very simplified, JS uses JSON.stringify)
            // For C++, this would require iterating KeyValuePairs and formatting them.
            // This is a complex part to replicate exactly without a JSON library.
            char env_str[512] = "none";
            if (server->env_vars && server->env_var_count > 0) {
                 snprintf(env_str, sizeof(env_str), "{...} (%d vars)", server->env_var_count); // Placeholder
            }
            snprintf(server_details, sizeof(server_details), "    Env: %s", env_str);
            print_info(server_details);
        }
    } else {
        print_info("  No servers configured");
    }

    // Display Groups
    print_info("Groups:"); // chalk.bold
    if (config->groups && config->group_config_count > 0) {
        for (int i = 0; i < config->group_config_count; ++i) {
            GroupConfig* group = &config->groups[i];
            char group_servers_str[512] = "none";
            if (group->server_names && group->server_count > 0) {
                group_servers_str[0] = '\0';
                for (int j = 0; j < group->server_count; ++j) {
                    strncat(group_servers_str, group->server_names[j], sizeof(group_servers_str) - strlen(group_servers_str) -1);
                    if (j < group->server_count - 1) {
                        strncat(group_servers_str, ", ", sizeof(group_servers_str) - strlen(group_servers_str) - 1);
                    }
                }
            }
            char group_details[1024];
            snprintf(group_details, sizeof(group_details), "  %s: %s", group->name, group_servers_str);
            print_info(group_details); // Again, group name could be highlighted
        }
    } else {
        print_info("  No groups configured");
    }

    // free_full_config(config); // Assumes this function exists to clean up
                               // It should handle freeing arrays of servers, groups, and their contents.
}


int handle_config_command(int argc, char *argv[]) {
    const char *command_name_for_usage = "cpm config";

    static const struct option long_options[] = {
        {"load", required_argument, NULL, 'l'},
        {"save", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0} // Terminator
    };

    char *load_path_arg = NULL;
    char *save_path_arg = NULL;
    int opt_char;
    int option_index = 0;
    bool action_taken = false; // To track if --load or --save was used

    optind = 1; // Reset optind for getopt_long

    while ((opt_char = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt_char) {
            case 'l': // --load
                load_path_arg = optarg;
                action_taken = true;
                break;
            case 's': // --save
                save_path_arg = optarg;
                action_taken = true;
                break;
            case '?': // Unrecognized option or missing argument
                print_info("Usage: %s [--load <path>] [--save <path>]", command_name_for_usage);
                return 1; // Indicate failure
            default:
                print_error("Unexpected option parsing error.");
                std::abort();
        }
    }

    // Handle --load option
    if (load_path_arg) {
        char resolved_load_path[PATH_MAX];
        if (realpath(load_path_arg, resolved_load_path) == NULL) {
            // realpath fails if the path doesn't exist (except for the last component).
            // The JS version allows non-existent paths and creates them later.
            // For simplicity, we'll try to resolve, but if it fails, use the original.
            // A more robust solution might involve creating parent directories.
            strncpy(resolved_load_path, load_path_arg, PATH_MAX -1);
            resolved_load_path[PATH_MAX-1] = '\0';
            print_warning("Could not fully resolve load path '%s'. Using as is.", load_path_arg);
        }
        
        set_custom_load_path_in_config_manager(resolved_load_path); // Assumes this function exists

        struct stat st;
        if (stat(resolved_load_path, &st) != 0) {
            // File does not exist
            print_warning("Config file %s does not exist. It will be created when needed.", resolved_load_path);
        } else {
            print_success("Now loading configuration from: %s", resolved_load_path);
        }
    }

    // Handle --save option
    if (save_path_arg) {
        char resolved_save_path[PATH_MAX];
         if (realpath(save_path_arg, resolved_save_path) == NULL) {
            // Similar to load, if path doesn't fully exist, realpath might fail.
            // We might want to resolve the directory part and append the filename.
            // For now, using the provided path or a simplified resolution.
            // A robust approach: dirname(save_path_arg) -> resolve -> append basename(save_path_arg)
            strncpy(resolved_save_path, save_path_arg, PATH_MAX -1);
            resolved_save_path[PATH_MAX-1] = '\0';
            print_warning("Could not fully resolve save path '%s'. Using as is. Ensure parent directory exists.", save_path_arg);
        }

        set_custom_save_path_in_config_manager(resolved_save_path); // Assumes this function exists
        print_success("Now saving configuration to: %s", resolved_save_path);

        // If we have a config loaded, save it to the new path
        FullConfig* current_config = read_full_config_from_manager(); // Assumes this exists
        if (current_config) {
            if (write_full_config_to_manager(current_config /*, resolved_save_path */)) { 
                // write_full_config_to_manager should use the path set by set_custom_save_path...
                print_success("Configuration saved to new location.");
            } else {
                print_error("Failed to save configuration to new location: %s", resolved_save_path);
            }
            // free_full_config(current_config); // Clean up after reading
        } else {
            print_warning("No current configuration to save, or failed to read it.");
        }
    }

    // If no options were given, display current config
    if (!action_taken) {
        if (optind < argc) { // Check for unexpected positional arguments
            print_error("Unexpected argument: %s", argv[optind]);
            print_info("Usage: %s [--load <path>] [--save <path>]", command_name_for_usage);
            return 1;
        }
        display_current_configuration();
    }

    return 0; // Success
}

