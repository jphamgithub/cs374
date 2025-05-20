/*************************************************
 * Filename: phamjac_assignment4.c               
 * Author: Jacob Pham (phamjac)                  
 * Course: CS 374 Operating Systems              
 * Assignment: Programming Assignment 4 SMALLSH  
 *
 * Description:                                  
 *   this is a small shell implementation that can
 *   run built in commands or fork and exec new ones
 *   handle redirection, backgrounding, and signals
 *
 * Compile:                                      
 *   gcc --std=gnu99 -Wall -o smallsh phamjac_assignment4.c
 *
 * Run:                                          
 *   ./smallsh

Summary of each function's job

// main() --> the main shell loop that drives everything
//    └── calls setup_signals() --> installs custom handlers for Ctrl+C and Ctrl+Z
//    └── calls check_bg_procs() --> checks if background processes are done
//    └── calls parse_input() --> gets the next command from the user and parses it
//        └── returns struct used by main() to determine built-in vs external
//    └── handles built-in commands (exit, cd, status)
//    └── if external command, forks:
//           ├── in child:
//           │    └── sets signal behavior (SIGINT, SIGTSTP)
//           │    └── sets up redirection using dup2()
//           │    └── calls execvp() to run the command
//           └── in parent:
//                └── waits if foreground OR tracks PID if background
//    └── calls free_command() --> frees memory from parse_input

// handle_sigtstp() --> runs ONLY when Ctrl+Z is pressed, toggles fg_only_mode

// setup_signals() --> called once in main() to connect signals to handlers

// parse_input() --> used by main() every loop to get and structure the user’s command

// check_bg_procs() --> used by main() to non-blockingly clean up background jobs

// free_command() --> used by main() to free memory before looping again

// handle_builtin() --> handles "exit", "cd", and "status" directly (no fork)

// restore_child_signals() --> resets SIGINT and SIGTSTP to default behavior for child process

// setup_redirection() --> sets up stdin/stdout redirection or /dev/null if needed


Basic Tests to run when you run shell:

#ones i made
cd ..         # change directory
cd            # go to HOME
pwd           # verify directory changed
status        # check exit value of last foreground command
exit          # exits the shell

# normal ones
ls            # list files
echo hello    # should print "hello"
sleep 1       # test if it delays and exits cleanly

# redirection 
echo hello > out.txt         # write to file
cat < out.txt                # read from file
cat < out.txt > out2.txt     # input and output
diff out.txt out2.txt        # should be empty output (files match)

# backgrounding

sleep 5 &        # should print "background pid is ####"
sleep 1 &        # fast background job
ls &             # background ls
# Run these then wait ~5 sec and check:
# background pid #### is done: exit value 0

#Ctrl+z testing
sleep 5 &        # should background
<press Ctrl+Z>   # should print: "Entering foreground-only mode"
sleep 5 &        # should NOT background (runs in foreground)
<press Ctrl+Z>   # should print: "Exiting foreground-only mode"

#comments and blanks
# this is a comment
<press Enter>    # blank line
 *************************************************/


// ====================
// Required Libraries
// ====================

// standard libs
// need this for printf, fgets, fflush for prompt and output
#include <stdio.h>
// needed this for malloc, free, exit to handle memory and quitting
#include <stdlib.h>
// need this for parsing commands like strtok, strcmp, strdup
#include <string.h>
// core posix stuff for fork, execvp, chdir, getpid
#include <unistd.h>
// needed this once I started using pid_t for fork/waitpid
#include <sys/types.h>
// used for waitpid and checking child exit status
#include <sys/wait.h>
// needed this for true/false style flags like is_bg
#include <stdbool.h>

// i/o and process control
// needed this once I added input/output redirection using open
#include <fcntl.h>
// brought this in to handle signals like ctrl+c and ctrl+z
#include <signal.h>

// ====================
// Constants
// ====================

// defined in spec - max number of characters per input line
#define INPUT_LENGTH 2048
// defined in spec - max number of arguments per command
#define MAX_ARGS 512
// made this up - max background processes my shell will track
#define MAX_BG_PROCS 100


