#include "csapp.h"

extern int gpid_now;
// extern char *cmdline;
// extern void add_job(int pid, int status, char *cmd_line);

void sigchild_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);