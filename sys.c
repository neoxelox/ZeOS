/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

#define true 1
#define false 0

// This is used to calculate the logical page number of the user_stack of the thread whith tid = TID.
// It is based on that the masterthread (tid = 0) owns the 20th page of user data (NUM_PAG_DATA is 20).
// remember that we use the user data pages for user global data and user stack(s).
// also take in account that TIDs are given sequentially (masterthread's TID is 0, the next thread has TID = 1, etc).
#define THREAD_USER_STACK_PAGE(TID) (PAG_LOG_INIT_DATA + (NUM_PAG_DATA - 1) + TID)

struct mutex_t mutexes[MAX_MUTEXES];

void *get_ebp();

// Avoid implicit declaration
int sys_mutex_destroy(int mutex_id);

int check_fd(int fd, int permissions)
{
  if (fd != 1)
    return -EBADF;
  if (permissions != ESCRIPTURA)
    return -EACCES;
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
  return -ENOSYS;
}

int sys_getpid()
{
  return current()->PID;
}

int global_PID = 1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  // Only the master thread of the current process can perform fork syscall
  if (current()->TID != 0)
    return -EPERM;

  struct list_head *lhcurrent = NULL;
  union task_union *uchild;

  /* Any free task_struct? */
  if (list_empty(&freequeue))
    return -ENOMEM;

  lhcurrent = list_first(&freequeue);

  list_del(lhcurrent);

  uchild = (union task_union *)list_head_to_task_struct(lhcurrent);

  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));

  /* new pages dir */
  allocate_DIR((struct task_struct *)uchild);

  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag = 0; pag < NUM_PAG_DATA; pag++)
  {
    new_ph_pag = alloc_frame();
    if (new_ph_pag != -1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA + pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i = 0; i < pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA + i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA + i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);

      /* Return error */
      return -EAGAIN;
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag = 0; pag < NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag = 0; pag < NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE + pag, get_frame(parent_PT, PAG_LOG_INIT_CODE + pag));
  }

  /* Copy parent's DATA to child. We will use the range [TOTAL_PAGES-NUM_PAG_DATA..TOTAL_PAGES-1] as temp logical pages to map to */
  for (pag = 0; pag < NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, TOTAL_PAGES - NUM_PAG_DATA + pag, get_frame(process_PT, PAG_LOG_INIT_DATA + pag));
    copy_data((void *)((PAG_LOG_INIT_DATA + pag) << 12), (void *)((TOTAL_PAGES - NUM_PAG_DATA + pag) << 12), PAGE_SIZE);
    del_ss_pag(parent_PT, TOTAL_PAGES - NUM_PAG_DATA + pag);
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID = ++global_PID;
  uchild->task.state = ST_READY;

  uchild->task.TID = 0;
  uchild->task.joined = NULL;
  uchild->task.errno = 0;
  uchild->task.retval = 0;
  init_tls(uchild->task.TLS);

  // Get a free threads_process list
  for (int i = 0; i < NR_TASKS; i++)
  {
    if (list_uninitialized(&(threads_processes[i])))
    {
      uchild->task.threads_process = &(threads_processes[i]);
      break;
    }
  }

  INIT_LIST_HEAD(uchild->task.threads_process);
  list_add_tail(&(uchild->task.list_threads), uchild->task.threads_process);

  int register_ebp; /* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int)get_ebp();
  register_ebp = (register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp = register_ebp + sizeof(DWord);

  DWord temp_ebp = *(DWord *)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp -= sizeof(DWord);
  *(DWord *)(uchild->task.register_esp) = (DWord)&ret_from_fork;
  uchild->task.register_esp -= sizeof(DWord);
  *(DWord *)(uchild->task.register_esp) = temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state = ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);

  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes)
{
  char localbuffer[TAM_BUFFER];
  int bytes_left;
  int ret;

  if ((ret = check_fd(fd, ESCRIPTURA)))
    return ret;
  if (nbytes < 0)
    return -EINVAL;
  if (!access_ok(VERIFY_READ, buffer, nbytes))
    return -EFAULT;

  bytes_left = nbytes;
  while (bytes_left > TAM_BUFFER)
  {
    copy_from_user(buffer, localbuffer, TAM_BUFFER);
    ret = sys_write_console(localbuffer, TAM_BUFFER);
    bytes_left -= ret;
    buffer += ret;
  }
  if (bytes_left > 0)
  {
    copy_from_user(buffer, localbuffer, bytes_left);
    ret = sys_write_console(localbuffer, bytes_left);
    bytes_left -= ret;
  }
  return (nbytes - bytes_left);
}

extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{
  // Only the master thread of the current process can perform exit syscall
  if (current()->TID != 0)
    return;

  /* Manage mutexes where the current process is implicated */

  for (int i = 0; i < MAX_MUTEXES; i++)
  {
    if (!mutexes[i].initialized)
      continue;

    // If im the process who initialized the mutex, unblock all the blocked threads and destroy it
    if (mutexes[i].pid_initializer == current()->PID)
    {
      struct list_head *pos, *aux;
      list_for_each_safe(pos, aux, &(mutexes[i].blockedqueue))
      {
        struct task_struct *tmp = list_head_to_task_struct(pos);

        tmp->state = ST_READY;
        list_del(&(tmp->list));
        list_add_tail(&(tmp->list), &readyqueue);
      }

      // Ensure no one has ownership (in order to call sys_mutex_destroy correctly)
      mutexes[i].pid_owner = -1;
      mutexes[i].tid_owner = -1;

      sys_mutex_destroy(i);
    }
    else // The current process didn't initialize the mutex
    {
      // Delete any blocked thread of the current process from the blockedqueue of the mutex
      struct list_head *pos, *aux;
      list_for_each_safe(pos, aux, &(mutexes[i].blockedqueue))
      {
        struct task_struct *tmp = list_head_to_task_struct(pos);

        if (tmp->PID == current()->PID)
        {
          list_del(&(tmp->list));
        }
      }

      // If im the owner of the mutex pass ownership
      if (mutexes[i].pid_owner == current()->PID)
      {
        mutexes[i].pid_owner = -1;
        mutexes[i].tid_owner = -1;

        if (!list_empty(&(mutexes[i].blockedqueue)))
        {
          struct list_head *tmp = list_first(&(mutexes[i].blockedqueue));
          struct task_struct *next_owner = list_head_to_task_struct(tmp);
          list_del(&(next_owner->list));

          mutexes[i].pid_owner = next_owner->PID;
          mutexes[i].tid_owner = next_owner->TID;
          next_owner->state = ST_READY;
          list_add_tail(&(next_owner->list), &readyqueue);
        }
      }
    }
  }

  int threads_num = 0;

  /* Remove resources from all the threads of the current process */

  struct list_head *pos;
  list_for_each(pos, current()->threads_process)
  {
    struct task_struct *tmp = list_head_to_task_struct(pos);

    tmp->PID = -1;
    tmp->TID = -1;
    list_add_tail(&(tmp->list), &freequeue); // Free task_struct

    threads_num++;
  }

  /* Remove resources of the current process */

  DESTROY_LIST_HEAD(current()->threads_process);

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (int i = 0; i < NUM_PAG_DATA + threads_num - 1; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA + i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA + i);
  }

  // Restart execution of the next ready process
  switch (sched_next_decide_level())
  {
  case 2:
    sched_next_rr_level2();
    break;
  case 1:
    panic("sys_exit switched to a thread of the same process");
    sched_next_rr_level1();
    break;
  default:
    // Switch to idle_task as the readyqueue is empty
    sched_next_rr(idle_task);
    break;
  }
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum_process;
extern int remaining_quantum_thread;