/**
 * Struct that holds all parsed parts of a user command.
 * We fill this in parse_input() then pass it to other logic.
 * Based on the starter parser in the assignment materials.
 */
struct command_line {
    char *argv[MAX_ARGS + 1];  // command + args, NULL-terminated, for execvp()
                               // example: "ls -l junk" → ["ls", "-l", "junk", NULL]
    int argc;                  // number of non-NULL arguments (not counting NULL at end)
                               // example: "cd folder" → argc == 2
    char *input_file;          // if user typed "< input.txt", this is "input.txt"
    char *output_file;         // if user typed "> output.txt", this is "output.txt"
    bool is_bg;                // true if user added '&' at end AND we're not in fg-only mode
};


// ====================
// Global State
// ====================

// added this first once I got status command working
// I needed to remember the exit value of the last foreground process
// so if someone runs "status", I can tell them what happened last
// ex: if last command was "ls", this might hold 0 or 1 depending on success
int last_fg_status = 0;

// added this later when I started handling ctrl+z (SIGTSTP)
// when this is true, & is ignored and everything runs in the foreground
// ex: user presses ctrl+z, then types "sleep 10 &" → runs as foreground
bool fg_only_mode = false;

// realized I needed this when I saw background processes could finish later
// this array keeps track of all the PIDs I forked into the background
// so I can later waitpid() on them and print when they finish
// ex: after "sleep 15 &", I save that pid here
// this is why I created  MAX_BG_PROCS
pid_t bg_pids[MAX_BG_PROCS];

// just needed a way to count how many valid bg pids I have saved
// I bump this every time I add a new one to bg_pids
int bg_count = 0;

// ====================
// Helper Functions
// ====================


/**
 * SIGTSTP handler (triggered by Ctrl+Z)
 * Toggles foreground-only mode on/off and prints a message
 * Doesn't kill the shell — just changes how we treat '&' at end of commands
 */
void handle_sigtstp(int signo) {
    
    // if we're NOT already in foreground-only mode...
    if (!fg_only_mode) {

        // build the message to show the user that backgrounding is now disabled
        // \n at the start makes it play nice if command was mid-line
        char *msg = "\nEntering foreground-only mode (& is now ignored)\n";

        // write directly to stdout (file descriptor 1)
        // I use write() instead of printf() because printf isn't safe inside signal handlers
        // printf() uses buffers that can mess up when interrupted
        // this is something the exploration specifically warned about ... use write() only
        write(STDOUT_FILENO, msg, strlen(msg));

        // flip the mode to true ... now backgrounding will be ignored
        fg_only_mode = true;

    } else {
        // same logic, but now we're turning foreground-only mode off

        // build message saying it's safe to background again
        char *msg = "\nExiting foreground-only mode\n";

        // write it safely using write(), just like above
        write(STDOUT_FILENO, msg, strlen(msg));

        // reset flag so background commands are honored again
        fg_only_mode = false;
    }
}

/**
 * Sets up how the shell responds to terminal signals
 * - Ignores SIGINT (Ctrl+C) so shell itself isn't killed
 * - Registers handle_sigtstp() for SIGTSTP (Ctrl+Z)
 */
void setup_signals() {
    // setup for SIGINT (Ctrl+C)
    // the shell itself should ignore Ctrl+C — we don't want the shell to close when the user hits it
    // only foreground child processes (like sleep or cat) should die on Ctrl+C
    struct sigaction sa_int = {0};        // zero-init the struct just in case
    sa_int.sa_handler = SIG_IGN;          // ignore the signal entirely in the parent shell
    sigaction(SIGINT, &sa_int, NULL);     // register the ignore handler for SIGINT

    // setup for SIGTSTP (Ctrl+Z)
    // when the user presses Ctrl+Z, we want to toggle foreground-only mode
    // this doesn't kill the shell — instead, it changes how & is handled
    struct sigaction sa_tstp = {0};       
    sa_tstp.sa_handler = handle_sigtstp;  // we tell the OS: run handle_sigtstp() when SIGTSTP happens
    sa_tstp.sa_flags = SA_RESTART;        // auto-restart things like fgets if interrupted (from exploration note)
    sigfillset(&sa_tstp.sa_mask);         // block other signals during handler to avoid conflicts
    sigaction(SIGTSTP, &sa_tstp, NULL);   // register this custom handler for SIGTSTP
}

