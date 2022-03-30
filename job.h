#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//백그라운드 잡
enum
{
    RUNNING,
    DONE,
    STOPPED
};

//찾을 때 무슨 모드 쓸건지
enum
{
    pid_find_mod,
    idx_find_mod
};

typedef struct JOB_INFO
{
    pid_t pid;
    int idx;
    int status;
    char cmd[8192]; // maxline 이다 csapp에 정의되어있는
    struct JOB_INFO *next;
} JOB_INFO;

void add_job(int pid, char *cmd_line);
void job_list();

extern JOB_INFO *head;