int sys_get_stats(int pid, struct stats *st)
{
  int i;

  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats)))
    return -EFAULT;

  if (pid < 0)
    return -EINVAL;
  for (i = 0; i < NR_TASKS; i++)
  {
    if (task[i].task.PID == pid)
    {
      task[i].task.p_stats.remaining_ticks = remaining_quantum_process;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}

int sys_pthread_create(int *TID, void *(*wrap_routine)(), void *(*start_routine)(), void *arg)
{
  int result = -1;

  // Check TID
  result = (TID == NULL ? -1 : 0);
  if (result < 0)
    return -EFAULT;

  result = (access_ok(VERIFY_WRITE, TID, sizeof(int)) ? 0 : -1);
  if (result < 0)
    return -EFAULT;

  // Check wrap_routine
  result = (wrap_routine == NULL ? -1 : 0);
  if (result < 0)
    return -EFAULT;

  // Check start_routine
  result = (start_routine == NULL ? -1 : 0);
  if (result < 0)
    return -EFAULT;

  // Check freequeue
  result = (list_empty(&freequeue) ? -1 : 0);
  if (result < 0)
    return -EAGAIN;

  /* Pick a free task */
  struct list_head *free_task = list_first(&freequeue);
  struct task_struct *new_task = list_head_to_task_struct(free_task);
  list_del(free_task); // Delete task from queue

  /* Initialize new task */
  union task_union *new_task_union = (union task_union *)new_task;      // Get the task_union for new_task
  union task_union *current_task_union = (union task_union *)current(); // Get the task_union for current task

  int threads_num = 0;

  struct list_head *pos;
  list_for_each(pos, current()->threads_process)
  {
    threads_num++;
  }

  /* Inherit system data */
  copy_data(current_task_union, new_task_union, sizeof(union task_union));

  /* Assign new TID */
  new_task->TID = threads_num;
  result = copy_to_user(&(new_task->TID), TID, sizeof(int)); // Copy new TID to *TID
  if (result < 0)
  {
    // Deallocate task_struct
    new_task->PID = -1;
    new_task->TID = -1;
    list_add_tail(&(new_task->list), &freequeue);
    return -EFAULT;
  }

  /* Inherit user data */

  // Get the page table for new_task (same for current task)
  page_table_entry *new_page_table = get_PT(new_task);

  // Allocate new frame for new user stack
  int new_stack_frame = alloc_frame();
  if (new_stack_frame < 0)
  {
    // Deallocate task_struct
    new_task->PID = -1;
    new_task->TID = -1;
    list_add_tail(&(new_task->list), &freequeue);
    return -ENOMEM;
  }

  set_ss_pag(new_page_table, THREAD_USER_STACK_PAGE(new_task->TID), new_stack_frame);

  /* Initialize task_struct structures */
  new_task->state = ST_READY;
  init_stats(&(new_task->p_stats));

  new_task->joined = NULL;
  new_task->errno = 0;
  new_task->retval = 0;
  init_tls(new_task->TLS);

  list_add_tail(&(new_task->list_threads), new_task->threads_process);

  /* Prepare new_task user stack */

  unsigned long *new_user_stack = (unsigned long *)(THREAD_USER_STACK_PAGE(new_task->TID) << 12);

  new_user_stack[USER_STACK_SIZE - 3] = 0;                            // fake @ret (the new thread will never need this because it will terminate before)
  new_user_stack[USER_STACK_SIZE - 2] = (unsigned long)start_routine; // void *(*start_routine)()
  new_user_stack[USER_STACK_SIZE - 1] = (unsigned long)arg;           // void *arg

  /* Prepare new_task context for task_switch */

  // Get the position of the current EBP in the new_task system stack
  int ebp_index = KERNEL_STACK_SIZE - 18; // 5 HW CTX | 11 SW CTX | 1 @ret_syscalls_handler | 1 previous EBP

  // Get the position of the current HW EIP in the new_task system stack
  int hw_eip_index = KERNEL_STACK_SIZE - 5; // 5 HW CTX | 11 SW CTX | 1 @ret_syscalls_handler | 1 previous EBP

  // Get the position of the current HW ESP in the new_task system stack
  int hw_esp_index = KERNEL_STACK_SIZE - 2; // 5 HW CTX | 11 SW CTX | 1 @ret_syscalls_handler | 1 previous EBP

  // Inject @wrap_routine in the new_task HW EIP
  new_task_union->stack[hw_eip_index] = (unsigned long)wrap_routine;

  // Inject new_user_stack @top in the new_task HW ESP
  new_task_union->stack[hw_esp_index] = (unsigned long)&(new_user_stack[USER_STACK_SIZE - 3]); // void *arg | void *(*start_routine)() | fake @ret

  // Point new_task register_esp to the current EBP
  new_task->register_esp = (int)&(new_task_union->stack[ebp_index]);

  // Enqueue new process to readyqueue
  list_add_tail(&(new_task->list), &readyqueue);

  return 0;
}

void sys_pthread_exit(int retval)
{
  // If the master thread calls pthread_exit terminate the current process
  if (current()->TID == 0)
    sys_exit();

  // If a thread joined me, unblock it
  struct task_struct *joined;
  if ((joined = current()->joined) != NULL)
  {
    joined->state = ST_READY;
    list_del(&(joined->list));
    list_add_tail(&(joined->list), &readyqueue);
  }

  struct list_head *pos;
  int threads_in_the_process_counter = 0;
  list_for_each(pos, current()->threads_process)
  {
    struct task_struct *t_aux = list_head_to_task_struct(pos);
    if (t_aux->state != ST_ZOMBIE)
      threads_in_the_process_counter++;
  }

  current()->retval = retval;
  current()->state = ST_ZOMBIE;

  /* Manage mutexes where the current thread is implicated */

  for (int i = 0; i < MAX_MUTEXES; i++)
  {
    if (!mutexes[i].initialized)
      continue;

    // If im the owner of the mutex pass ownership
    if (mutexes[i].pid_owner == current()->PID && mutexes[i].tid_owner == current()->TID)
    {
      mutexes[i].pid_owner = -1;
      mutexes[i].tid_owner = -1;

      if (!list_empty(&(mutexes[i].blockedqueue)))
      {
        struct list_head *tmp = list_first(&(mutexes[i].blockedqueue));
        struct task_struct *next_owner = list_head_to_task_struct(tmp);
        list_del(&(next_owner->list));

        mutexes[i].pid_owner = next_owner->PID;
        mutexes[i].tid_owner = next_owner->TID;
        next_owner->state = ST_READY;
        list_add_tail(&(next_owner->list), &readyqueue);
      }
    }
  }

  if (threads_in_the_process_counter == 1)
  {
    panic("sys_pthread_exit executed with only the calling thread ready and it's not the master thread");
    sys_exit();
  }
  else if (threads_in_the_process_counter > 1)
  {
    switch (sched_next_decide_level())
    {
    case 2:
      sched_next_rr_level2();
      break;
    case 1:
      sched_next_rr_level1();
      break;
    default:
      // Switch to idle_task as the readyqueue is empty
      panic("sys_pthread_exit executed letting all the system blocked");
      sched_next_rr(idle_task);
      break;
    }
  }
  else
  {
    panic("sys_pthread_exit executed without ready threads and the calling thread is not the master thread");
  }
}

int sys_pthread_join(int TID, int *retval)
{
  // Joining myself would produce a deadlock
  if (TID == current()->TID)
    return -EDEADLK;

  // Check retval if wanted
  if (retval != NULL && !access_ok(VERIFY_WRITE, retval, sizeof(int)))
    return -EFAULT;

  struct list_head *pos;
  struct task_struct *t_thread_to_join_with = NULL;
  list_for_each(pos, current()->threads_process)
  {
    struct task_struct *t_aux = list_head_to_task_struct(pos);
    if (TID == t_aux->TID)
    {
      t_thread_to_join_with = t_aux;
      break;
    }
  }

  // There isn't any thread with that TID in the process
  if (t_thread_to_join_with == NULL)
    return -ESRCH;

  // Trying to join the same thread that joined me would produce a deadlock
  if (current()->joined == t_thread_to_join_with)
    return -EDEADLK;

  // Cannot join a tread which is already joined by someone else
  if (t_thread_to_join_with->joined != NULL)
    return -EINVAL;

  // If the thread i'm trying to join with hasn't finished its execution yet, block until it finishes its execution
  if (t_thread_to_join_with->state != ST_ZOMBIE)
  {
    t_thread_to_join_with->joined = current(); // That thread is officially joined by me
    current()->state = ST_BLOCKED;
    list_add_tail(&(current()->list), &blockedqueue);

    switch (sched_next_decide_level())
    {
    case 2:
      sched_next_rr_level2();
      break;
    case 1:
      sched_next_rr_level1();
      break;
    default:
      // Switch to idle_task as the readyqueue is empty
      panic("sys_pthread_join executed letting all the system blocked");
      sched_next_rr(idle_task);
      break;
    }
  }

  // This point will be reached when the thread_to_join_with finish its execution (exits).

  if (retval != NULL)
    *retval = t_thread_to_join_with->retval;

  /* Free resources of thread_to_join_with as it terminated */

  int user_stack_VPN = THREAD_USER_STACK_PAGE(TID);
  page_table_entry *process_PT = get_PT(t_thread_to_join_with);
  free_frame(get_frame(process_PT, user_stack_VPN));
  del_ss_pag(process_PT, user_stack_VPN);

  t_thread_to_join_with->PID = -1; // Not needed. Just for consistency with other functions
  t_thread_to_join_with->TID = -1; // Not needed. Just for consistency with other functions
  list_del(&(t_thread_to_join_with->list_threads));
  list_add_tail(&(t_thread_to_join_with->list), &freequeue);

  return 0;
}

int sys_mutex_init()
{
  for (int i = 0; i < MAX_MUTEXES; i++)
  {
    if (!mutexes[i].initialized)
    {
      mutexes[i].initialized = true;
      INIT_LIST_HEAD(&(mutexes[i].blockedqueue));
      mutexes[i].pid_initializer = current()->PID;
      return i; // Returns the mutex identifier that has been initialized.
    }
  }

  // There isn't any mutex left, all have been already initialized
  return -EAGAIN;
}

int sys_mutex_destroy(int mutex_id)
{
  // The identifier is invalid or the mutex is not initialized
  if (mutex_id < 0 || mutex_id >= MAX_MUTEXES || !mutexes[mutex_id].initialized)
    return -EINVAL;

  // Cannot destroy a mutex that is being used (locked) at this moment
  if (mutexes[mutex_id].pid_owner > 0 && mutexes[mutex_id].tid_owner >= 0)
    return -EBUSY;

  // Note that it's not needed to check if there is any blocked thread in the mutex because if the
  // mutex is not being used by anyone implies that there isn't any thread in the mutex queue.

  // The mutex can only be destroyed by the initializer process
  if (mutexes[mutex_id].pid_initializer != current()->PID)
    return -EPERM;

  mutexes[mutex_id].pid_owner = -1;
  mutexes[mutex_id].tid_owner = -1;
  DESTROY_LIST_HEAD(&(mutexes[mutex_id].blockedqueue));
  mutexes[mutex_id].initialized = false;
  mutexes[mutex_id].pid_initializer = -1;

  return 0;
}

int sys_mutex_lock(int mutex_id)
{
  // The identifier is invalid or the mutex is not initialized
  if (mutex_id < 0 || mutex_id >= MAX_MUTEXES || !mutexes[mutex_id].initialized)
    return -EINVAL;

  if (mutexes[mutex_id].pid_owner > 0 && mutexes[mutex_id].tid_owner >= 0)
  {
    // Locking a mutex already owned by me would produce a deadlock
    if (mutexes[mutex_id].pid_owner == current()->PID && mutexes[mutex_id].tid_owner == current()->TID)
      return -EDEADLK;

    // The mutex has already an owner, so block until I get the ownership
    current()->state = ST_BLOCKED;
    list_add_tail(&(current()->list), &(mutexes[mutex_id].blockedqueue));

    switch (sched_next_decide_level())
    {
    case 2:
      sched_next_rr_level2();
      break;
    case 1:
      sched_next_rr_level1();
      break;
    default:
      // Switch to idle_task as the readyqueue is empty
      panic("sys_mutex_lock executed letting all the system blocked");
      sched_next_rr(idle_task);
      break;
    }
  }

  mutexes[mutex_id].pid_owner = current()->PID;
  mutexes[mutex_id].tid_owner = current()->TID;

  return 0;
}

int sys_mutex_unlock(int mutex_id)
{
  // The identifier is invalid or the mutex is not initialized
  if (mutex_id < 0 || mutex_id >= MAX_MUTEXES || !mutexes[mutex_id].initialized)
    return -EINVAL;

  // Cannot unlock a mutex not owned by me
  if (mutexes[mutex_id].pid_owner != current()->PID && mutexes[mutex_id].tid_owner != current()->TID)
    return -EPERM;

  mutexes[mutex_id].pid_owner = -1;
  mutexes[mutex_id].tid_owner = -1;

  // Pass ownership
  if (!list_empty(&(mutexes[mutex_id].blockedqueue)))
  {
    struct list_head *tmp = list_first(&(mutexes[mutex_id].blockedqueue));
    struct task_struct *next_owner = list_head_to_task_struct(tmp);
    list_del(&(next_owner->list)); // Delete next_owner from the mutex's blockedqueue

    mutexes[mutex_id].pid_owner = next_owner->PID;
    mutexes[mutex_id].tid_owner = next_owner->TID;
    next_owner->state = ST_READY;
    list_add_tail(&(next_owner->list), &readyqueue);
  }

  return 0;
}

int sys_pthread_key_create()
{
  // Initialize a TLS entry for the calling thread
  for (int i = 0; i < TLS_SIZE; i++)
  {
    if (!(current()->TLS[i].used))
    {
      current()->TLS[i].used = true;
      return i; // Returns the initialized entry key
    }
  }

  // There isn't any free entry in calling thread's TLS
  return -EAGAIN;
}

int sys_pthread_key_delete(int key)
{
  // Cannot delete a TLS entry with an invalid key or if it is already uninitialized
  if (key < 0 || key >= TLS_SIZE || !current()->TLS[key].used)
    return -EINVAL;

  // Uninitialize the entry indicated by key
  current()->TLS[key].value = NULL;
  current()->TLS[key].used = false;

  return 0;
}

void *sys_pthread_getspecific(int key)
{
  // Check invalid key or if it is not initialized
  if (key < 0 || key >= TLS_SIZE || !current()->TLS[key].used)
    return (void *)(-EINVAL);

  return current()->TLS[key].value;
}

int sys_pthread_setspecific(int key, void *value)
{
  // Check invalid key or if it is not initialized
  if (key < 0 || key >= TLS_SIZE || !current()->TLS[key].used)
    return -EINVAL;

  // Store value into the TLS entry indicated by key
  current()->TLS[key].value = value;

  return 0;
}