/**
 * Gets user input, tokenizes it, and stores it in a command_line struct
 * Skips comment lines (starting with #) and blank lines
 * Handles input/output redirection and backgrounding '&'
 */
struct command_line *parse_input() {
    char input[INPUT_LENGTH]; // raw user input
    struct command_line *cmd = calloc(1, sizeof(struct command_line)); // alloc new struct

    printf(": "); // show prompt
    fflush(stdout); // flush so user sees prompt immediately
    fgets(input, INPUT_LENGTH, stdin); // get command line from user

    if (input[0] == '\n' || input[0] == '#') { // skip blanks and comments
        free(cmd);
        return NULL;
    }

    char *token = strtok(input, " \n"); // start tokenizing by space and newline
    while (token) {
        if (strcmp(token, "<") == 0) { // next token is input file
            cmd->input_file = strdup(strtok(NULL, " \n"));
        } else if (strcmp(token, ">") == 0) { // next token is output file
            cmd->output_file = strdup(strtok(NULL, " \n"));
        } else if (strcmp(token, "&") == 0 && strtok(NULL, " \n") == NULL) {
            cmd->is_bg = true; // & only means background if it's the last token
        } else {
            cmd->argv[cmd->argc++] = strdup(token); // normal argument
        }
        token = strtok(NULL, " \n"); // move to next word
    }
    return cmd;
}

/**
 * Checks for completed background processes
 * Prints exit status or signal of completed background jobs
 * Called at the top of every main loop iteration
 */
void check_bg_procs() {
    for (int i = 0; i < bg_count; i++) {
        int status;
        pid_t result = waitpid(bg_pids[i], &status, WNOHANG); // non-blocking check
        if (result > 0) {
            if (WIFEXITED(status)) {
                printf("background pid %d is done: exit value %d\n", result, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("background pid %d is done: terminated by signal %d\n", result, WTERMSIG(status));
            }
            fflush(stdout); // make sure it's printed
            bg_pids[i] = -1; // mark as cleaned up
        }
    }
}

/**
 * Frees all memory used by a command_line struct
 * Called after each loop in main once command is done
 */
void free_command(struct command_line *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    free(cmd->input_file);
    free(cmd->output_file);
    free(cmd);
}


/**
 * Handles built-in commands: exit, cd, and status
 * Returns true if the command was handled here (so main won't fork)
 */
bool handle_builtin(struct command_line *cmd) {
    // exit command - kill background children and exit
    if (strcmp(cmd->argv[0], "exit") == 0) {
        for (int i = 0; i < bg_count; i++) {
            if (bg_pids[i] > 0) {
                kill(bg_pids[i], SIGTERM); // politely ask bg procs to die
            }
        }
        exit(0); // quit the shell
    }

    // cd command - change directory
    else if (strcmp(cmd->argv[0], "cd") == 0) {
        // if no argument given, go to $HOME
        char *target = (cmd->argc > 1) ? cmd->argv[1] : getenv("HOME");
        if (chdir(target) != 0) {
            perror("cd"); // print error if chdir fails
        }
        return true;
    }

    // status command - report last foreground exit/signal
    else if (strcmp(cmd->argv[0], "status") == 0) {
        if (WIFEXITED(last_fg_status)) {
            printf("exit value %d\n", WEXITSTATUS(last_fg_status));
        } else if (WIFSIGNALED(last_fg_status)) {
            printf("terminated by signal %d\n", WTERMSIG(last_fg_status));
        }
        fflush(stdout);
        return true;
    }

    return false; // not a built-in command
}

/**
 * Used in child process to restore default signal behavior
 * SIGINT (Ctrl+C) should kill foreground children
 * SIGTSTP (Ctrl+Z) is ignored by child processes
 */
void restore_child_signals() {
    struct sigaction sa_default = {0};
    sa_default.sa_handler = SIG_DFL;

    // Ctrl+C will now kill child processes
    sigaction(SIGINT, &sa_default, NULL);

    // Ctrl+Z still ignored — only parent should toggle mode
    struct sigaction sa_ignore = {0};
    sa_ignore.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &sa_ignore, NULL);
}

