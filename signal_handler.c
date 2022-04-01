#include "signal_handler.h"

enum
{
    RUNNING,
    DONE,
    STOPPED
}; // job status

extern int gpid_now;
extern char *cmdline;
extern void add_job(int pid, int status, char *cmd_line);
