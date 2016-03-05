#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "userprog/syscall.h"

struct child {
    pid_t pid;
    bool   wait;
    bool   exit;
    int    ret;
    struct list_elem elem;
    struct semaphore exit_sema;
};

tid_t process_execute (const char *file_name);

int process_wait (pid_t);
void process_exit (void);
void process_activate (void);

struct child* getChild(pid_t pid);


#endif /* userprog/process.h */
