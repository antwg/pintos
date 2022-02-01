#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "threads/init.h"

static void syscall_handler (struct intr_frame *);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) {
  printf ("system call!\n");

  int syscall_nr = * (int*) f->esp;

  printf("syscall number is %d\n", syscall_nr);

  switch(syscall_nr){
    case SYS_HALT:
      halt_call();
      break;
    case SYS_CREATE:
      create_call(f);
      break;
    case SYS_OPEN:
      open_call(f);
      break;
    case SYS_CLOSE:
      break;
    case SYS_READ:
      break;
    case SYS_WRITE:
      write_call(f);
      break;
    case SYS_EXIT:
      thread_exit();
      break;
  }
}

void halt_call(){
  power_off();
}

void create_call(struct intr_frame *f){
  const void *name = *(void**) (f->esp + 4);
  unsigned size = *(unsigned*) (f->esp + 8);
  f -> eax = filesys_create(name, size); // Return bool
}

void open_call(struct intr_frame *f){
  const void *name = *(void**) (f->esp + 4);
  f -> eax = filesys_open(name); // TODO Return fd, or -1
}

void write_call (struct intr_frame *f){
  int fd = *(int*) (f-> esp + 4);
  const void *buffer = *(void**) (f->esp + 8);
  unsigned size = *(unsigned*) (f->esp + 12);
  
  // Write to console
  if (fd == 1){ 
    putbuf(buffer, size);
    f -> eax = size; // Return size
    return;
  }
  // Write to file
}
