/* $begin shellmain */
#include "csapp.h"
#include "mycd.h"
#include "job.h"
#include "signal_handler.h"
#include <errno.h>
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline, bool got_piped, int cardinality);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
int pipe_running(char **argv, int starting_point);
int pair_clear(char *buf, char c);
void find_pipe(char **argv, char *front[], char *back[], bool *pipeFlag, int *starting_point);

JOB_INFO jobs[100];
JOB_INFO *head = NULL;
int gpid_now;
int test = 0;

int main()
{

    char cmdline[MAXLINE]; /* Command line */
    Signal(SIGCHLD, sigchild_handler);
    //  Signal(SIGINT, sigint_handler); // set handler for SIGINT // 일단 없에서 디버깅 용도로 사용한다.
    Signal(SIGTSTP, sigstp_handler); // set handler for SIGTSTP

    while (1)
    {
        /* Read */
        printf("> ");
        fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);
        eval(cmdline, false, 0);
    }
    return 0;
    // jobs 구조체 해제하기
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline, bool got_piped, int step)
{
    char *argv[MAXARGS];   /* Argument list execve() */
    char buf[MAXLINE];     /* Holds modified command line */
    int bg;                /* Should the job run in bg or fg? */
    int argc;              /*number of arguments*/
    pid_t pid;             /* Process id */
    int status;            /*자식 프로세스 종료 여부 확인*/
    char *next_command;    /*(일단 파이프 용도로 만듬)다음에 넘겨줄 명령어*/
    bool pipeFlag = false; /*파이프인지 여부를 판단*/
    int backtick_flag[2], small_quote_flag[2], big_quote_flag[2];
    sigset_t mask_all, mask_one, prev_one;

    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);

    Sigprocmask(SIG_BLOCK, &mask_one, NULL);

    strcpy(buf, cmdline);

    pair_clear(buf, '"'); // clear ""
    pair_clear(buf, 39);  // clear '
    pair_clear(buf, '`'); // clear '
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    for (argc = 0; argv[argc] != NULL; argc++) /*pipe 유무를 찾아주는 반복문*/
        if (!strcmp(argv[argc], "|"))
            pipeFlag = true;

    // fprintf(stdout, "%c\n", argv[argc - 1][strlen(argv[argc - 1]) - 1]);

    if (argv[argc - 1][strlen(argv[argc - 1]) - 1] == '&')
    {
        bg = 1;
        argv[argc - 1][strlen(argv[argc - 1]) - 1] = '\0';
    }

    if (!builtin_command(argv))
    { // quit -> exit(0), & -> ignore, other -> run
        // printf("hi my name is !\n");
        if ((pid = Fork()) == 0)
        {
            if (setpgid(0, 0) == -1)
                unix_error("error in setpgid on line 87!\n");
            // printf("running the job %s\n", argv[0]);
            /* Child runs user job */
            char dest[128];
            strcpy(dest, "/bin/");
            strcat(dest, argv[0]);

            if (pipeFlag)
            {
                pipe_running(argv, 0);
            }
            else if (execve(dest, argv, environ) < 0) // 일단 임시방편
            {                                         // ex) /bin/ls ls -al &
                printf("%s Command not found.\n", argv[0]);
                exit(0);
            }
        }
        else
        {
            /* Parent waits for foreground job to terminate */
            if (!bg)
            {
                gpid_now = pid;
                fprintf(stdout, "parent pid is %d\n", getpid());
                fprintf(stdout, "pid of child  is %d\n", pid);
                fprintf(stdout, "gpid of child is %d\n", gpid_now);
                Wait(&status);
                fprintf(stdout, "passed test on Wait on line 119 on eval func\n");
                gpid_now = 0;
                //   fprintf(stdout, "eval caculated!\n");
            }
            else // when there is background process!
            {
                add_job(pid, cmdline);
                // fprintf(stdout, "%d %s", pid, cmdline);
            }
        }
    }
    // fprintf(stdout, "this is the end\n");
    Sigprocmask(SIG_UNBLOCK, &mask_one, NULL);
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (strcmp("cd", argv[0]) == 0) // cd 인 경우 포함해서
    {
        int my_cd_return = my_cd(argv);

        if (my_cd_return != 0)
            fprintf(stdout, "error has happend in my_cd function!\n");

        return 1;
    }
    else if (!strcmp(argv[0], "jobs"))
    {
        /* job update() 를 해야함 원하는 구상은 jobs에서 부르면 그냥 업데이트만 해주고 꺼져주기 아니면 출력하고 삭제까지*/
        job_list();
        return 1;
    }
    else if (!strcmp(argv[0], "fg"))
    {
        return 1;
    }
    else if (!strcmp(argv[0], "bg"))
    {
        return 1;
    }
    else if (!strcmp(argv[0], "kill"))
    {
        // Kill()
        return 1;
    }
    else if (!strcmp(argv[0], "exit")) /* quit command */
        exit(0);
    else if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;
    return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
    char *delim; /* Points to first space delimiter */
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = ' ';   /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc] = buf;
        *delim = '\0'; // 종지부 찍어주기
        buf = delim + 1;

        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
        argc++;
    }
    argv[argc] = NULL;

    if (argc == 0) /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL;

    return bg;
}
/* $end parseline */

