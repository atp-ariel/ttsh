#include "main.h"
#include <errno.h>
#include <dirent.h>

int main(int argc, char** argv){
    shell_init();
    shell_loop();
}

void shell_init(){
    signal(SIGINT, SIG_TRY_KILL_PROC);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    pid_t pid = getpid();
    setpgid(pid, pid);
    tcsetpgrp(STDOUT_FILENO, pid);

    shell = calloc(1, sizeof(struct info_shell));

    for (int i = 0; i < NR_JOBS; i++)
        shell->jobs[i] = NULL; 
    shell->back_id = init();
    load_history();  
    childpid = -1;
}
int get_job_id_by_pid(int pid){
    struct process* proc;
    for(int i = 0; i < NR_JOBS; i++){
        if(shell->jobs[i] != NULL){
            for(proc = shell->jobs[i]->root; proc != NULL; proc = proc->next)
                if(proc->pid == pid)
                    return i;
        }
    }
    return -1;
}
void SIG_TRY_KILL_PROC(int signal){
    if(signal == SIGINT){
        int index = -1;
        int pid = getpid();
        printf("%d", pid);
        struct process* proc;
        for(int i = 0; i < NR_JOBS; i++)
            if(shell->jobs[i] != NULL)
                for(proc = shell->jobs[i]->root; proc != NULL; proc = proc->next)
                    if(proc->pid == pid)
                        index = i;
        struct job* job = shell->jobs[index];

        if(job ==  NULL){
            return;
        }
        else if(job->count_kill == 0)
        {
            kill(pid, SIGKILL);
            job->count_kill++;
            printf("%i\n", job->count_kill);
        }
        else if(job->count_kill-- == 1)
            kill(pid, SIGKILL);
    }
}
void update_dir_info(){
    getcwd(shell->cur_dir, sizeof(shell->cur_dir));
}

void print_prompt(){
    update_dir_info();
    printf( COLOR_BLUE BOLD_TEXT "%s" COLOR_NONE, shell->cur_dir);
    printf( PROMPT);
}


void shell_loop(){
    char* line;
    struct job* job;
    int status = 1;

    while(1){
        verify_zombies();

        print_prompt();
        
        line = read_line();
        if(strlen(line) == 0)
            continue;
        
        job = parse_command(line);

        if(job->save){
            if(strncmp(job->command, "again", 5))
                append(history, strdup(job->command));
            while(history->size > HISTORY_LIMIT)
                popfirst(history);
        }
        status = launch_job(job);
    }
}
list* helper_strtrim(char* line){
    char* head = line;
    char* tail = line + strlen(line);
    int* save = calloc(1, sizeof(int));
    *save = 1;
    while(*head == ' ')
    {
        *save = 0;
        head++;
    }
    while(*tail == ' '){
        tail[0] = '\0';
        tail--;
    }
    list* i = init();

    append(i, head);
    append(i, save);
    return i;
}
list* tokenizer(char* argv){
    int maxSize = RL_BUFSIZE;
    int pos = 0;
    int* pos2 = calloc(1, sizeof(int));
    *pos2 = 0;
    char** tokens = calloc(1, maxSize*sizeof(char*));
    char* token = calloc(1, RL_BUFSIZE * sizeof(char));
    int quotation = 0;
    while(*argv != '\0'){
        token = calloc(1, RL_BUFSIZE * sizeof(char));
        pos = 0;
        if(*argv == '\"'){
            quotation++;
            argv++;
            while(*argv != '\"'){
                if(*argv == '\0'){
                    printf("ERROR\n");
                    return NULL;
                }
                token[pos++] = *argv;
                if(pos == sizeof(token) -1){
                    maxSize *= 2;
                    token = realloc(token, maxSize * sizeof(char));
                }
                argv++;
            }
            quotation++;
            argv++;
        }
        else if(!strncmp(argv, " ", 1)){
            argv++;
            continue;
        }
        else{
            while(strncmp(argv, " ", 1) && strncmp(argv, "\0", 1))
            {
                token[pos++] = *argv;
                argv++;
            }
        }
        if(*pos2 == sizeof(tokens)-1){
            maxSize *= 2;
            tokens = realloc(tokens, sizeof(char*) * maxSize);
        }
        tokens[*pos2] = token;
        *pos2 = *pos2 + 1;
    }
    list* to_return = init();
    append(to_return, tokens);
    append(to_return, pos2);
    return to_return;
}
struct process* parse_command_segment(char* segment){
    int bufSize = TOKEN_BUFSIZE;
    //int position = 0;
    char* command = strdup(segment);
    //char* token;
    //char** tokens = calloc(1, bufSize*sizeof(char*));

