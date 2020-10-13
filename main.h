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
#define COLOR_BLUE "\x1b[034m"
#define COLOR_NONE "\x1b[0m"
#define BOLD_TEXT "\033[1m"
#define PROMPT "$ "
#define COLOR_RED "\x1b[031m"
#define COLOR_GREEN "\x1b[032m"

#define MAXLETTERS 1000 // numero maximo de letras soportadas
#define MAXCOMMANDS 100 // numero maximo de componentes soportados
#define RL_BUFSIZE 1024
#define NR_JOBS 512
#define TOKEN_BUFSIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a\""

#define HISTORY_LIMIT 10

#define COMMAND_EXTERN 0
#define COMMAND_HISTORY 1
#define COMMAND_AGAIN 2
#define COMMAND_EXIT 3
#define COMMAND_JOBS 4
#define COMMAND_FG 5
#define COMMAND_CD 6
#define COMMAND_HELP 7

#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_SUSPENDED 2
#define STATUS_CONTINUED 3
#define STATUS_TERMINATED 4

#define BACKGROUND_EXECUTION 0
#define FOREGROUND_EXECUTION 1
#define PIPELINE_EXECUTION 2

#define FILTER_ALL 0
#define FILTER_DONE 1
#define FILTER_REMAINING 2

const char* STATUS_STRING[] = {"Running", "Done", "Suspended", "Continued", "Terminated"};

struct info_shell{
    char cur_dir[MAXLETTERS];
    struct job* jobs[NR_JOBS + 1];
    list* back_id;
};

struct info_shell *shell;
list* history;
pid_t  childpid;

#pragma endregion DEFINITIONS

#pragma region PROTOTYPES
//Inicializa el SHELL
void shell_init();
//Actualiza la dirección del directorio del SHELL
void update_dir_info();
char* get_user_dir();
//Imprime el Prompt del SHELL
void print_prompt();
//Ejecuta el proceso del SHELL
void shell_loop();
//Lee la linea de comando del SHELL
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

