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

void sigchild_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);

void fg_bg_kill_finder(char **argv, int mod);

char cmdline[MAXLINE]; /* Command line */
JOB_INFO *head = NULL;
int gpid_now;

enum
{
    BG,
    FG,
    KILL
};

int main()
{

    Signal(SIGCHLD, sigchild_handler);
    // Signal(SIGINT, sigint_handler);   // set handler for SIGINT // 일단 없에서 디버깅 용도로 사용한다.
    Signal(SIGTSTP, sigtstp_handler); // set handler for SIGTSTP

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

    // Sigfillset(&mask_all);
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
                // fprintf(stdout, "parent pid is %d\n", getpid());
                // fprintf(stdout, "pid of child  is %d\n", pid);
                // fprintf(stdout, "gpid of child is %d\n", gpid_now);
                Waitpid(-1, &status, WUNTRACED);
                // fprintf(stdout, "passed test on Wait on line 119 on eval func\n");
                gpid_now = 0;
                //   fprintf(stdout, "eval caculated!\n");
            }
            else // when there is background process!
            {
                add_job(pid, RUNNING, cmdline);
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
        fg_bg_kill_finder(argv, FG);
        return 1;
    }
    else if (!strcmp(argv[0], "bg"))
    {
        // printf("now on bg option\n");
        fg_bg_kill_finder(argv, BG);
        return 1;
    }
    else if (!strcmp(argv[0], "kill"))
    {
        // Kill()
        fg_bg_kill_finder(argv, KILL);
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

void sigchild_handler(int sig)
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;   // pid
    int wstatus; // 종료상태
    Sigfillset(&mask_all);
    while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0)
    { /* Reap child */
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        if (WIFEXITED(wstatus))
        {
            //정상적으로 나갔을 때 exit을 불러서
            // Sio_puts("job exited normally!\n");
            if (find_job(-1, pid, PID) != -1)
                delete_job(-1, pid, PID); /* Delete the child from the job list */
        }
        else if (WIFSIGNALED(wstatus))
        {
            //시그널에 의해서 종료가 되었을 때
            if (find_job(-1, pid, PID) != -1)
                delete_job(-1, pid, PID); /* Delete the child from the job list */
        }
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    // if (errno != ECHILD)
    //     Sio_error("waitpid error on sig child function!\n");
    // sio_puts("now in sigchild handler!\n");
    errno = olderrno;
}

void sigint_handler(int sig)
{

    sigset_t mask_all, prev_all;
    pid_t pid;   // pid
    int wstatus; // 종료상태
    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    // 아무것도 삭제할꺼 없을 때는 어캐함? 일 끝내고 온거는 해당 안되네 ;; 대충 해결 한 듯 ㅇㅇ
    assert(gpid_now != getpid());
    if (gpid_now)
        Kill(-gpid_now, SIGINT);
    gpid_now = 0; // 다시 무효화
    Sigprocmask(SIG_UNBLOCK, &mask_all, &prev_all);
}

void sigtstp_handler(int sig)
{
    // 일단 처음 실행된 작업인 경우만 계산해준다.
    assert(gpid_now != getpid());
    sigset_t mask_all, prev_all;
    pid_t pid;   // pid
    int wstatus; // 종료상태
    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    if (gpid_now)
    {
        Kill(-gpid_now, SIGTSTP);
        add_job(gpid_now, STOPPED, cmdline); // 다른 파일에 넣었더만 cmd_line의 주소가 제대로 안넘어감;;
    }
    gpid_now = 0; // 다시 무효화
    Sigprocmask(SIG_UNBLOCK, &mask_all, &prev_all);
}

void fg_bg_kill_finder(char **argv, int mod)
{
    if (argv[1] == NULL)
    {
        printf("default option!\n");
    }
    else if (argv[1][0] == '%')
    {
        // printf("percentage option number %d\n", argv[1][1]);
        if (argv[1][1] == '\0') // 기본 사양으로 돌아가게한다.
        {
            printf("default function!\n");
        }
        else if (isdigit(argv[1][1]))
        {
            int idx = atoi(argv[1] + 1);
            // printf("the idx number is %s!\n", argv[1] + 1);
            // printf("the idx number is %d!\n", idx);
            if (find_job(idx, -1, INDEX) != -1) // 찾았으면 ㄱㄱ
            {
                // printf("found the job!\n");
                switch (mod)
                {
                case FG:
                    fprintf(stdout, "here comes the fg command! %d\n", find_job(idx, -1, INDEX));
                    signal(SIGTTIN, SIG_IGN);
                    signal(SIGTTOU, SIG_IGN);

                    tcsetpgrp(STDIN_FILENO, find_job(idx, -1, INDEX));
                    kill(-find_job(idx, -1, INDEX), SIGCONT);
                    tcsetpgrp(STDIN_FILENO, getpgrp());

                    // safe to end protection from signals
                    signal(SIGTTIN, SIG_DFL);
                    signal(SIGTTOU, SIG_DFL);
                    // Kill(-change_job(idx, RUNNING), SIGCONT); // 링크드 리스트 내에서 바꿔주고 sigcont 신호 보내주기
                    break;
                case BG:
                    Kill(-change_job(idx, RUNNING), SIGCONT); // 링크드 리스트 내에서 바꿔주고  신호 보내주기
                    break;
                case KILL:
                    Kill(-change_job(idx, DONE), SIGKILL); // 링크드 리스트 내에서 바꿔주고 kill 신호 보내주기
                    break;
                }
            }
            else
            {
                printf("no such job!\n"); // 모르면 ㄴㄴ
            }
        }
    }
    else
    {
        printf("undefined!\n");
    }
}