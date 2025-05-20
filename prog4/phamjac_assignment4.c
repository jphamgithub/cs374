/*************************************************
 * Filename: phamjac_assignment4.c               
 * Author: Jacob Pham (phamjac)                  
 * Course: CS 374 Operating Systems              
 * Assignment: Programming Assignment 4 SMALLSH  
 *
 * Description:                                  
 *   this is a small shell implementation that can
 *   run built-in commands or fork+exec new ones,
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


 Tests to run
# Basic Commands & Output Redirection
ls
ls > junk
status
cat junk
wc < junk > junk2
wc < junk

# Error Cases (Bad Files, Failures)
test -f badfile
status
wc < badfile
status
badfile

# Foreground Command + Ctrl+C Test
sleep 5
# then press Ctrl+C
status &

# Background Process
sleep 15 &
ps
# wait ~15 sec and you should see:
# background pid #### is done: exit value 0


# Comment and Blank Line Handling
# (press Enter)
# that was a blank command line, this is a comment line


# Background Kill and Signal Reporting
sleep 30 &
# note the PID it prints (e.g., 4941)
kill -15 4941
# should show: background pid 4941 is done: terminated by signal 15


#Other
pwd
cd
pwd
cd CS344
pwd
echo 4867

# press Ctrl+Z
date
sleep 5 &
date
# press Ctrl+Z again
date
sleep 5 &

and EXIT!!!

exit

 *************************************************/

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

// defined in spec - max number of characters per input line
#define INPUT_LENGTH 2048
// defined in spec - max number of arguments per command
#define MAX_ARGS 512
// made this up - max background processes my shell will track
#define MAX_BG_PROCS 100


/** 
this struct holds everything about one line the user typed
I based it on the sample parser from the assignment
I need it so I can parse the command once and pass it around cleanly
command line struct adapted from the sample parser in the assignment
**/
struct command_line {
    char *argv[MAX_ARGS + 1];  // argument list to pass to execvp
                               // ex: if user types "ls -l junk", this becomes ["ls", "-l", "junk", NULL]

    int argc;                  // number of actual arguments (not counting NULL)
                               // ex: for "cd folder", argc == 2

    char *input_file;          // if user types "< input.txt", this becomes "input.txt"
                               // ex: for "sort < unsorted.txt", input_file == "unsorted.txt"

    char *output_file;         // if user types "> output.txt", this becomes "output.txt"
                               // ex: for "ls > junk", output_file == "junk"

    bool is_bg;                // true if user ends command with "&" and fg-only mode is off
                               // ex: for "sleep 10 &", is_bg == true
};

// global values to manage shell behavior

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

/**
this function runs when user presses ctrl+z (aka SIGTSTP)
it turns foreground-only mode on or off, depending on the current state
I set this function as the handler using sigaction in setup_signals()
this idea came from the Signal Handling API exploration
**/
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
sets up how the shell handles terminal signals
specifically:
- ignore SIGINT (Ctrl+C) in the shell itself so it doesn’t kill the shell
- catch SIGTSTP (Ctrl+Z) and call handle_sigtstp() to toggle foreground-only mode

this setup is done once at the start of main()
the idea and syntax came from the Signal Handling API exploration
**/
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

// parse user input into structured command (based on parser provided in assignment)
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

// check for background processes that finished
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

// free memory from parse_input result
void free_command(struct command_line *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    free(cmd->input_file);
    free(cmd->output_file);
    free(cmd);
}

/**
 * main loop of the shell
 * sets up signals, waits for user commands, and runs them
 * built-ins handled directly (cd, exit, status)
 * all other commands fork off child and use execvp
 * background processes tracked separately
 */
int main() {
    setup_signals(); // set signal handling for shell process

    while (1) { // main shell loop
        check_bg_procs(); // clean up finished bg processes
        struct command_line *cmd = parse_input(); // get next command
        if (!cmd) continue; // skip blank or comment lines

        if (cmd->argc == 0) { // ignore empty command
            free_command(cmd);
            continue;
        }

        // check for built-in commands
        if (strcmp(cmd->argv[0], "exit") == 0) {
            for (int i = 0; i < bg_count; i++) {
                if (bg_pids[i] > 0) kill(bg_pids[i], SIGTERM); // clean up all bg procs
            }
            free_command(cmd);
            break; // end shell

        } else if (strcmp(cmd->argv[0], "cd") == 0) {
            char *target = cmd->argc > 1 ? cmd->argv[1] : getenv("HOME"); // go to arg or $HOME
            chdir(target); // change dir (covered in Environment exploration)
            free_command(cmd);
            continue;

        } else if (strcmp(cmd->argv[0], "status") == 0) {
            if (WIFEXITED(last_fg_status)) {
                printf("exit value %d\n", WEXITSTATUS(last_fg_status));
            } else {
                printf("terminated by signal %d\n", WTERMSIG(last_fg_status));
            }
            fflush(stdout);
            free_command(cmd);
            continue;
        }

        // run non built-in command
        pid_t spawnpid = fork(); // create child process
        if (spawnpid == 0) { // child logic
            struct sigaction sa_default = {0};
            sa_default.sa_handler = SIG_DFL;
            sigaction(SIGINT, &sa_default, NULL); // restore ctrl-c

            struct sigaction sa_ignore = {0};
            sa_ignore.sa_handler = SIG_IGN;
            sigaction(SIGTSTP, &sa_ignore, NULL); // child ignores ctrl-z

            if (cmd->input_file) {
                int input_fd = open(cmd->input_file, O_RDONLY); // open for reading
                if (input_fd == -1) {
                    perror("cannot open input file");
                    exit(1);
                }
                dup2(input_fd, 0); // redirect stdin (Processes and I/O exploration)
            } else if (cmd->is_bg && !fg_only_mode) {
                int devnull = open("/dev/null", O_RDONLY);
                dup2(devnull, 0);
            }

            if (cmd->output_file) {
                int output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); // write
                if (output_fd == -1) {
                    perror("cannot open output file");
                    exit(1);
                }
                dup2(output_fd, 1); // redirect stdout
            } else if (cmd->is_bg && !fg_only_mode) {
                int devnull = open("/dev/null", O_WRONLY);
                dup2(devnull, 1);
            }

            execvp(cmd->argv[0], cmd->argv); // run command
            perror(cmd->argv[0]); // if we get here, exec failed
            exit(1);
        }
        else if (spawnpid > 0) { // parent
            if (cmd->is_bg && !fg_only_mode) {
                printf("background pid is %d\n", spawnpid);
                fflush(stdout);
                bg_pids[bg_count++] = spawnpid; // track bg pid
            } else {
                waitpid(spawnpid, &last_fg_status, 0); // block until child done
                if (WIFSIGNALED(last_fg_status)) {
                    printf("terminated by signal %d\n", WTERMSIG(last_fg_status));
                    fflush(stdout);
                }
            }
        }
        free_command(cmd); // cleanup after each loop
    }
    return EXIT_SUCCESS; // done
}

/*** end of file ***/