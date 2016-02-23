#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void syscall_init (void) 
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

uint32_t getArg(void** vp) {
    uint32_t* d = (uint32_t*)*vp;
    *vp -= 4; // update esp
    return *d;
}


static void syscall_handler (struct intr_frame *f UNUSED) 
{
    // thread_exit ();
    int x, y;
    char* cp;
    void* vp;
    pid_t  p;
    unsigned u;

    switch (f->vec_no) {
        case SYS_HALT:
            halt();
            break;
        case SYS_EXIT:
            x      = (int)      getArg(&f->esp); // status
            exit(x);
            break;
        case SYS_EXEC:
            cp     = (char*)    getArg(&f->esp); // console command
            f->eax = (uint32_t) exec(cp);        // child (pid_t)
            break;
        case SYS_WAIT:
            p      = (pid_t)    getArg(&f->esp); // pid
            f->eax = (uint32_t) wait(p);         // exit status (int)
            break;
        case SYS_CREATE:
            cp     = (char*)    getArg(&f->esp); // filename
            x      = (int)      getArg(&f->esp); // size
            f->eax = (uint32_t) create(cp, x);   // success (bool)
            break;
        case SYS_REMOVE:
            cp     = (char*)    getArg(&f->esp); // filename
            f->eax = (uint32_t) remove(cp);      // success (bool)
            break;
        case SYS_OPEN:
            cp     = (char*)    getArg(&f->esp); // filename
            f->eax = (uint32_t) open(cp);        // file descriptor (int)
            break;
        case SYS_FILESIZE:
            x      = (int)      getArg(&f->esp); // file descriptor
            f->eax = (uint32_t) filesize(x);     // size (int)
            break;
        case SYS_READ:
            x      = (int)      getArg(&f->esp); // file descriptor
            vp     = (void*)    getArg(&f->esp); // buffer
            y      = (int)      getArg(&f->esp); // size
            f->eax = (uint32_t) read(x, vp, y);  // numBytes read (int)
            break;
        case SYS_WRITE:
            x      = (int)      getArg(&f->esp); // file descriptor
            vp     = (void*)    getArg(&f->esp); // buffer
            y      = (int)      getArg(&f->esp); // size
            f->eax = (uint32_t) write(x, vp, y); // numBytes written (int)
            break;
        case SYS_SEEK:
            x      = (int)      getArg(&f->esp); // file descriptor
            u      = (unsigned) getArg(&f->esp); // position
            seek(x, u);
            break;
        case SYS_TELL:
            x      = (int)      getArg(&f->esp); // file descriptor
            f->eax = (uint32_t) tell(x);         // position (unsigned)
            break;
        case SYS_CLOSE:
            x      = (int)      getArg(&f->esp); // file descriptor
            close(x);
            break;
        default:
            printf ("system call not implemented!\n");
    }
}


/** project system calls **/

// calls shutdown_power_off()
void halt (void) {
    return;
}

// terminates the user program, returning status to the kernel
void exit (int status) {
    return;
}

// Runs the executable whose name is given in cmd_line,
// passing any given arguments, and returns the new process's program id (pid).
// Must return pid -1, which otherwise should not be a valid pid, if the
// program cannot load or run for any reason. The parent process cannot return
// from the exec until it knows whether the child process successfully loaded
// its executable. You must use appropriate synchronization to ensure this. 
pid_t exec (const char *cmd_line) {
    return (pid_t)-1;
}

// Wait for a child process, and retrieve its exit status
int wait (pid_t pid) {
    return -1;
}

// create a new file with an initial size
// return whether or not successful
bool create (const char *file, unsigned initial_size) {
    return false;
}

// delete a file
// return whether or not successful
bool remove (const char *file) {
    return false;
}

// open a file, and return a file descriptor
int open (const char *file) {
    return -1;
}

// returns the size in bytes of the file specified by the fd
int filesize (int fd) {
    return -1;
}

// read size bytes from the fd open into buffer
// returns number of bytes actually read
// fd 0 reads from the keyboard using input_getc()
int read (int fd, void *buffer, unsigned size) {
    return -1;
}

// write size bytes from buffer to the open file fd.
// returns number of bytes actually written
// fd 1 writes to the console using one call to putbuf() as long as size isn't
//    longer than a few hundred bytes (weird stuff happens)
int write (int fd, const void *buffer, unsigned size) {
    return -1;
}

// changes the next byte to be read or written
void seek (int fd, unsigned position) {
    return;
}

// return the position of the next byte to be read or written
unsigned tell (int fd) {
    return (unsigned)-1;
}

// close file descriptor fd.
// make sure to close all fds when a process ends
void close (int fd) {
    return;
}



