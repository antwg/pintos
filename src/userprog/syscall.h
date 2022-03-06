#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* All call funtions return void but instead use "f -> eax" to 
   return their respective values.*/

struct intr_frame;

void syscall_init (void);

// Helper functions
void syscall_halt(void);

void syscall_create(struct intr_frame *f);

void syscall_open(struct intr_frame *f);

void syscall_close(struct intr_frame *f);

void syscall_read(struct intr_frame *f);

void syscall_call (struct intr_frame *f);

void syscall_exec(struct intr_frame *f);

void syscall_exit(struct intr_frame *f, int status);

void syscall_wait(struct intr_frame *f);

void valid_pointer(void *ptr, struct intr_frame *f, int arg_num, int arg_size);

#endif /* userprog/syscall.h */
