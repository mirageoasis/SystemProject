#include "signal_handler.h"

extern int gpid_now;
extern char *cmdline;
extern void add_job(int pid, char *cmd_line);

void sigchild_handler(int sig)
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;   // pid
    int wstatus; // 종료상태
    Sigfillset(&mask_all);
    while ((pid = waitpid(-1, &wstatus, WUNTRACED | WNOHANG)) > 0)
    { /* Reap child */
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        // delete_job(pid); /* Delete the child from the job list */
        if (WIFEXITED(wstatus))
        {
            //정상적으로 나갔을 때 exit을 불러서
        }
        else if (WIFSIGNALED(wstatus))
        {
            //시그널에 의해서 종료가 되었을 때
            sio_puts("now in sigchild function!\n");
        }
        else if (WIFSTOPPED(wstatus))
        {
            // 시그널에 의해서 멈췄을 때
        }
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    // if (errno != ECHILD)
    //     Sio_error("waitpid error on sig child function!");
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

void sigstp_handler(int sig)
{
    // 일단 처음 실행된 작업인 경우만 계산해준다.
    sigset_t mask_all, prev_all;
    pid_t pid;   // pid
    int wstatus; // 종료상태
    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    // 아무것도 삭제할꺼 없을 때는 어캐함? 일 끝내고 온거는 해당 안되네 ;; 대충 해결 한 듯 ㅇㅇ
    assert(gpid_now != getpid());
    if (gpid_now)
    {
        sio_putl(gpid_now);
        Kill(-gpid_now, SIGTSTP);
        // add_job(gpid_now, cmdline);
    }
    gpid_now = 0; // 다시 무효화
    Sigprocmask(SIG_UNBLOCK, &mask_all, &prev_all);
}
