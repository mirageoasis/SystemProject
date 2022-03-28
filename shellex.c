/* $begin shellmain */
#include "csapp.h"
#include <errno.h>
#define MAXARGS 128
int fds[MAXARGS - 1][2]; /*파일 디스크립터 저장용*/

/* Function prototypes */
void eval(char *cmdline, bool got_piped, int cardinality);
int parseline(char *buf, char **argv, bool *pipeFlag);
int builtin_command(char **argv);
int pipe_running(char **argv, int starting_point);
void find_pipe(char **argv, char *front[], char *back[], bool *pipeFlag, int *starting_point);

int main()
{
    char cmdline[MAXLINE]; /* Command line */
    while (1)
    {
        /* Read */
        printf("> ");
        fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);
        // tester();
        /* Evaluate */
        eval(cmdline, false, 0);
    }
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline, bool got_piped, int step)
{
    char *argv[MAXARGS];   /* Argument list execve() */
    char buf[MAXLINE];     /* Holds modified command line */
    int bg;                /* Should the job run in bg or fg? */
    pid_t pid;             /* Process id */
    int status;            /*자식 프로세스 종료 여부 확인*/
    char *next_command;    /*(일단 파이프 용도로 만듬)다음에 넘겨줄 명령어*/
    bool pipeFlag = false; /*파이프인지 여부를 판단*/
    strcpy(buf, cmdline);
    bg = parseline(buf, argv, &pipeFlag);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    // fprintf(stdout, "now command %s\n", cmdline);

    // for (int i = 0; argv[i] != NULL; i++)
    // {
    //     fprintf(stdout, "the %dth argument is %s\n", i, argv[i]);
    // }

    if (!builtin_command(argv))
    { // quit -> exit(0), & -> ignore, other -> run

        // printf("hi my name is !\n");
        if ((pid = Fork()) == 0)
        {
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
            }
        }
        else
        {
            /* Parent waits for foreground job to terminate */
            if (!bg)
            {
                Wait(&status);
                // fprintf(stdout, "eval caculated!\n");
            }
            else // when there is background process!
                printf("%d %s", pid, cmdline);
        }
    }
    // fprintf(stdout, "this is the end\n");
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
    if (!strcmp(argv[0], "exit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;
    return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv, bool *pipeFlag)
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

        if (!strcmp(argv[argc], "|"))
        { // 지금 문자열이 | 인지 확인 중
            *pipeFlag = true;
            // break;
        }
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
        assert(0);
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
        // close(fd[1]);
        // dup2(fd[0], STDIN_FILENO);
        char dest[128];
        strcpy(dest, "/bin/");
        strcat(dest, front[0]);               // argv 없에기!
        if (execve(dest, front, environ) < 0) // 일단 임시방편
        {                                     // ex) /bin/ls ls -al &
            printf("%s Command not found.\n", argv[prev_starting_point]);
        }
    }
}

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
            // fprintf(stdout, "%d %d\n", i, i + 1);
            *starting_point = i + 1; // 자신 뒤로 starting point를 옮기기
            break;
        }
        front[front_cnt] = argv[i];
    }

    if (argv[i] == NULL && pipeFlag == false) // 파이프도 없는데 끝까지 도달했다?
    {
        *starting_point = i;
        return;
    }

    for (i = *starting_point; argv[i] != 0; i++, back_cnt++)
    {
        back[back_cnt] = argv[i];
    }
}