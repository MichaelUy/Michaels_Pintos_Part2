#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;

void syscall_init (void);

bool validate_addr(const void* uddr);
bool validate_buffer(const void* uaddr, off_t size);
bool validate_string(const char* uaddr);
void halt(void) NO_RETURN;
void exit(int status) NO_RETURN;
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close (int fd);
bool validate_addr(const void* uddr);
bool validate_buffer(const void* uaddr, off_t size);
bool validate_string(const char* uaddr);

uint32_t getArg(void**);

#endif /* userprog/syscall.h */
