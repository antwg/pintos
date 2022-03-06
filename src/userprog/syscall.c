#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "threads/init.h"
#include "devices/input.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
void valid_pointer(void *ptr, struct intr_frame *f, int arg_num, int arg_size){
  if (!is_user_vaddr (ptr) || 
      !is_user_vaddr (ptr + arg_size * arg_num - 1) || 
      pagedir_get_page(thread_current()->pagedir, ptr) == NULL ||
      pagedir_get_page (thread_current ()->pagedir, ptr + arg_size * arg_num - 1) == NULL) { //ptr > PHYS_BASE
    //printf("Close program, invalid ptr or page\n");
    f->eax = -1;
    syscall_exit(f, -1);
  } 
  //printf("Ptr ok\n");
}

void validate_buffer(void *ptr, int size, struct intr_frame *f){
  //printf("Size: %d", size);
  for (int i = 0; i < size; i++){ 
    valid_pointer(ptr + i, f, 1, 1);
  }
}


static void
syscall_handler (struct intr_frame *f UNUSED) {

  valid_pointer(f->esp, f, 1, sizeof (uint32_t * ));  //maybe switch so that all arguments is in args
  //printf("ptr valid in syscall handler");
  //uint32_t* args = ((uint32_t*) f->esp); 
  void** arg1 = *(void**) (f->esp + 4);
  void** arg2 = *(void**) (f->esp + 8);
  void** arg3 = *(void**) (f->esp + 12);
  int syscall_nr = * (int*) f->esp;
  //printf("entering the interruppt switch case");
  switch(syscall_nr){ 
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_CREATE:
      validate_buffer(arg1, arg2, f);
      syscall_create(f);
      break;
    case SYS_OPEN:
      validate_buffer(arg1, arg2, f);
      syscall_open(f);
      break;
    case SYS_CLOSE:
      syscall_close(f);
      break;
    case SYS_READ:
      validate_buffer(arg2, arg3, f);
      syscall_read(f);
      break;
    case SYS_WRITE:
      validate_buffer(arg2, arg3, f);
      syscall_write(f);
      break;
    case SYS_EXIT:
      valid_pointer(arg1, f, 1, sizeof (uint32_t *));
      syscall_exit(f, arg1);
      break;
    case SYS_EXEC:
      validate_buffer(arg1, arg2, f);
      syscall_exec(f);
      break;
    case SYS_WAIT:
      valid_pointer(arg1, f);
      syscall_wait(f);
      break;
  }
}

void syscall_wait(struct intr_frame *f){
  f->eax = process_wait(**(tid_t**) (f->esp + 4));
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
  //if (f->eax == -1)
  //  {
  //    //syscall_exit (f);
  //  }
}

void
syscall_exit (struct intr_frame *f, int status) 
{
  //uint32_t* args = ((uint32_t*) f->esp);
  //int status =  args[1];
  f->eax = status;
  printf("%s: exit(%d)\n", thread_name(), status);
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

void syscall_write (struct intr_frame *f){
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