/**
 * Sets up input/output redirection for a command
 * Uses dup2() to redirect stdin and stdout as needed
 * For background jobs with no redirection, sends to /dev/null
 */
void setup_redirection(struct command_line *cmd) {
    // handle input redirection
    if (cmd->input_file) {
        int input_fd = open(cmd->input_file, O_RDONLY);
        if (input_fd == -1) {
            perror("cannot open input file");
            exit(1);
        }
        dup2(input_fd, STDIN_FILENO); // redirect stdin
        close(input_fd);
    } else if (cmd->is_bg && !fg_only_mode) {
        // background process with no input file → /dev/null
        int devnull = open("/dev/null", O_RDONLY);
        dup2(devnull, STDIN_FILENO);
        close(devnull);
    }

    // handle output redirection
    if (cmd->output_file) {
        int output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd == -1) {
            perror("cannot open output file");
            exit(1);
        }
        dup2(output_fd, STDOUT_FILENO); // redirect stdout
        close(output_fd);
    } else if (cmd->is_bg && !fg_only_mode) {
        // background process with no output file → /dev/null
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        close(devnull);
    }
}

/**
 * main loop of the shell
 * sets up signals, waits for user commands, and runs them
 * built-ins handled directly (cd, exit, status)
 * all other commands fork off child and use execvp
 * background processes tracked separately
 */
int main() {
    // first thing we do - install signal handlers
    // this makes sure the shell ignores ctrl+c (SIGINT)
    // and uses our custom toggle handler for ctrl+z (SIGTSTP)
    setup_signals();

    // loop forever until user types "exit"
    // every loop = one user command
    while (1) {
        // before asking for a command, check if any background jobs finished
        // this prints something like:
        // "background pid 1234 is done: exit value 0"
        check_bg_procs();

        // show the : prompt, grab what the user typed,
        // and break it into tokens like cmd, args, redirection, &
        struct command_line *cmd = parse_input();

        // if the user just hit enter or typed a comment, skip this loop
        if (!cmd || cmd->argc == 0) {
            free_command(cmd); // still clean up any memory
            continue;
        }

        // check if the command is one of the built-ins: exit, cd, or status
        // if it is, we handle it right away without forking
        if (handle_builtin(cmd)) {
            free_command(cmd); // always clean up the struct
            continue;          // back to the top of the loop
        }

        // if we got here, it's not a built-in command (like "ls", "sleep", etc)
        // so we need to fork a new child process to run it
        pid_t spawnpid = fork();

        // child process (this is where we run the actual command)
        if (spawnpid == 0) {
            // let child handle ctrl+c normally again
            // so it can be killed if it's running in foreground
            restore_child_signals();

            // do any input/output redirection (like < or >)
            // this runs dup2() and opens files as needed
            setup_redirection(cmd);

            // now try to run the command using execvp
            // this replaces the current process with the command (if it works)
            execvp(cmd->argv[0], cmd->argv);

            // if we got here, exec failed (like "badfile" or command not found)
            // print the error and exit with code 1 so parent can see it
            perror(cmd->argv[0]);
            exit(1);
        }

        // parent process (this is still the shell)
        else if (spawnpid > 0) {
            // if the command ended with & and we're not in fg-only mode
            if (cmd->is_bg && !fg_only_mode) {
                // background mode - don't wait
                // just print the background pid and save it to track later
                printf("background pid is %d\n", spawnpid);
                fflush(stdout);
                bg_pids[bg_count++] = spawnpid;
            } else {
                // foreground mode - must wait for child to finish
                // this blocks until the command is done
                waitpid(spawnpid, &last_fg_status, 0);

                // if the foreground command was killed by a signal (like ctrl+c),
                // show which signal caused it (ex: "terminated by signal 2")
                if (WIFSIGNALED(last_fg_status)) {
                    printf("terminated by signal %d\n", WTERMSIG(last_fg_status));
                    fflush(stdout);
                }
            }
        }

        // always clean up the command struct after every loop
        free_command(cmd);
    }

    // we'll only reach here if user typed "exit"
    return EXIT_SUCCESS;
}
/*** end of file ***/