#pragma region BUILTIN
void shell_exit();
int shell_cd(int argc, char** argv );
int shell_history(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
int shell_again(int argc, char** argv);
int shell_jobs(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
int shell_fg(int argc, char** argv);
void shell_help(struct job* job, struct process* proc, int in_fd, int out_fd, int mode);
#pragma endregion BUILTIN

#pragma region HELP
#define  WELCOME "\033[1m\x1b[031mBienvenidos a la ayuda de \'ttsh\'\x1b[0m"
#define  DEVELOPERS "\033[1m\x1b[031mIntegrantes:\x1b[0m Carlos Toledo Silva C-211\n\t     Ariel Alfonso Triana Pérez C-211"
#define  FUNCTIONS "\033[1m\x1b[031mFuncionalidades:\n\x1b[032mbasic:\x1b[0m funcionalidades básicas (3 puntos)\n\033[1m\x1b[032mmulti-pipe:\x1b[0m múltiples tuberías (1 punto)\n\033[1m\x1b[032mbackground:\x1b[0m procesos ejecutándose en segundo plano (0.5 puntos)\n\033[1m\x1b[032mspaces:\x1b[0m comandos con cualquier cantidad de espacios (0.5 puntos)\n\033[1m\x1b[032mhistory:\x1b[0m historial de comandos (0.5 puntos)\n\033[1m\x1b[032mctrl+c:\x1b[0m envia la señal SIGINT al proceso que se ejecuta (0.5 puntos)\n\033[1m\x1b[032mhelp:\x1b[0m descripción de las funcionalidades del shell (1 punto)\n"
#define  BUILTINS  "\033[1m\x1b[031mComandos built-in:\n\x1b[032mcd:\x1b[0m cambia de direcciones\n\033[1m\x1b[032mexit\x1b[0m termina el shell\n\033[1m\x1b[032mhistory:\x1b[0m muestra el historial\n\033[1m\x1b[032mfg:\x1b[0m devuelve al foreground un proceso del background\n\033[1m\x1b[032magain:\x1b[0m ejecuta un comando del historial\n\033[1m\x1b[032mhelp:\x1b[0m muestra la ayuda\n\033[1m\x1b[032mjobs:\x1b[0m muestra todos los jobs en cola\n\nPara ver la descripción de todos estos built-in ejecute el comando \033[1mhelp --all\x1b[0m"
#define TOTAL_POINTS "\033[1m\x1b[031mTotal:\x1b[0m\t 7 puntos\n"

#define EXIT_HELP "En el shell con el comando \033[1m\x1b[032mexit\x1b[0m se puede cerrar el mismo de forma que salve los comandos en el historial de comandos. Antes de cerrar el shell se mostrará un mensaje de despedida." 
#define HISTORY_HELP "En el shell con el comando \033[1m\x1b[032mhistory\x1b[0m se puede consultar los últimos 10 comandos en las ejecuciones del shell.\n\nPara implementarlo se crea un archivo en el directorio /home/USER llamado \033[1mhistory.dat\x1b[0m donde se guardan los últimos 10 comandos. Es posible mostrar más de 10 comandos, solo es necesario definir la cantidad en la macro \033[1mHISTORY_LIMITS\x1b[0m del archivo \033[1mmain.h\x1b[0m. Cuando se inicializa el shell, se carga el historial en una lista, y al cerrar el shell se guarda la lista en el archivo. Si se desea que un comando no se guarde en el historial solo es necesario introducir un espacio antes del comando. Por ejemplo:\n\033[1mls\n echo hola mundo\x1b[0m\nEl comando \033[1mls\x1b[0m se guarda en el historial pero \033[1mecho hola mundo\x1b[0m no se guarda."
#define AGAIN_HELP "En el shell con el comando \033[1m\x1b[032magain <index>\x1b[0m se puede volver a ejecutar el comando que ocupa la posición \033[1mindex\x1b[0m en el historial. Al ejecutar el comando lo que se guarda en el historial es el comando ha ejecutar por el \033[1magain\x1b[0m.\n\nEl comando \033[1magain\x1b[0m puede dar varios errores en ejecución:\n\t1. El primer error es que no exista ningún comando en el índice especificado y reportando el error: \'\033[1mUnexistent command in history at index: <index>\x1b[0m\'\n\t2. El segundo es que el índice esté fuera de la cantidad máxima de comandos en el historial, reportando el error: \'\033[1mIndex out of range of history\x1b[0m\'"
#define CD_HELP "En el shell con el comando \033[1m\x1b[032mcd <dir>\x1b[0m se puede cambiar la dirección del shell.\n\nSi el comando \033[1mcd\x1b[0m no recibe ningún parámetro entonces cambia la dirección hacia \033[1m/home/USER\x1b[0m. Si recibe \033[1m<dir> = ..\x1b[0m cambia la dirección hacia el padre del actual directorio.\n\nAlguno de los problemas es que no se puede pasar como parámetro una dirección que contenga espacios, por ejemplo: \033[1m/home/USER/proyecto shell\x1b[0m."
#define JOBS_HELP "En el shell con el comando  \033[1m\x1b[032mjobs\x1b[0m se puede obtener todos los comandos que se estan ejecutando en el background. Imprime por consola todos los procesos con el formato:\n \033[1m[<index>] <pid> <command> <state>\x1b[0m"
#define FG_HELP "En el shell con el comando \033[1m\x1b[032mfg <index>\x1b[0m se puede traer un proceso de background hacia el foreground. Si no recibe ningún \033[1m<index>\x1b[0m el comando \033[1mfg\x1b[0m trae a foreground el último proceso que se envió a background. Si recibe algún \033[1mindex\x1b[0m envía a foreground el comando que está en ese índice en jobs."
#pragma endregion HELP
