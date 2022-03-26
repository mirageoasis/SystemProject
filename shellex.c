/* $begin shellmain */
#include "csapp.h"
#include <errno.h>
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline, char **envp);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main(int argc, char **argv, char **envp)
{
    char cmdline[MAXLINE]; /* Command line */
    environ = envp;
    while (1)
    {
        /* Read */
        printf("> ");
        fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            _exit(0);
        // tester();
        /* Evaluate */
        eval(cmdline, envp);
    }
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline, char **envp)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    int status;          /*자식 프로세스 종료 여부 확인*/
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_command(argv))
    { // quit -> exit(0), & -> ignore, other -> run

        if ((pid = Fork()) == 0)
        {
            /* Child runs user job */
            char *dest = (char *)malloc(sizeof(char) * (strlen(argv[0]) + 6));
            strcpy(dest, "/bin/");
            strcat(dest, argv[0]);
            // fprintf(stdout, "converted into %s\n", dest);
            if (execve(dest, argv, environ) < 0) // 일단 임시방편
            {                                    // ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
            }
            free(dest);
        }

        /* Parent waits for foreground job to terminate */
        if (!bg)
        {
            // fprintf(stdout, "this is not background\n");
            wait(&status); /*parent waits for foreground child*/ // it is temporary
        }
        else // when there is background process!
            printf("%d %s", pid, cmdline);
    }
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
        _exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
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
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
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
