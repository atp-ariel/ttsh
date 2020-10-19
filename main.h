#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> 
#include <sys/wait.h> 
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include "list.h"
#include "process.h"


#pragma region DEFINITIONS
/* Color definitions */
#define COLOR_BLUE "\x1b[034m"
#define COLOR_NONE "\x1b[0m"
#define COLOR_RED "\x1b[031m"
#define COLOR_GREEN "\x1b[032m"
/* End color definitions */

/* Type of text definitions */
#define BOLD_TEXT "\033[1m"
/* End type of text definitions */

/* Prompt definitions */
#define PROMPT "$ "
/* End prompt definitions */

/* Type of command definitions */
#define COMMAND_EXTERN 0
#define COMMAND_HISTORY 1
#define COMMAND_AGAIN 2
#define COMMAND_EXIT 3
#define COMMAND_JOBS 4
#define COMMAND_FG 5
#define COMMAND_CD 6
#define COMMAND_HELP 7
/* End type of command definitions */

/* History settings */
#define HISTORY_LIMIT 10
/* End history settings */

/* General settings and variables*/
#define MAXLETTERS 1000 // numero maximo de letras soportadas
#define MAXCOMMANDS 100 // numero maximo de componentes soportados
#define RL_BUFSIZE 1024
#define NR_JOBS 512
#define TOKEN_BUFSIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a\""

struct info_shell *shell;
list* history;
pid_t  childpid;
/* End general settings and variables*/

/* Process status & execution */
#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_SUSPENDED 2
#define STATUS_CONTINUED 3
#define STATUS_TERMINATED 4

#define BACKGROUND_EXECUTION 0
#define FOREGROUND_EXECUTION 1
#define PIPELINE_EXECUTION 2

const char* STATUS_STRING[] = {"Running", "Done", "Suspended", "Continued", "Terminated"};
/* End process status & execution */

/* Filters to process */
#define FILTER_ALL 0
#define FILTER_DONE 1
#define FILTER_REMAINING 2
/* End filters to process */

#pragma region InfoShell
struct info_shell{
    int pid;
    //Direction of the shell
    char cur_dir[MAXLETTERS];
    //Jobs of in the shell
    struct job* jobs[NR_JOBS + 1];
    list* back_id;
};
#pragma endregion InfoShell
#pragma endregion DEFINITIONS

#pragma region PROTOTYPES
// Shell initializate
void shell_init();

// Update the direction of the shell on shell info
void update_dir_info();

// Return the direction of the user
char* get_user_dir();

// Print prompt
void print_prompt();
// Execute the loop of the shell
void shell_loop();
// Read the line in stdin
char* read_line();
// Load the history file to the list
void load_history();
// Save the list history to file
void save_history();
// Check if exist a zombie process to report its.
void verify_zombies();
// Parse the command line
struct job* parse_command(char* line);
// Parse a segment of the command line
struct process* parse_command_segment(char* segment);
// Return command type
int get_command_type(char* command);
// Execute the jobs
int launch_job(struct job* job);
// Execute a process
int launch_process(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
// Execute a builtin command
int execute_builtin_command(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
// Quit whitespace at the begining and the end of the command line
list* helper_strtrim(char* line);
// Return the id of the job with a process with this pid
int get_job_id_by_pid(int pid);
// Return a job with id.
struct job* get_job_by_id(int id);
// Search the next null job and return it
int get_next_job_id();
// Print process jobs
int print_processes_of_job(int id);
// Print status job
int print_job_status(int id);
// Clean a job
int release_job(int id);
// Insert job
int insert_job(struct job* job);
// Delete a job
int remove_job(int id);
// Return 1 if job was finish
int is_job_completed(int id);
// Set status to a process
int set_process_status(int pid, int status);
// Set status to all the process in the job
int set_job_status(int id, int status);
// Wait for a process
int wait_for_pid(int pid);
// Wait for a job
int wait_for_job(int id);
// Return a count of a process that satisfied a condition filter.
int get_proc_count(int id, int filter);


#pragma endregion PROTOTYPES

#pragma region BUILTIN.
// Close the shell
void shell_exit();
// Change the direction 
int shell_cd(int argc, char** argv );
// Print a history
int shell_history(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
// Execute again
int shell_again(int argc, char** argv);
// Execute jobs
int shell_jobs(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
// Execute fg
int shell_fg(int argc, char** argv);
// Execute help
void shell_help(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
#pragma endregion BUILTIN


