#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAXLINE 8192 /* Max text line length */

//백그라운드 잡

//찾을 때 무슨 모드 쓸건지
enum
{
    pid_find_mod,
    idx_find_mod
};

enum
{
    RUNNING,
    DONE,
    STOPPED
}; // job status

typedef struct JOB_INFO
{
    pid_t pid;
    int idx;
    int status;
    char cmd[MAXLINE]; // maxline 이다 csapp에 정의되어있는
    struct JOB_INFO *next;
} JOB_INFO;

void add_job(int pid, int status, char *cmd_line);
int del_job(int);
void job_list();
int locate_job_by_pid();

extern JOB_INFO *head;