    //if(!tokens){
    //    fprintf(stderr, COLOR_RED "ERROR\t" COLOR_NONE "Allocation error\n");
    //    exit(EXIT_FAILURE);
    //}
    
    //token = strtok(segment, TOKEN_DELIMITERS);
    
    //while(token != NULL){
    //    tokens[position++] = token;
    //    token = strtok(NULL, TOKEN_DELIMITERS);
    //}
    list* tokenize = tokenizer(segment);
    char** tokens = (char**)(tokenize->first->data);
    int real_size = *(int*)(tokenize->tail->data);
    int argc = 0;
    char* input_path= NULL;
    char* output_path = NULL;
    int i;
    
    for (i = 0; i < /*position*/ real_size; i++)
        if(tokens[i][0] == '<' || tokens[i][0] == '>' || !strcmp(tokens[i], ">>")) 
            break;
    argc = i;

    for(; i < /*position*/ real_size; i++){
        if(tokens[i][0] == '<'){
            input_path = calloc(1, strlen(tokens[i+1])+1);
            strcpy(input_path, tokens[++i]);
        }
        else if(tokens[i][0] == '>' || !strcmp(tokens[i], ">>")){
            output_path = calloc(1, strlen(tokens[i+1])+1);
            strcpy(output_path, tokens[++i]);
            
            if(!strcmp(tokens[i-1],">")){
                remove(output_path);
                int fd = open(output_path, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                close(fd);
            }
        }
        else break;
    }
    for(i = argc; i <= /*position*/ real_size; i++)
        tokens[i] = NULL;
    struct process* new_proc = calloc(1, sizeof(struct process));
    new_proc->command = command;
    new_proc->argv = tokens;
    new_proc->argc = argc;
    new_proc->input_path = input_path;
    new_proc->output_path = output_path;
    new_proc->pid = -1;
    new_proc->type = get_command_type(tokens[0]);

    new_proc->next = NULL;
    return new_proc;
}
struct job* parse_command(char* line){
    list*  temp = helper_strtrim(line);

    line = (char*)(temp->first->data);
    char* command = strdup(line);
    
    struct process *root_proc = NULL, *proc = NULL;
    char* line_cursor = line, *c = line, *seg;
    unsigned int seg_len = 0, mode = FOREGROUND_EXECUTION;

    if(line[strlen(line) - 1] == '&'){
        mode = BACKGROUND_EXECUTION;
        line[strlen(line)-1] = '\0';
    }

    while(1){
        if(*c == '\0' || *c == '|'){
            seg = (char*)malloc((seg_len + 1) * sizeof(char));
            strncpy(seg, line_cursor, seg_len);
            seg[seg_len] = '\0';

            struct process* new_proc = parse_command_segment(seg);
            if(!root_proc){
                root_proc = new_proc;
                proc = root_proc;
            }
            else{
                proc->next = new_proc;
                proc = new_proc;
            }

            if(*c != '\0'){
                line_cursor = c;
                while(*(++line_cursor) == ' ');
                c = line_cursor;
                seg_len = 0;
                continue;
            }
            else break;
        }
        else{
            seg_len++;
            c++;
        }
    }

