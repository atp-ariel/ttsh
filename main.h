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
#include "help.h"


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
//Carga el historial del archivo
void load_history();
//Salva el historial hacia el archivo
void save_history();
//Revisa si existe algun proceso zombi y lo reporta.
void verify_zombies();
//Parsea el comando entrado por consola
struct job* parse_command(char* line);
struct process* parse_command_segment(char* segment);
//Devuelve el tipo de comando.
int get_command_type(char* command);
//Ejecuta los comandos.
int launch_job(struct job* job);
//Ejecuta un proceso
int launch_process(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
//Ejecuta los comando builtin
int execute_builtin_command(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
//Quita los espacios al inicio y al final del comando.
list* helper_strtrim(char* line);
//Devuelve el id del job que contiene al proceso con PID pid
int get_job_id_by_pid(int pid);
//Devuelve el job segun un ID
struct job* get_job_by_id(int id);
//Devuelve el id del proximo job NULL
int get_next_job_id();
//Imprime los procesos de un job
int print_processes_of_job(int id);
//Imprime el estado de un job.
int print_job_status(int id);
//Vacía un job
int release_job(int id);
//Insertar un job
int insert_job(struct job* job);
//Remover un job
int remove_job(int id);
//Devuelve 1 si el job se completo
int is_job_completed(int id);
//Set el estado de un proceso.
int set_process_status(int pid, int status);
//Set el estado a todos los procesos  de un job
int set_job_status(int id, int status);
//Esperar por el proceso con PID pid
int wait_for_pid(int pid);
//Esperar por job
int wait_for_job(int id);
//Devuelve la cantidad de procesos de un job que cumplen cierta condicion.
int get_proc_count(int id, int filter);


void SIG_TRY_KILL_PROC(int signal);
#pragma endregion PROTOTYPES

#pragma region BUILTIN.
void shell_exit();
int shell_cd(int argc, char** argv );
int shell_history(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
int shell_again(int argc, char** argv);
int shell_jobs(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
int shell_fg(int argc, char** argv);
void shell_help(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
#pragma endregion BUILTIN


