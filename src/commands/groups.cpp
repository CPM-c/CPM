#include "groups_command.hpp"
#include "../utils/config_manager.h"  // For group manipulation functions
#include "../utils/output_utils.h"    // For printing messages
#include "../utils/parsing_utils.h"   // For parse_comma_separated_values
#include "../../include/cpm_types.h"  // For GroupConfig, FullConfig (if used for listing)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>  // For getopt_long

// Forward declarations for subcommand handlers
static int do_add_group(int argc, char *argv[]);
static int do_remove_group(int argc, char *argv[]);
static int do_list_groups(int argc, char *argv[]); // argc, argv might not be needed if no options

// Main dispatcher for "cpm groups"
int handle_groups_command(int argc, char *argv[]) {
    if (argc < 1) { // Should have at least one arg: the subcommand
        print_error("No subcommand specified for 'groups'.");
        print_info("Usage: cpm groups <add|remove|list> [options]");
        return 1;
    }

    const char *subcommand = argv[0];

    if (strcmp(subcommand, "add") == 0) {
        return do_add_group(argc - 1, argv + 1);
    } else if (strcmp(subcommand, "remove") == 0) {
        return do_remove_group(argc - 1, argv + 1);
    } else if (strcmp(subcommand, "list") == 0) {
        return do_list_groups(argc - 1, argv + 1);
    } else {
        print_error("Unknown subcommand for 'groups': %s", subcommand);
        print_info("Usage: cpm groups <add|remove|list> [options]");
        return 1;
    }
    return 0; // Should be unreachable
}

// Handler for "cpm groups add <group-name> --servers <server-list>"
static int do_add_group(int argc, char *argv[]) {
    const char *command_name_for_usage = "cpm groups add";
    char *group_name_arg = NULL;
    char *servers_opt_str = NULL;

    static const struct option long_options[] = {
        {"servers", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0}
    };

    optind = 1; // Reset for getopt_long
    int opt_char;

    while ((opt_char = getopt_long(argc, argv, "s:", long_options, NULL)) != -1) {
        switch (opt_char) {
            case 's': // --servers
                servers_opt_str = optarg;
                break;
            case '?':
                print_info("Usage: %s <group-name> --servers \"server1,server2,...\"", command_name_for_usage);
                return 1;
            default:
                std::abort();
        }
    }

    // Group name is the positional argument
    if (optind < argc) {
        group_name_arg = argv[optind];
        optind++;
    } else {
        print_error("Group name is required.");
        print_info("Usage: %s <group-name> --servers <server-list>", command_name_for_usage);
        return 1;
    }
     if (optind < argc) { // Check for extra positional arguments
        print_error("Too many arguments. Unexpected: %s", argv[optind]);
        print_info("Usage: %s <group-name> --servers <server-list>", command_name_for_usage);
        return 1;
    }


    if (!group_name_arg || strlen(group_name_arg) == 0) { // Should be caught above
        print_error("Group name is critically missing.");
        return 1;
    }

    if (!servers_opt_str || strlen(servers_opt_str) == 0) {
        print_error("Server list is required (--servers).");
        print_info("Usage: %s %s --servers \"server1,server2,...\"", command_name_for_usage, group_name_arg);
        return 1;
    }

    int server_count = 0;
    // Casting const char* to char* for parse_comma_separated_values if it modifies its input,
    // which it shouldn't. If it takes const char*, then no cast needed.
    // Assuming parse_comma_separated_values takes const char*
    char **server_list = parse_comma_separated_values(servers_opt_str, &server_count);

    if (!server_list && server_count == -1) { // Let's say -1 indicates parsing error
        print_error("Failed to parse server list.");
        return 1;
    }
    if (server_count == 0) {
        print_error("Server list cannot be empty.");
        // free_string_array(server_list, server_count); // if server_list can be non-NULL for count 0
        if (server_list) free(server_list); // Simplistic free if it's just one block
        return 1;
    }
    
    // Convert char** to const char** for the config manager function
    // This is safe if add_or_update_group_in_config doesn't modify the strings.
    const char** const_server_list = (const char**)server_list;


    if (add_or_update_group_in_config(group_name_arg, const_server_list, server_count)) {
        // Construct server list string for display
        char display_servers[1024] = "";
        for(int i=0; i<server_count; ++i) {
            strncat(display_servers, server_list[i], sizeof(display_servers) - strlen(display_servers) -1);
            if (i < server_count - 1) {
                strncat(display_servers, ", ", sizeof(display_servers) - strlen(display_servers) -1);
            }
        }
        print_success("Group '%s' added/updated with servers: %s", group_name_arg, display_servers);
    } else {
        print_error("Failed to add/update group '%s'", group_name_arg);
    }

    // Clean up server_list (array of strings and the array itself)
    if (server_list) {
        for (int i = 0; i < server_count; ++i) {
            free(server_list[i]); // Free each string
        }
        free(server_list); // Free the array of pointers
    }
    return 0;
}

// Handler for "cpm groups remove <group-name>"
static int do_remove_group(int argc, char *argv[]) {
    const char *command_name_for_usage = "cpm groups remove";
    if (argc < 1) {
        print_error("Group name is required for remove.");
        print_info("Usage: %s <group-name>", command_name_for_usage);
        return 1;
    }
    if (argc > 1) {
        print_error("Too many arguments for remove.");
        print_info("Usage: %s <group-name>", command_name_for_usage);
        return 1;
    }

    const char *group_name_arg = argv[0];

    if (remove_group_from_config(group_name_arg)) {
        print_success("Group '%s' removed.", group_name_arg);
    } else {
        print_error("Group '%s' not found or failed to remove.", group_name_arg);
    }
    return 0;
}

// Handler for "cpm groups list"
static int do_list_groups(int argc, char *argv[]) {
    const char *command_name_for_usage = "cpm groups list";
     if (argc > 0) { // list command should not have positional arguments
        print_error("Too many arguments for list. Unexpected: %s", argv[0]);
        print_info("Usage: %s", command_name_for_usage);
        return 1;
    }

    // This assumes groups are read as part of FullConfig.
    // If there's a direct get_all_groups, the logic would be simpler.
    FullConfig* full_config = read_full_config_from_manager(); 
    if (!full_config) {
        print_error("Failed to read configuration to list groups.");
        return 1;
    }

    if (!full_config->groups || full_config->group_config_count == 0) {
        print_warning("No server groups defined."); // chalk.yellow
        // free_full_config(full_config); // Important to free if allocated
        return 0;
    }

    print_info("Server Groups:"); // chalk.bold

    for (int i = 0; i < full_config->group_config_count; ++i) {
        GroupConfig* group = &full_config->groups[i];
        // print_info("\n%s:", group->name); // chalk.cyan for name
        char group_name_line[256];
        snprintf(group_name_line, sizeof(group_name_line), "\n%s:", group->name);
        print_info(group_name_line);


        if (group->server_names && group->server_count > 0) {
            for (int j = 0; j < group->server_count; ++j) {
                char server_line[256];
                snprintf(server_line, sizeof(server_line), "  - %s", group->server_names[j]);
                print_info(server_line);
            }
        } else {
            print_warning("  No servers in this group"); // chalk.yellow
        }
    }
    print_info(""); // Empty line at the end

    // free_full_config(full_config); // Free the read configuration
    return 0;
}