    struct job* new_job = calloc(1,sizeof(struct job));
    new_job->root = root_proc;
    new_job->command = command;
    new_job->pgid = -1;
    new_job->mode = mode;
    new_job->save = *(int*)(temp->tail->data);
    new_job->count_kill = 0;
    return new_job;
}
int get_command_type(char* command){
    if(command == NULL)
        return COMMAND_EXTERN;
    if(!strcmp(command, "exit"))
        return COMMAND_EXIT;
    else if(!strcmp(command, "history"))
        return COMMAND_HISTORY;
    else if(!strcmp(command, "again"))
        return COMMAND_AGAIN;
    else if(!strcmp(command, "cd"))
        return COMMAND_CD;
    else if(!strcmp(command, "jobs"))
        return COMMAND_JOBS;
    else if(!strcmp(command, "fg"))
        return COMMAND_FG;
    else if(!strcmp(command, "help"))
        return COMMAND_HELP;
    else return COMMAND_EXTERN;
}
char* read_line()
{
    int bufSize = RL_BUFSIZE;
    int position = 0;
    char* buf = calloc(1, bufSize * sizeof(char));
    int c;
    int count_quotation = 0;
    if(!buf){
        fprintf(stderr,COLOR_BLUE "ERROR\t" COLOR_NONE "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        c = getchar();
        if(c == '\"')
            count_quotation++;
        if( c == '#' && count_quotation % 2 == 0){
            buf[position] = '\0';
            while(1)
            {
                c = getchar();
                if(c == EOF || c == '\n' )
                   return buf;
            }
        }
        if(c == EOF || c == '\n' ){
            buf[position] = '\0';
            return buf;
        }
        else
            buf[position] = c;
        position++;

        if(position >= bufSize){
            bufSize += RL_BUFSIZE;
            buf = (char*)realloc(buf, bufSize * sizeof(char));
            if(!buf)
            {
                fprintf(stderr,COLOR_BLUE "ERROR\t" COLOR_NONE "Reallocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int launch_job(struct job* job){
    struct process* proc;
    int status = 0, in_fd = 0, fd[2], job_id = -1;

    verify_zombies();
    if(job->root->type == COMMAND_EXTERN || job->root->type == COMMAND_HELP || job->root->type == COMMAND_HISTORY || job->root->type == COMMAND_JOBS)
        job_id = insert_job(job);
    
    for(proc = job->root; proc != NULL; proc = proc->next){
        if(proc == job->root && proc->input_path != NULL)
        {
            in_fd = open(proc->input_path, O_RDONLY);
            if(in_fd < 0){
                fprintf(stderr, COLOR_RED "ERROR\t" COLOR_NONE "No such file or directory: " COLOR_GREEN "%s\n", proc->input_path);
                remove_job(job_id);
                return -1;
            }
        }
        int out_fd = STDOUT_FILENO;
        if(proc->output_path != NULL){
            out_fd = open(proc->output_path, O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if(out_fd < 0)
               out_fd = STDOUT_FILENO;
        }
        if(proc->next != NULL){
            pipe(fd);
            if(out_fd == STDOUT_FILENO)
                status = launch_process(job, proc, in_fd, fd[1], PIPELINE_EXECUTION);
            else 
                status = launch_process(job, proc, in_fd, out_fd, PIPELINE_EXECUTION);

            close(fd[1]);
            in_fd = fd[0];
        }
        else 
            status = launch_process(job, proc, in_fd, out_fd, job->mode);

    }

    if(job->root->type == COMMAND_EXTERN || job->root->type == COMMAND_HISTORY || job->root->type == COMMAND_JOBS){
        if(status >= 0 && job->mode == FOREGROUND_EXECUTION)
            remove_job(job_id);
        else if(job->mode == BACKGROUND_EXECUTION)
            print_processes_of_job(job_id);
    }
    return status;
}

int launch_process(struct job* job, struct process* proc, int in_fd, int out_fd, int mode){
    proc->status = STATUS_RUNNING;
    if(proc->type != COMMAND_EXTERN && execute_builtin_command(job,proc, in_fd, out_fd, mode))
        return 0;

    int status = 0;
        
    childpid = fork();

    if(childpid < 0)
       return -1;
    else if(!childpid){
        signal(SIGINT, SIG_TRY_KILL_PROC);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);


        proc->pid = getpid();
        if(job->pgid > 0)
            setpgid(0, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(0, job->pgid);
        }

        if(in_fd != STDIN_FILENO){
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if(out_fd != STDOUT_FILENO){
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        
        if(execvp(proc->argv[0], proc->argv)< 0)
        {
            fprintf(stderr, "%s: command not found\n", proc->argv[0]);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else{
        proc->pid = childpid;
        if(job->pgid>0)
            setpgid(childpid, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(childpid, job->pgid);
        }

        if(mode == FOREGROUND_EXECUTION){
            tcsetpgrp(STDOUT_FILENO, job->pgid);
            status = wait_for_job(job->id);
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(STDOUT_FILENO, getpid());
            signal(SIGTTOU, SIG_DFL);
        }
    }
    return status;
}

int execute_builtin_command(struct job* job, struct process* proc, int in_fd, int out_fd, int mode){
    int status = 1;
    switch(proc->type){
        case COMMAND_HELP:
            shell_help(job, proc, in_fd, out_fd, mode);
            break;
        case COMMAND_EXIT:
            shell_exit();
            break;
        case COMMAND_CD:
            shell_cd(proc->argc, proc->argv);
            break;
        case COMMAND_HISTORY:
            shell_history( job, proc, in_fd, out_fd, mode);
            break;
        case COMMAND_AGAIN:
            shell_again(proc->argc, proc->argv);
            break;
        case COMMAND_JOBS:
            shell_jobs(job, proc, in_fd, out_fd, mode);
            break;
        case COMMAND_FG:
            shell_fg(proc->argc, proc->argv);
            break;
        default:
            status = 0;
            break;
    }
    return status;
}

char* get_user_dir()
{
    char* user = getenv("USER");
    char* dir = calloc(1, sizeof(user) + 6);
    strcat(dir, "/home/");
    strcat(dir, user);
    return dir;
}
int shell_cd(int argc, char** argv){
    if(argc == 1){
        char* dir = get_user_dir();
        chdir(dir);
        update_dir_info();
        return 1;
    }
    if(!strncmp(argv[1], "~", 1)){
        char* dir = get_user_dir();
        chdir(dir);
        update_dir_info();
        argv[1]++;
        if(!strncmp(argv[1],"\0",1) || argv[1] == NULL)
            return 1;
    }
    if(!chdir(argv[1])){
        update_dir_info();
        return 1;
    }  
    fprintf(stderr, COLOR_RED "ERROR\t" COLOR_GREEN " cd: %s" COLOR_NONE ": No such file or directory\n", argv[1]);
    return 0;
}
int shell_jobs(struct job* job, struct process* proc, int in_fd, int out_fd, int mode){
    int argc = proc->argc;
    char** argv = proc->argv;
    childpid = fork();

    if(!childpid){
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);


        proc->pid = getpid();
        if(job->pgid > 0)
            setpgid(0, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(0, job->pgid);
        }

        if(in_fd != STDIN_FILENO){
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if(out_fd != STDOUT_FILENO){
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        for(int i = 0;i < NR_JOBS; i++)
          if(shell->jobs[i]!= NULL && shell->jobs[i]->mode == BACKGROUND_EXECUTION)
            print_job_status(i);
        exit(EXIT_SUCCESS);
    }
    else{
        proc->pid = childpid;
        if(job->pgid>0)
            setpgid(childpid, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(childpid, job->pgid);
        }

        if(mode == FOREGROUND_EXECUTION){
            tcsetpgrp(STDOUT_FILENO, job->pgid);
            wait_for_job(job->id);
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(STDOUT_FILENO, getpid());
            signal(SIGTTOU, SIG_DFL);
        }
    }
}
void shell_exit(){
    save_history();
    exit(0);
}
void load_history(){
    history = init();
    char* dir = get_user_dir();
    dir = realloc(dir, sizeof(dir) + 12);
    dir = strcat(dir, "/history.dat");

    int fd = open(dir, O_CREAT|O_APPEND |O_RDONLY, S_IRWXU);
    if(fd < 0)
    {
        fprintf(stderr, COLOR_RED "ERROR\t" COLOR_NONE "Opening history\n");
        return;
    }
    
    char ** array = calloc(HISTORY_LIMIT, HISTORY_LIMIT * sizeof(char*));
    int maxSize = 64;
    char letter;
    char *buf;

    for (int i = 0; read(fd, &letter, 1)>0 && i < HISTORY_LIMIT; i++)
    {
        buf = calloc(maxSize, sizeof(char));
        array[i] = buf;
        buf[0] = letter;
        for (int j = 1; read(fd, &letter, 1)>0 && letter != '\n'; j++)
        {
            if(j == maxSize -1){
                maxSize *= 2;
                buf = realloc(buf, maxSize * sizeof(char));
            }
            buf[j] = letter;
            buf[j+1] = '\0';

        }
        maxSize = 64;
        append(history, array[i]);
    }
    close(fd);
}
void save_history(){
    char* dir = get_user_dir();
    dir = realloc(dir, sizeof(dir) + 12);
    dir = strcat(dir, "/history.dat");
    if(remove(dir)<0)
    {
        fprintf(stderr,COLOR_RED "ERROR\t" COLOR_NONE "Deleting history\n");
        return;
    }
    int fd = open(dir, O_CREAT |O_RDWR, S_IRWXU);
    
    if(fd < 0)
    {
        fprintf(stderr, COLOR_RED "ERROR\t" COLOR_NONE "Opening history\n");
        return;
    }

    while(history->size > HISTORY_LIMIT)
        popfirst(history);
    
    char* cmd;
    node* iter = history->first;

    for(int i = 0;i < history->size; i++){
        cmd = (char*)iter->data;

        int j = 0;
        while(cmd[j] != '\0')
            write(fd, &cmd[j++], sizeof(char));
        write(fd, "\n", sizeof(char));
        iter = iter->next;
    }
    close(fd);
}
void verify_zombies(){
    int status, pid;
    while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED|__W_CONTINUED)>0)){
        if(WIFEXITED(status))
            set_process_status(pid, STATUS_DONE);
        else if(WIFSTOPPED(status))
            set_process_status(pid, STATUS_SUSPENDED);
        else if(WIFCONTINUED(status))
            set_process_status(pid, STATUS_CONTINUED);
        
        int job_id = get_job_id_by_pid(pid);
        if(job_id > 0 && is_job_completed(job_id)){
            print_job_status(job_id);
            remove_job(job_id);
        }
    }
}

struct job* get_job_by_id(int id){
    if(id > NR_JOBS)
        return NULL;
    return shell->jobs[id];
}

int get_pgid_by_job_id(int id){
    struct job* job = get_job_by_id(id);
    if(job == NULL)
        return -1;
    return job->pgid;
}

int get_next_job_id(){ 
    for(int i  = 1; i <= NR_JOBS; i++)
        if(shell->jobs[i] == NULL)
            return i;
    return -1;
}

int print_processes_of_job(int id){
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return -1;
    
    printf("[%d]", id);

    struct process *proc;
    for(proc = shell->jobs[id]->root; proc != NULL; proc = proc->next)
        printf(" %d", proc->pid);
    printf("\n");
    return 0;
}
int print_job_status(int id)
{
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return -1;
    
    printf("[%d]", id);
    
    struct process * proc;
    for(proc = shell->jobs[id]->root; proc != NULL; proc = proc->next)
        printf("\t%d\t%s\t%s\n", proc->pid, STATUS_STRING[proc->status], proc->command);
    return 0;
}
int release_job(int id){
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return -1;

    struct job* job = get_job_by_id(id);
    struct process *proc, *temp;
    for(proc = job->root; proc != NULL;){
        temp = proc->next;
        free(proc->command);
        free(proc->argv);
        free(proc->input_path);
        free(proc->output_path);
        free(proc);
        proc = temp;
    }
    free(job->command);
    free(job);

    return 0;
}
int insert_job(struct job* job){
    int* id = calloc(1, sizeof(int));
    *id = get_next_job_id();
    if(id < 0)
        return -1;
    
    job->id = *id;
    shell->jobs[*id] = job;
    if(job->mode == BACKGROUND_EXECUTION)
        insert(shell->back_id, id);
    return *id;
}

int remove_job(int id){
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return -1;   
    
    release_job(id);
    shell->jobs[id] = NULL;
    return 0;
}

int is_job_completed(int id){
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return 0;

    struct process *proc;
    for(proc = shell->jobs[id]->root; proc != NULL; proc = proc->next)
        if(proc->status != STATUS_DONE)
            return 0;
    return 1;
}
int set_process_status(int pid, int status){
    int i;
    struct process *proc;
    for(i = 1; i < NR_JOBS; i++){
        if(shell->jobs[i] == NULL)
            continue;
        for(proc = shell->jobs[i]->root; proc != NULL; proc = proc->next)
            if(proc->pid ==  pid){
                proc->status = status;
                return 0;
            }
    }
    return -1;
}
int set_job_status(int id, int status){
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return -1;

    int i;
    struct process* proc;
    struct job* job = get_job_by_id(id);
    for(proc = job->root; proc != NULL; proc = proc->next)
        if(proc->status != STATUS_DONE)
            proc->status = status;
    return 0;
}

int wait_for_pid(int pid){
    int status = 0;

    waitpid(pid, &status, WUNTRACED);
    if(WIFEXITED(status))
        set_process_status(pid, STATUS_DONE);
    else if(WIFSIGNALED(status))
        set_process_status(pid, STATUS_TERMINATED);
    else if(WSTOPSIG(status))
    {
        status = -1;
        set_process_status(pid, STATUS_SUSPENDED);
    }
    return status;
}

int wait_for_job(int id){
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return -1;
    
    int proc_count = get_proc_count(id, FILTER_REMAINING);
    int wait_pid = -1, wait_count = 0;
    int status = 0;

    do{
        wait_pid = waitpid(-shell->jobs[id]->pgid, &status, WUNTRACED);
        wait_count++;

        if(WIFEXITED(status))
            set_process_status(wait_pid, STATUS_DONE);
        else if(WIFSIGNALED(status))
            set_process_status(wait_pid, STATUS_TERMINATED);
        else if(WSTOPSIG(status))
        {
            status = -1;
            set_process_status(wait_pid, STATUS_SUSPENDED);
            if(wait_count == proc_count)
                print_job_status(id);
        }
    }while(wait_count < proc_count);
    return status;
}
int get_proc_count(int id, int filter){
    if(id > NR_JOBS || shell->jobs[id] == NULL)
        return -1;
    
    int count = 0;
    struct process *proc;
    struct job* job = get_job_by_id(id);
    for(proc = job->root; proc != NULL; proc = proc->next)
        if(filter == FILTER_ALL || (filter == FILTER_DONE && proc->status == STATUS_DONE) || (filter == FILTER_REMAINING && proc->status != STATUS_DONE))
            count++;
    return count;
}
int shell_history(struct job* job, struct process* proc, int in_fd, int out_fd, int mode){
    childpid = fork();

    if(!childpid){
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);


        proc->pid = getpid();
        if(job->pgid > 0)
            setpgid(0, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(0, job->pgid);
        }

        if(in_fd != STDIN_FILENO){
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if(out_fd != STDOUT_FILENO){
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        node* iter;
    
        while(history->size > HISTORY_LIMIT)
            popfirst(history);

        iter = history->first;
        char* cmd;
        for(int i = 0; i < history->size; i++){
            cmd = (char*)(iter->data);
            printf("[%i]: %s\n", i+1, cmd);
            iter = iter->next;
        }
        save_history();
        exit(EXIT_SUCCESS);
    }
    else{
        proc->pid = childpid;
        if(job->pgid>0)
            setpgid(childpid, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(childpid, job->pgid);
        }

        if(mode == FOREGROUND_EXECUTION){
            tcsetpgrp(STDOUT_FILENO, job->pgid);
            wait_for_job(job->id);
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(STDOUT_FILENO, getpid());
            signal(SIGTTOU, SIG_DFL);
        }
    }
}
int shell_again(int argc, char** argv)
{
    int index = atoi(argv[1]);
    if(index > history->size){
        fprintf(stderr, COLOR_RED "ERROR\t" COLOR_NONE "Unexistent command in history at index: " COLOR_GREEN "%i\n", index);
        return -1;
    }
    index--;
    if(index < 0 || index >= HISTORY_LIMIT){
        fprintf(stderr, COLOR_RED "ERROR\t" COLOR_NONE "Index out of range of history\n");
        return -1;
    }
    char* line = strdup((char*)(get(history, index)->data));
    struct job* job = parse_command(line);
    if(job->save){
        append(history, strdup(job->command));
        while(history->size > HISTORY_LIMIT)
            popfirst(history);
    }
    launch_job(job);
}

int shell_fg(int argc, char**argv)
{
    int status;
    pid_t pid;
    int job_id = -1;

    if(argc == 1)
    {
        job_id  = *(int*)(popfirst(shell->back_id)->data);
        pid = get_pgid_by_job_id(job_id);
        if(pid < 0){
            fprintf(stderr, COLOR_RED "ERROR\t" COLOR_GREEN "fg %s: " COLOR_NONE "No such job\n", argv[1]);
            return -1;
        }
        if(kill(-pid, SIGCONT) < 0){
            fprintf(stderr, COLOR_RED "ERROR\t" COLOR_GREEN "fg %s: " COLOR_NONE "No such job\n", argv[1]);
            return -1;
        }

        tcsetpgrp(STDOUT_FILENO, pid);
        set_job_status(job_id, STATUS_CONTINUED);
        if(wait_for_job(job_id)>= 0)
            remove_job(job_id);


        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(STDOUT_FILENO, getpid());
        signal(SIGTTOU, SIG_DFL);

        return status;
    }

    job_id = atoi(argv[1]);
    int index = -1;
    for(int i = 0; i < shell->back_id->size; i++)
        if(*(int*)(get(shell->back_id, i)->data) == job_id)
            index = i;
    if(index > -1)
        remove_at(shell->back_id, index);
        
    pid = get_pgid_by_job_id(job_id);
    if(pid < 0){
        fprintf(stderr, COLOR_RED "ERROR\t" COLOR_GREEN "fg %s: " COLOR_NONE "No such job\n", argv[1]);
        return -1;
    }
    if(kill(-pid, SIGCONT) < 0){
        fprintf(stderr, COLOR_RED "ERROR\t" COLOR_GREEN "fg %s: " COLOR_NONE "No such job\n", argv[1]);
        return -1;
    }
    
    tcsetpgrp(STDOUT_FILENO, pid);
    set_job_status(job_id, STATUS_CONTINUED);
    if(wait_for_job(job_id)>= 0)
        remove_job(job_id);
    

    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDOUT_FILENO, getpid());
    signal(SIGTTOU, SIG_DFL);

    return status;
}

void shell_help(struct job* job, struct process* proc, int in_fd, int out_fd, int mode){
    childpid = fork();

    if(!childpid){
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);


        proc->pid = getpid();
        if(job->pgid > 0)
            setpgid(0, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(0, job->pgid);
        }

        if(in_fd != STDIN_FILENO){
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if(out_fd != STDOUT_FILENO){
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        
        FILE* f = (FILE*)malloc(sizeof(FILE));
        char* buff = (char*)malloc(sizeof(char));

        if(proc->argc == 1)
            f = fopen("./help/help.ttsh.help", "r");
        else if(!strcmp(proc->argv[1], "exit"))
            f = fopen("./help/exit.ttsh.help", "r");
        else if(!strcmp(proc->argv[1], "history"))
            f = fopen("./help/history.ttsh.help", "r");
        else if(!strcmp(proc->argv[1], "again"))
            f = fopen("./help/again.ttsh.help", "r");
        else if(!strcmp(proc->argv[1], "cd"))
            f = fopen("./help/cd.ttsh.help", "r");
        else if(!strcmp(proc->argv[1], "jobs"))
            f = fopen("./help/jobs.ttsh.help", "r");
        else if(!strcmp(proc->argv[1], "fg"))
            f = fopen("./help/fg.ttsh.help", "r");
        else if(!strcmp(proc->argv[1], "--all"))
        {
            printf("HASD");
            remove("./help/all.ttsh.help");
            DIR * d;
            struct dirent *dir;
            d = opendir("./help");
            list* dir_file = init();
            if(d){
                while((dir = readdir(d)) != NULL){
                    append(dir_file, dir->d_name);
                }
                closedir(d);
            }
            f = fopen("./help/all.ttsh.help", "w+");
            
            FILE* f_aux = (FILE*)malloc(sizeof(FILE));
            char* buff_aux = (char*)malloc(sizeof(char));
            char* filename = (char*)malloc(sizeof(char) * dir_file->size);
            int i = 0;
            printf("%d", dir_file->size);
            while(i < dir_file->size){
                f_aux = fopen(get(dir_file, i)->data, "r");
                while (read(f_aux->_fileno,buff_aux,1) > 0){
                    printf("%c", *buff_aux);
                    fwrite(*buff_aux, 1, 1, f);
                    //fwrite(f,"%c", *buff_aux);
                }
                fclose(f_aux);
                i++;
            }

        }
        else
        {
            fprintf(stderr, "'%s' it's not a ttsh command\n", proc->argv[1]);
            exit(EXIT_FAILURE);
        }
        
        while (read(f->_fileno,buff,1) > 0){
            printf("%c", *buff);
        }
        exit(EXIT_SUCCESS);
    }
    else{
        proc->pid = childpid;
        if(job->pgid>0)
            setpgid(childpid, job->pgid);
        else{
            job->pgid = proc->pid;
            setpgid(childpid, job->pgid);
        }

        if(mode == FOREGROUND_EXECUTION){
            tcsetpgrp(STDOUT_FILENO, job->pgid);
            wait_for_job(job->id);
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(STDOUT_FILENO, getpid());
            signal(SIGTTOU, SIG_DFL);
        }
    }
}


