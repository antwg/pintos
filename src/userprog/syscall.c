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

  int syscall_nr = * (int*) f->esp;
  
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
      close_call(f);
      break;
    case SYS_READ:
      read_call(f);
      break;
    case SYS_WRITE:
      write_call(f);
      break;
    case SYS_EXIT:
      thread_exit();
      break;
    case SYS_EXEC:
      exec_call(f);
      break;
  }
}

void exec_call(struct intr_frame *f){
  void *name = *(void**) (f->esp + 4);
  f->eax = process_execute(name);
}

void halt_call(){
  power_off();
}

void create_call(struct intr_frame *f){
  const void *name = *(void**) (f->esp + 4);
  unsigned size = *(unsigned*) (f->esp + 8);
  f -> eax = filesys_create(name, size); // Return true if successful, else false
}

void open_call(struct intr_frame *f){
  const void *name = *(void**) (f->esp + 4);
  struct file *opened_file = filesys_open(name);
  if (opened_file == NULL){
    f -> eax = -1;
    return;
  }
  f -> eax = thread_get_fd(opened_file);
}

void close_call(struct intr_frame *f){
  const int fd = *(int*) (f->esp + 4);
  file_close(thread_get_file(fd));
  thread_remove_fd(fd);
}

void read_call(struct intr_frame *f){
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

void write_call (struct intr_frame *f){
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