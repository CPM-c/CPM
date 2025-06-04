// =============================================================================
// File: src/commands/add_command.cpp
// Description: Implementation of the 'cpm add' command.
// =============================================================================
#include "add_command.hpp" // Or "add_command.h"
#include "../utils/config_manager.h"  // Assuming these are C headers/implementations for now
#include "../utils/parsing_utils.h"   // or C++ compatible versions
#include "../utils/output_utils.h"
#include "../../include/cpm_types.h" // For ServerConfig

#include <cstdio>    // For printf, fprintf, stderr (from stdio.h)
#include <cstdlib>   // Forstdlib, abort, malloc, free (from stdlib.h)
#include <cstring>   // For strcmp, strlen, strdup (from string.h)
#include <getopt.h>  // For getopt_long (GNU extension, common on Linux)

// Note: output_utils.h/c uses printf, which is fine.
// If output_utils were C++, it might use iostream.

// Main function for the "add" command
// This is largely the C code from the previous immersive, with C++ headers.
int handle_add_command(int argc, char *argv[]) {
    const char *command_name_for_usage = "cpm add"; // For usage messages

    // Option parsing setup for getopt_long
    static const struct option long_options[] = {
        {"command", required_argument, NULL, 'c'},
        {"args",    required_argument, NULL, 'a'},
        {"env",     required_argument, NULL, 'e'},
        {NULL,      0,                 NULL, 0} // Terminator
    };

    char *name_arg = NULL;
    char *command_opt_str = NULL;
    char *args_opt_str = NULL;
    char *env_opt_str = NULL;
    int opt_char;
    int option_index = 0; 

    // Reset optind for getopt_long.
    optind = 1; 

    // Parse options
    while ((opt_char = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt_char) {
            case 'c': // --command
                command_opt_str = optarg;
                break;
            case 'a': // --args
                args_opt_str = optarg;
                break;
            case 'e': // --env
                env_opt_str = optarg;
                break;
            case '?': // Unrecognized option or missing argument
                print_info("Usage: %s <name> --command <executable> [--args \"arg1,arg2\"] [--env \"KEY1=VAL1,KEY2=VAL2\"]", command_name_for_usage);
                return 1; // Indicate failure
            default:
                print_error("Unexpected option parsing error.");
                std::abort(); // C++ equivalent of abort()
        }
    }

    // After options, the next non-option argument is the <name>
    if (optind < argc) {
        name_arg = argv[optind];
        optind++; 
    } else {
        print_error("Server/Task name is required.");
        print_info("Usage: %s <name> --command <executable> ...", command_name_for_usage);
        return 1;
    }

    if (optind < argc) {
        print_error("Too many arguments. Unexpected: %s", argv[optind]);
        print_info("Usage: %s <name> --command <executable> ...", command_name_for_usage);
        return 1;
    }
    
    if (!name_arg || std::strlen(name_arg) == 0) { 
        print_error("Server/Task name is critically missing (logic error).");
        return 1;
    }

    if (!command_opt_str || std::strlen(command_opt_str) == 0) {
        print_error("The --command option is required.");
        print_info("Usage: %s %s --command <executable> ...", command_name_for_usage, name_arg);
        return 1;
    }
    
    if (!ensure_config_initialized()) {
        print_error("Failed to initialize CPM configuration.");
        return 1;
    }

    ServerConfig *existing_server_ptr = get_server_by_name_from_config(name_arg);
    bool is_update = (existing_server_ptr != NULL);

    if (is_update) {
        print_warning("Configuration for \"%s\" already exists. It will be updated.", name_arg);
    }

    ServerConfig server_to_add = {0}; // Initialize all fields to NULL/0/false

    server_to_add.id = is_update ? strdup(existing_server_ptr->id) : generate_uuid_string();
    if (!server_to_add.id) {
        print_error("Failed to generate/assign ID for the configuration.");
        if (existing_server_ptr) free_server_config(existing_server_ptr); 
        return 1; 
    }

    // Using strdup (from <cstring>) which uses malloc. C++ way would be std::string.
    server_to_add.name = strdup(name_arg);
    server_to_add.command = strdup(command_opt_str);
    server_to_add.type = strdup("process"); 
    server_to_add.enabled = true;

    if (!server_to_add.name || !server_to_add.command || !server_to_add.type) {
        print_error("Memory allocation failed for server/task details.");
        // Free any parts that were successfully allocated for server_to_add
        std::free(server_to_add.id); 
        std::free(server_to_add.name); 
        std::free(server_to_add.command); 
        std::free(server_to_add.type);
        if (existing_server_ptr) free_server_config(existing_server_ptr);
        return 1;
    }
    
    if (args_opt_str) {
        server_to_add.args = parse_comma_separated_values(args_opt_str, &server_to_add.arg_count);
        if (args_opt_str && !server_to_add.args && server_to_add.arg_count > 0) { 
             print_error("Failed to parse arguments string or allocate memory for arguments.");
             // TODO: Perform full cleanup for server_to_add before returning
        }
    } else {
        server_to_add.args = NULL;
        server_to_add.arg_count = 0;
    }

    if (env_opt_str) {
        server_to_add.env_vars = parse_key_value_pairs(env_opt_str, &server_to_add.env_var_count);
         if (env_opt_str && !server_to_add.env_vars && server_to_add.env_var_count > 0) {
             print_error("Failed to parse environment variables string or allocate memory.");
             // TODO: Perform full cleanup
        }
    } else {
        server_to_add.env_vars = NULL;
        server_to_add.env_var_count = 0;
    }

    if (add_or_update_server_in_config(&server_to_add)) {
        print_success("Successfully %s configuration: %s", 
                      is_update ? "updated" : "added", server_to_add.name);
    } else {
        print_error("Failed to %s configuration: %s",
                    is_update ? "update" : "add", server_to_add.name);
    }

    // Clean up dynamically allocated members of stack-allocated server_to_add
    std::free(server_to_add.id);
    std::free(server_to_add.name);
    std::free(server_to_add.command);
    std::free(server_to_add.type);
    if (server_to_add.args) {
        for (int i = 0; i < server_to_add.arg_count; i++) std::free(server_to_add.args[i]);
        std::free(server_to_add.args);
    }
    if (server_to_add.env_vars) {
        for (int i = 0; i < server_to_add.env_var_count; i++) {
            std::free(server_to_add.env_vars[i].key);
            std::free(server_to_add.env_vars[i].value);
        }
        std::free(server_to_add.env_vars);
    }

    if (existing_server_ptr) {
        // free_server_config is expected to free the ServerConfig struct itself
        // and all its members. It's assumed to be from the C utils.
        free_server_config(existing_server_ptr); 
    }

    return 0; // Success
}

