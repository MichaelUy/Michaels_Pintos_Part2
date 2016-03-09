#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
bool validate_addr(const void* uddr);
bool validate_buffer(const void* uaddr, off_t size);
bool validate_string(const char* uaddr);
uint32_t getArg(void**);
struct file* getFileP(int);

struct lock file_lock;

struct fds {
    int file_desc;
    struct file* file_ptr;
    struct list_elem elem;
};

void syscall_init (void) {
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    lock_init (&file_lock);
}

uint32_t getArg(void** vp) {
    uint32_t* d = (uint32_t*)*vp;
    *vp += 4; // update esp
    return *d;
}


bool validate_addr(const void* uaddr) {
     return uaddr &&
            is_user_vaddr(uaddr) &&
            pagedir_get_page(thread_current()->pagedir, uaddr);
}

bool validate_buffer(const void* uaddr, off_t size) {
    int i;
    for (i = 0; i < size; ++i, ++uaddr) {
        if (!validate_addr(uaddr)) return false;
    }
    return true;
}

bool validate_string(const char* uaddr) {
    if (!uaddr) return false;
    int i;
    for(i = 0; *uaddr != '\0'; ++i, ++uaddr) {
        if(!validate_addr(uaddr)) return false;
    }
    return true;
}

static void syscall_handler (struct intr_frame *f UNUSED) 
{
    int x, y;
    char* cp;
    void* vp;
    pid_t  p;
    unsigned u;

    void* fesp = f->esp;

    if (!validate_buffer(fesp, 16)) exit(-1);

    switch (getArg(&f->esp)) {
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
            printf ("system call [%d] not implemented!\n", f->vec_no);
    }
    // fix f->esp
    f->esp = fesp;
}


/** project system calls **/

// calls shutdown_power_off()
void halt (void) {
    shutdown_power_off();
}

// terminates the user program, returning status to the kernel
void exit (int status) {
    struct thread *t = thread_current();
    // if parent still exists, and we have a struct child_t*, set return value
    if (thread_get(t->parent) && t->cp) {
        t->cp->ret = status;
    }
    printf ("%s: exit(%d)\n",t->name,status);
    // thread_exit (when run by user) will call process_execute to get
    // rid of all process-related stuff
    thread_exit();
}

// Runs the executable whose name is given in cmd_line,
// passing any given arguments, and returns the new process's program id (pid).
// Must return pid -1, which otherwise should not be a valid pid, if the
// program cannot load or run for any reason. The parent process cannot return
// from the exec until it knows whether the child process successfully loaded
// its executable. You must use appropriate synchronization to ensure this. 
pid_t exec (const char *cmdline) {
    if (!validate_string(cmdline)) exit(-1);
    return process_execute(cmdline);
}

// Wait for a child process, and retrieve its exit status
int wait (pid_t pid) {
    return process_wait(pid);
}

// create a new file with an initial size
// return whether or not successful
bool create (const char *file, unsigned initial_size) {
    if (!validate_string(file)) exit(-1);
    lock_acquire(&file_lock);
    bool ret = filesys_create(file, initial_size);
    lock_release(&file_lock);
    return ret;
}

// delete a file
// return whether or not successful
bool remove (const char *file) {
    if (!validate_string(file)) exit(-1);
    lock_acquire(&file_lock);
    bool ret = filesys_remove(file);
    lock_release(&file_lock);
    return ret;
}

// open a file, and return a file descriptor
int open (const char *file) {
    if (!validate_string(file)) exit(-1);
    lock_acquire(&file_lock);
    int file_desc = thread_current()->fd++; //file_desc takes and curr file desc
    struct fds* fdsp;                       // and increments
    struct file* f = filesys_open(file);
    if(f) {
        lock_release(&file_lock);
        return -1;
    }
    fdsp = (struct fds*)malloc(sizeof(fdsp));
    ASSERT(fdsp);
    fdsp->file_desc = file_desc;
    fdsp->file_ptr  = f;
    list_push_back(&thread_current()->files, &fdsp->elem);
    lock_release(&file_lock);
    return file_desc;
}


struct file* getFileP(int fd) {
    struct fds*  fdsp = NULL;
    struct thread* t = thread_current();
    struct list_elem* e = NULL;
    for(e = list_begin(&t->files); e != list_end(&t->files); e = list_next(e)) {
        fdsp = list_entry(e, struct fds, elem);
        if (fdsp->file_desc == fd) return fdsp->file_ptr;
    }
    return NULL;
}

// returns the size in bytes of the file specified by the fd
int filesize (int fd) {
    lock_acquire(&file_lock);
    struct file* f = getFileP(fd);
    if(f) {
        lock_release(&file_lock);
        return -1;
    }
    int ret = file_length(f);
    lock_release(&file_lock);
    return ret;
}

// read size bytes from the fd open into buffer
// returns number of bytes actually read
// fd 0 reads from the keyboard using input_getc()
int read (int fd, void *buffer, unsigned size) {
    if (!validate_buffer(buffer, size)) exit(-1);
    if (fd == 0)
    {
        //write read from keyboard
        unsigned i;
        for(i = 0; i < size; i++)
        {
            ((char*)buffer)[i] = input_getc();
        }
    }
    lock_acquire(&file_lock);
    struct file* f = getFileP(fd);
    if(f) {
        lock_release(&file_lock);
        return -1;
    }
    int ret = file_read(f, buffer, size);
    lock_release(&file_lock);
    return ret;
}

// write size bytes from buffer to the open file fd.
// returns number of bytes actually written
// fd 1 writes to the console using one call to putbuf() as long as size isn't
//    longer than a few hundred bytes (weird stuff happens)
int write (int fd, const void *buffer, unsigned size) {
    if (!validate_buffer(buffer, size)) exit(-1);
    if(fd == 1) {
        //write  to console
        putbuf(buffer, size);
        return size;
    }
    struct file* f = getFileP(fd);
    if(f) {
        lock_release(&file_lock);
        return -1;
    }
    int ret = file_write(f, buffer, size);
    lock_release(&file_lock);
    return ret;
}

// changes the next byte to be read or written
void seek (int fd, unsigned position) {
    
    lock_acquire(&file_lock);
    struct file* f = getFileP(fd);
    if(f)
    {
        file_seek(f, position);
    }
    lock_release(&file_lock);
}

// return the position of the next byte to be read or written
unsigned tell (int fd) {
    lock_acquire(&file_lock);
    struct file* f = getFileP(fd);
    if(f)
    {
        lock_release(&file_lock);
        return -1;
    }
    unsigned ret = file_tell(f);
    lock_release(&file_lock);
    return ret;
}

// close file descriptor fd.
// make sure to close all fds when a process ends
void close (int fd) {
    lock_acquire(&file_lock);
    struct thread* t = thread_current();
    struct list_elem* e;
    for(e = list_begin(&t->files);e != list_end(&t->files); e= list_next(e))
    {
        struct fds* fdsp = list_entry(e, struct fds, elem);
        if(fdsp->file_desc == fd)
        {
            list_remove(&fdsp->elem);
            file_close(fdsp->file_ptr);
            free(fdsp);
        }
    }
    lock_release(&file_lock);
}



