#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* All call funtions return void but instead use "f -> eax" to 
   return their respective values.*/

struct intr_frame;

void syscall_init (void);

// Helper functions
void halt_call(void);

void create_call(struct intr_frame *f);

void open_call(struct intr_frame *f);

void close_call(struct intr_frame *f);

void read_call(struct intr_frame *f);

void write_call (struct intr_frame *f);

void exec_call(struct intr_frame *f);

#endif /* userprog/syscall.h */
