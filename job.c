#include "job.h"

void job_list()
{
    JOB_INFO *cur = head; // head 설정

    char job_status[10];
    char option[2];
    while (cur != NULL)
    {
        switch (cur->status)
        {
        case RUNNING:
            strcpy(job_status, "Running");
            break;
        case DONE:
            strcpy(job_status, "Done");
            break;
        case STOPPED:
            strcpy(job_status, "Stopped");
            break;
        }
        fprintf(stdout, "[%d]%s %s          %s", cur->idx, " ", job_status, cur->cmd); // cmd명령어에 엔터있어서 딱히 개행 안함 ㅋㅋㅋ
        cur = cur->next;
    }
}

void add_job(int pid, int status, int fgFlag, char *cmd_line)
{
    /*프로세스가 하나도 추가가 되지 않았다면*/
    JOB_INFO *pnew = (JOB_INFO *)malloc(sizeof(JOB_INFO));
    pnew->pid = pid;
    pnew->status = status;
    pnew->fgFlag = fgFlag;
    strcpy(pnew->cmd, cmd_line);
    pnew->next = NULL;
    assert(cmd_line != NULL);
    assert(pnew != NULL);
    if (head == NULL)
    {
        pnew->idx = 1;
        head = pnew;
    }
    else
    {
        assert(head != NULL);
        JOB_INFO *cur = head;
        JOB_INFO *prev;

        /*순회하면서 마지막 친구 찾아주기 prev가 진짜다 cur는 그냥 순회하다가 null 찍으면 끝!*/
        while (cur != NULL)
        {
            prev = cur;
            cur = cur->next;
        }
        /*pnew 정보 입력*/
        pnew->idx = (prev->idx) + 1;
        /*pnew 이전 친구 next 설정하기*/
        prev->next = pnew;
    }

    Sio_puts("[");
    Sio_putl(pnew->idx);
    Sio_puts("]");
    Sio_putl(pid);
    Sio_puts("\n");
}

JOB_INFO *find_job(int idx, pid_t pid, int mod)
{
    JOB_INFO *temp = head;
    while (temp != NULL)
    {
        if (mod == INDEX)
            if (idx == temp->idx)
                return temp;
        if (mod == PID)
            if (pid == temp->pid)
                return temp;

        temp = temp->next;
    }

    return NULL;
}

pid_t change_job(int idx, int pid, int status, int fgFlag, int mod)
{
    JOB_INFO *temp = head;

    while (temp != NULL)
    {
        if (mod == INDEX)
            if (idx == temp->idx)
            {
                // Sio_puts("changed idx: ");
                // Sio_putl(temp->idx);
                // Sio_puts("\n");
                temp->status = status;
                temp->fgFlag = fgFlag;
                return temp->pid;
            }
        if (mod == PID)
            if (pid == temp->pid)
            {
                // Sio_puts("changed idx: ");
                // Sio_putl(temp->idx);
                // Sio_puts("\n");
                temp->status = status;
                temp->fgFlag = fgFlag;
                return temp->pid;
            }
        temp = temp->next;
    }

    return -1;
}

/*무조건 find_job 이 확인되고 나서 쓴다*/
int delete_job(int idx, pid_t pid, int mod)
{
    JOB_INFO *cur = head;
    JOB_INFO *prev = head;
    while (cur != NULL)
    {
        if (mod == INDEX)
            if (idx == cur->idx)
                break;
        if (mod == PID)
            if (pid == cur->pid)
                break;
        prev = cur; // 전에꺼 저장
        cur = cur->next;
    }
    // Sio_puts("deleted to job!\n");
    // Sio_puts("[");
    // Sio_putl(cur->idx);
    // Sio_puts("] pid ");
    // Sio_putl(cur->pid);
    // Sio_puts("      ");
    // Sio_puts(cur->cmd);
    if (cur == head)
    {
        head = cur->next; // NULL로 만드는거나
        // Sio_puts("head!\n");
        free(cur);
    }
    else
    {
        prev->next = cur->next;
        free(cur);
    }

    return 0;
}