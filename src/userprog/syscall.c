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

/*
  Validates a pointer by making sure is in userspace and has a valid pagedir.
  If not, terminates the process.
*/
void val_ptrs (void *ptr, struct intr_frame *f, int num, int size){
  int* pagedir = thread_current ()->pagedir;
  if (!is_user_vaddr (ptr) || !is_user_vaddr (ptr + size * num - 1) // under PHYS SPACE
    || pagedir_get_page(pagedir, ptr) == NULL                       // Checks so it is in user space
    || pagedir_get_page(pagedir, ptr + size * num - 1) == NULL)
    {
      syscall_exit (f, -1);
    }
}
/*
  Validates a buffer.
*/
void val_buff (void *ptr, struct intr_frame *f, int size) {
  for (int i = 0; i < size; i++){
      val_ptrs (ptr + i, f, 1, 1);
    }
}


static void
syscall_handler (struct intr_frame *f UNUSED) {

  //Validate esp
  uint32_t* args = ((uint32_t*) f->esp);
  val_ptrs (&args[0], f, 1, sizeof (uint32_t *));

  void** status = *(void**) (f->esp + 4);
  int syscall_nr = * (int*) f->esp;

  switch(syscall_nr){ 
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_CREATE:
      // Checks so that the where the file is created is valid.
      //      (void*)name                 int pointer
      val_ptrs (&args[1], f, 2, sizeof (uint32_t * )); 
      // checks so all the used space by the file is valid
      //        name*               size of file
      val_ptrs ((char *) args[1], f, args[2], sizeof (char));
      syscall_create(f);
      break;
    case SYS_OPEN:
      // Because it is a pointer to a pointer, we need to check it twice
      val_ptrs (&args[1], f, 1, sizeof (uint32_t * ));
      val_ptrs ((void *) args[1], f, 0, sizeof (uint32_t * ));
      syscall_open(f);
      break;
    case SYS_CLOSE:
      val_ptrs (&args[1], f, 1, sizeof (uint32_t * ));
      syscall_close(f);
      break;
    case SYS_READ:
      val_ptrs (&args[1], f, 3, sizeof (uint32_t * ));
      val_buff ((void *) args[2], f, args[3]);
      syscall_read(f);
      break;
    case SYS_WRITE:
      val_ptrs (&args[1], f, 3, sizeof (uint32_t * ));
      val_buff ((void *) args[2], f, args[3]);
      syscall_write(f);
      break;
    case SYS_EXIT:
      val_ptrs (&args[1], f, 1, sizeof (uint32_t * ));
      syscall_exit(f, status);
      break;
    case SYS_EXEC:
      // Because it is a pointer to a pointer, we need to check it twice.
      val_ptrs (&args[1], f, 1, sizeof (uint32_t * ));
      val_ptrs ((void *) args[1], f, 1, sizeof (uint32_t * ));
      syscall_exec(f);
      break;
    case SYS_WAIT:
      val_ptrs (&args[1], f, 1, sizeof (uint32_t * ));
      syscall_wait(f);
      break;
  }
}

void syscall_wait(struct intr_frame *f){
  void *tid = *(void**) (f->esp + 4);
  f->eax = process_wait(tid);
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
  if(name == NULL){
    f->eax = -1;
    syscall_exit(f, -1);
    return;
  }
  f -> eax = filesys_create(name, size); // Return true if successful, else false
} 

void
syscall_exit (struct intr_frame *f, int status) {
  thread_current()->parent_child->exit_status = status;
  f->eax = status;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit ();
}

void syscall_open(struct intr_frame *f){
  const void *name = *(void**) (f->esp + 4);
  if (name == NULL){
    f -> eax = -1;
    syscall_exit(f, -1);
    return;
  }
  struct file *opened_file = filesys_open(name);
  if (opened_file == NULL){
    f -> eax = -1;
    return;
  }

  f -> eax = thread_get_fd(opened_file);
}

void syscall_close(struct intr_frame *f){
  const int fd = *(int*) (f->esp + 4);
  if (fd > 128){
    f -> eax = -1;
    return;
  }
  file_close(thread_get_file(fd));
  thread_remove_fd(fd);
}

void syscall_read(struct intr_frame *f){
  int fd = *(int*) (f-> esp + 4);
  char *buffer = *(void**) (f->esp + 8);
  unsigned size = *(unsigned*) (f->esp + 12);

  // Invalid fd
  if (fd > 128 || fd < 0){
    f -> eax = -1;
    syscall_exit(f, -1);
    return;
  }

  if (size == 0){
    f -> eax = 0;
    return;
  }

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

  if (fd > 128 || fd < 0){
    f -> eax = -1;
    syscall_exit(f, -1);
    return;
  }
  if (size == 0){
    f -> eax = 0;
    return;
  }

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