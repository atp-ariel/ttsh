#pragma once
#include <sys/types.h>

struct process{
    char* command;
    int argc;
    char** argv;
    char* input_path;
    char* output_path;
    pid_t pid;
    int type;
    int status;
    struct process* next;
};

struct job{
    int id;
    int count_kill;
    struct process* root;
    char* command;
    pid_t pgid;
    int mode;
    int save;
};