/*function for dealing with piped sentence*/
int pipe_running(char **argv, int starting_point)
{
    char *front[MAXARGS] = {0};
    char *back[MAXARGS] = {0};
    pid_t pid;
    bool pipeFlag = false;
    int fd[2];
    int prev_starting_point = starting_point;

    if (argv[prev_starting_point] == NULL)
        return 0;

    // for (int i = prev_starting_point; argv[i] != NULL; i++)
    // {
    //     fprintf(stdout, "the %dth argument is %s\n", i, argv[i]);
    // }

    if (pipe(fd) == -1)
    {
        fprintf(stderr, "pipe not working!\n");
        exit(1);
    }

    // 파이프 있는지 찾기

    find_pipe(argv, front, back, &pipeFlag, &starting_point);
    if (pipeFlag)
    {
        // fprintf(stdout, "pipe %d arg is front %s back %s\n", pipeFlag, argv[prev_starting_point], argv[starting_point]);
        // fprintf(stdout, "starting number is %d next number is %d\n", prev_starting_point, starting_point);
        if ((pid = Fork()) == 0)
        {
            /* Child runs user job */
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            char dest[128];
            strcpy(dest, "/bin/");
            strcat(dest, argv[prev_starting_point]);

            if (execve(dest, argv + prev_starting_point, environ) < 0) // 일단 임시방편
            {                                                          // ex) /bin/ls ls -al &
                printf("%s Command not found.\n", argv[prev_starting_point]);
                exit(0);
            }
        }
        else
        {
            /* Parent waits for foreground job to terminate */
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            pipe_running(argv, starting_point);
        }
    }
    else
    {
        char dest[128];
        strcpy(dest, "/bin/");
        strcat(dest, front[0]);               // argv 없에기!
        if (execve(dest, front, environ) < 0) // 일단 임시방편
        {                                     // ex) /bin/ls ls -al &
            printf("%s Command not found.\n", argv[prev_starting_point]);
            exit(0);
        }
    }
}

/*find if there are pipe in argv*/
void find_pipe(char **argv, char *front[], char *back[], bool *pipeFlag, int *starting_point)
{
    int front_cnt = 0;
    int back_cnt = 0;
    int i = 0;
    /*front argv를 채워 넣는 과정*/
    for (i = *starting_point; argv[i] != 0; i++, front_cnt++)
    {
        if (!strcmp(argv[i], "|"))
        {
            *pipeFlag = true;
            argv[i] = NULL;
            *starting_point = i + 1; // 자신 뒤로 starting point를 옮기기
            break;
        }
        front[front_cnt] = argv[i];
    }

    if (argv[i] == NULL && pipeFlag == false) // 파이프도 없는데 끝까지 도달했다? out!
    {
        *starting_point = i;
        return;
    }

    for (i = *starting_point; argv[i] != 0; i++, back_cnt++)
    {
        back[back_cnt] = argv[i];
    }
}

/*determine if there are two qutoes in sentence*/
int pair_clear(char *buf, char c)
{
    int temp[2];
    int count = 0;
    for (int i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == c)
        {
            temp[count % 2] = i;
            count++;
            if (!(count % 2))
            { // pair가 맞을 때
                buf[temp[0]] = ' ';
                buf[temp[1]] = ' ';
            }
        }
    }
}
