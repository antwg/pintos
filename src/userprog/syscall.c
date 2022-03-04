#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "threads/init.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) {

//if(!valid_pointer(f->esp)){
  //f-eax = -1
  //exit_handler(-1);
//}

  int syscall_nr = * (int*) f->esp;
  
  switch(syscall_nr){
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_CREATE:
      syscall_create(f);
      break;
    case SYS_OPEN:
      syscall_open(f);
      break;
    case SYS_CLOSE:
      syscall_close(f);
      break;
    case SYS_READ:
      syscall_read(f);
      break;
    case SYS_WRITE:
      syscall_call(f);
      break;
    case SYS_EXIT:
      syscall_exit(f);
      break;
    case SYS_EXEC:
      syscall_exec(f);
      break;
    case SYS_WAIT:
      syscall_wait(f);
      break;
  }
}

void syscall_wait(struct intr_frame *f){
  f->eax = process_wait(*(void**) (f->esp + 4));
}

void syscall_exec(struct intr_frame *f){
  void *name = *(void**) (f->esp + 4);
  f->eax = process_execute(name);
}

void syscall_halt(){
  power_off();
}

void syscall_create(struct intr_frame *f){
  const void *name = *(void**) (f->esp + 4);
  unsigned size = *(unsigned*) (f->esp + 8);
  f -> eax = filesys_create(name, size); // Return true if successful, else false
  if (f->eax == -1)
    {
      syscall_exit (f);
    }
}

void
syscall_exit (struct intr_frame *f)
{
  //int *status = thread_current()->parent_child->exit_status;//*(void**) (f->esp + 4);
  thread_current()->parent_child->exit_status = *(void**) (f->esp + 4);
  //thread_current()->parent_child->exit_status = status;
  printf("%s: exit(%d)\n", thread_name(), thread_current()->parent_child->exit_status);
  //f->eax = status;
  thread_exit ();
}

void syscall_open(struct intr_frame *f){
  const void *name = *(void**) (f->esp + 4);
  struct file *opened_file = filesys_open(name);
  if (opened_file == NULL){
    f -> eax = -1;
    return;
  }
  f -> eax = thread_get_fd(opened_file);
}

void syscall_close(struct intr_frame *f){
  const int fd = *(int*) (f->esp + 4);
  file_close(thread_get_file(fd));
  thread_remove_fd(fd);
}

void syscall_read(struct intr_frame *f){
  int fd = *(int*) (f-> esp + 4);
  char *buffer = *(void**) (f->esp + 8);
  unsigned size = *(unsigned*) (f->esp + 12);

  // read from console
  if (fd == 0){ 
    for (unsigned i = 0; i < size; i++){
      buffer[i] = input_getc();
    }
    f -> eax = size;
    return;
  }

  // Illegal argument
  if (fd == 1){
    f -> eax = -1;
    return;
  }

  // Read from file
  int read_bits = (int) file_read(thread_get_file(fd), buffer, size);
  if(read_bits == 0){
    read_bits = -1;
  }
  f -> eax = read_bits;
}

void syscall_call (struct intr_frame *f){
  int fd = *(int*) (f-> esp + 4);
  const void *buffer = *(void**) (f->esp + 8);
  unsigned size = *(unsigned*) (f->esp + 12);
  
  // Write to console
  if (fd == 1){ 
    putbuf(buffer, size);
    f -> eax = size; 
    return;
  }

  // Illegal argument
  if (fd == 0){
    f -> eax = -1;
    return;
  }
  
  // Write to file
  int written_bits = file_write(thread_get_file(fd), buffer, size);
  if(written_bits == 0){
    written_bits = -1;
  }
  f -> eax = written_bits;
}