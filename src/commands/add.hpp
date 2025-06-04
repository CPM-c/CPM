// Conceptual C++ Code Structure for 'cpm add' command (initial C-style port)

// =============================================================================
// File: src/commands/add_command.hpp (or add_command.h)
// Description: Header for the 'add' command handler.
// =============================================================================
#ifndef ADD_COMMAND_HPP // Or ADD_COMMAND_H if you prefer .h
#define ADD_COMMAND_HPP

/**
 * Handles the "cpm add" command.
 * Parses arguments from argv.
 * Expected: cpm add <name> --command <command> [--args <args>] [--env <env>]
 * argc and argv are relative to the "add" command itself
 * (e.g., argv[0] might be "add").
 * Returns 0 on success, non-zero on failure.
 */
int handle_add_command(int argc, char *argv[]);

#endif // ADD_COMMAND_HPP
