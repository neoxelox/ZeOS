/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <types.h>
#include <hardware.h>
#include <segment.h>
#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>
#include <p_stats.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS + 2]
    __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;

// Blocked queue
struct list_head blockedqueue;
// Free task structs
struct list_head freequeue;
// Ready queue
struct list_head readyqueue;

// Sentinels vector of thread queues of the same process
// I.E.: Each element will be a list head of the threads queue of a certain process
struct list_head threads_processes[NR_TASKS];

void init_stats(struct stats *s)
{
  s->user_ticks = 0;
  s->system_ticks = 0;
  s->blocked_ticks = 0;
  s->ready_ticks = 0;
  s->elapsed_total_ticks = get_ticks();
  s->total_trans = 0;
  s->remaining_ticks = get_ticks();
}

void init_tls(struct tls_t *TLS)
{
  for (int i = 0; i < TLS_SIZE; i++)
  {
    TLS[i].value = NULL;
    TLS[i].used = 0;
  }
}

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry *get_DIR(struct task_struct *t)
{
  return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry *get_PT(struct task_struct *t)
{
  return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr)) << 12);
}

int allocate_DIR(struct task_struct *t)
{
  int pos;

  pos = ((int)t - (int)task) / sizeof(union task_union);

  t->dir_pages_baseAddr = (page_table_entry *)&dir_pages[pos];

  return 1;
}

void cpu_idle(void)
{
  __asm__ __volatile__("sti"
                       :
                       :
                       : "memory");

  printk_color("\nExecuting Idle Task...", 0x03);

  while (1)
  {
  }
}

int remaining_quantum_process = 0;
int remaining_quantum_thread = 0;

int get_quantum_process(struct task_struct *t)
{
  return t->total_quantum;
}

int get_quantum_thread(struct task_struct *t)
{
  return t->quantum_thread;
}

void set_quantum_process(struct task_struct *t, int new_quantum)
{
  t->total_quantum = new_quantum;
}

void set_quantum_thread(struct task_struct *t, int new_quantum)
{
  t->quantum_thread = new_quantum;
}

struct task_struct *idle_task = NULL;

void update_sched_data_rr(void)
{
  if (--remaining_quantum_process < 0)
    remaining_quantum_process = 0; // Prevent underflow
  if (--remaining_quantum_thread < 0)
    remaining_quantum_thread = 0; // Prevent underflow
}

// Returns
// 0 : Switch not needed
// 1 : Needs switch 1st level (thread same process)
// 2 : Needs switch 2nd level (thread different process)
int needs_sched_rr(void)
{
  if (list_empty(&readyqueue))
    return 0;

  // There is a thread in the readyqueue

  // If executing idle_task, switch instantly to the other ready process
  if (current() == idle_task)
    return 2;

  // The current process didn't finish its time in cpu
  if (remaining_quantum_process > 0 && remaining_quantum_thread > 0)
    return 0;

  // Try to find a switcheable thread as we don't have process quantum
  // or thread quantum left and readyqueue is not empty

  struct task_struct *thread_same_process = NULL;      // Ready thread of the same process candidate
  struct task_struct *thread_different_process = NULL; // Ready thread of a different process candidate

  struct list_head *pos;
  list_for_each(pos, &readyqueue)
  {
    struct task_struct *tmp = list_head_to_task_struct(pos);
    if (thread_same_process == NULL && tmp->PID == current()->PID)
      thread_same_process = tmp;
    else if (thread_different_process == NULL && tmp->PID != current()->PID)
      thread_different_process = tmp;

    if (thread_same_process != NULL && thread_different_process != NULL)
      break;
  }

  // The current thread finished its quantum but not the current process
  if (remaining_quantum_thread <= 0 && remaining_quantum_process > 0)
  {
    if (thread_same_process != NULL)
      return 1; // Switch to the ready thread_same_process
    return 0;   // As there isn't any ready thread_same_process
  }             // continue process quantum with the current thread

  // The current process has no quantum left

  // If there is a ready thread_different_process switch to it
  if (thread_different_process != NULL)
    return 2;

  // As there isn't any ready thread_different_process
  // switch to thread_same_process if current thread quantum is done
  if (remaining_quantum_thread <= 0)
    return 1;
  return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
  if (t->state != ST_RUN)
    list_del(&(t->list));
  if (dst_queue != NULL)
  {
    list_add_tail(&(t->list), dst_queue);
    if (dst_queue != &readyqueue)
      t->state = ST_BLOCKED;
    else
    {
      update_stats(&(t->p_stats.system_ticks), &(t->p_stats.elapsed_total_ticks));
      t->state = ST_READY;
    }
  }
  else
    t->state = ST_RUN;
}

// Puts new in RUN
void sched_next_rr(struct task_struct *new)
{
  new->state = ST_RUN;
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
  update_stats(&(new->p_stats.ready_ticks), &(new->p_stats.elapsed_total_ticks));
  new->p_stats.total_trans++;

  task_switch((union task_union *)new);
}

// Scheduler 1st level (thread same process)
void sched_next_rr_level1(void)
{
  struct task_struct *thread_same_process = NULL; // Ready thread of the same process

  struct list_head *pos;
  list_for_each(pos, &readyqueue)
  {
    struct task_struct *tmp = list_head_to_task_struct(pos);
    if (tmp->PID == current()->PID)
    {
      thread_same_process = tmp;
      break;
    }
  }

  if (thread_same_process == NULL)
    panic("sched_next_rr_level1 executed with only ready threads of a different process");

  list_del(&(thread_same_process->list));

  remaining_quantum_thread = get_quantum_thread(thread_same_process);
  sched_next_rr(thread_same_process);
}

// Scheduler 2nd level (thread different process)
void sched_next_rr_level2(void)
{
  struct task_struct *thread_different_process = NULL; // Ready thread of a different process

  struct list_head *pos;
  list_for_each(pos, &readyqueue)
  {
    struct task_struct *tmp = list_head_to_task_struct(pos);
    if (tmp->PID != current()->PID)
    {
      thread_different_process = tmp;
      break;
    }
  }

  if (thread_different_process == NULL)
    panic("sched_next_rr_level2 executed with only ready threads of the same process");

  list_del(&(thread_different_process->list));

  remaining_quantum_process = get_quantum_process(thread_different_process);
  remaining_quantum_thread = get_quantum_thread(thread_different_process);
  sched_next_rr(thread_different_process);
}

// Returns
// 0 : Switch to idle_task
// 1 : Needs switch 1st level (thread same process)
// 2 : Needs switch 2nd level (thread different process)
int sched_next_decide_level(void)
{
  if (list_empty(&readyqueue))
    return 0;

  // There is a thread in the readyqueue

  // Try to find a switcheable thread of the same process
  // as we know both quantums still have ticks left

  struct task_struct *thread_same_process = NULL; // Ready thread of the same process candidate

  struct list_head *pos;
  list_for_each(pos, &readyqueue)
  {
    struct task_struct *tmp = list_head_to_task_struct(pos);
    if (tmp->PID == current()->PID)
    {
      thread_same_process = tmp;
      break;
    }
  }

  // Switch to the ready thread_same_process
  if (thread_same_process != NULL)
    return 1;

  // As there isn't any ready thread_same_process
  // switch to thread_different_process
  return 2;
}

void schedule()
{
  update_sched_data_rr();

  int level;

  // Switch if needed (not 0)
  if ((level = needs_sched_rr()))
  {
    update_process_state_rr(current(), &readyqueue);

    if (level == 1)
      sched_next_rr_level1();
    else if (level == 2)
      sched_next_rr_level2();
  }
}

void init_idle(void)
{
  struct list_head *l = list_first(&freequeue);
  list_del(l);
  struct task_struct *c = list_head_to_task_struct(l);
  union task_union *uc = (union task_union *)c;

  c->PID = 0;
  c->TID = 0;

  c->total_quantum = DEFAULT_QUANTUM_PROCESS;
  c->quantum_thread = DEFAULT_QUANTUM_THREAD;

  init_stats(&c->p_stats);

  allocate_DIR(c);

  uc->stack[KERNEL_STACK_SIZE - 1] = (unsigned long)&cpu_idle; /* Return address */
  uc->stack[KERNEL_STACK_SIZE - 2] = 0;                        /* register ebp */

  c->register_esp = (int)&(uc->stack[KERNEL_STACK_SIZE - 2]); /* top of the stack */

  idle_task = c;
}

void setMSR(unsigned long msr_number, unsigned long high, unsigned long low);

void init_task1(void)
{
  struct list_head *l = list_first(&freequeue);
  list_del(l);
  struct task_struct *c = list_head_to_task_struct(l);
  union task_union *uc = (union task_union *)c;

  c->PID = 1;
  c->TID = 0;

  c->total_quantum = DEFAULT_QUANTUM_PROCESS;
  c->quantum_thread = DEFAULT_QUANTUM_THREAD;

  c->state = ST_RUN;

  c->joined = NULL;
  c->errno = 0;
  c->retval = 0;
  init_tls(c->TLS);

  // Get a free threads_process list
  for (int i = 0; i < NR_TASKS; i++)
  {
    if (list_uninitialized(&(threads_processes[i])))
    {
      c->threads_process = &(threads_processes[i]);
      break;
    }
  }

  INIT_LIST_HEAD(c->threads_process);
  list_add_tail(&(c->list_threads), c->threads_process);

  init_stats(&c->p_stats);

  remaining_quantum_process = c->total_quantum;
  remaining_quantum_thread = c->quantum_thread;

  allocate_DIR(c);

  set_user_pages(c);

  tss.esp0 = (DWord) & (uc->stack[KERNEL_STACK_SIZE]);
  setMSR(0x175, 0, (unsigned long)&(uc->stack[KERNEL_STACK_SIZE]));

  set_cr3(c->dir_pages_baseAddr);
}

void init_freequeue()
{
  int i;

  INIT_LIST_HEAD(&freequeue);

  /* Insert all task structs in the freequeue */
  for (i = 0; i < NR_TASKS; i++)
  {
    task[i].task.PID = -1;
    list_add_tail(&(task[i].task.list), &freequeue);
  }
}

void init_sched()
{
  init_freequeue();
  INIT_LIST_HEAD(&readyqueue);
  INIT_LIST_HEAD(&blockedqueue);
}

struct task_struct *current()
{
  int ret_value;

  return (struct task_struct *)(((unsigned int)&ret_value) & 0xfffff000);
}

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return (struct task_struct *)((int)l & 0xfffff000);
}

/* Do the magic of a task switch */
void inner_task_switch(union task_union *new)
{
  page_table_entry *new_DIR = get_DIR(&new->task);

  /* Update TSS and MSR to make it point to the new stack */
  tss.esp0 = (int)&(new->stack[KERNEL_STACK_SIZE]);
  setMSR(0x175, 0, (unsigned long)&(new->stack[KERNEL_STACK_SIZE]));

  // Switch current's errno with new's errno
  // Protect against dereferencing perrno as:
  //  - Idle_task, because is a system process and doesn't have the user address space where errno is.
  //  - A process that just exited.
  if (current() != idle_task && current()->PID != -1)
  {
    current()->errno = *perrno;
  }

  /* TLB flush. New address space */
  set_cr3(new_DIR);

  // Switch current's errno with new's errno
  // Protect against dereferencing perrno as idle_task, because is asystem
  // process and doesn't have the user address space where errno is.
  if (&(new->task) != idle_task)
  {
    *perrno = new->task.errno;
  }

  switch_stack(&current()->register_esp, new->task.register_esp);
}

/* Force a task switch assuming that the scheduler does not work with priorities */
void force_task_switch()
{
  update_process_state_rr(current(), &readyqueue);

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
    // It should never reach here because of above's update_process_state_rr to readyqueue
    panic("force_task_switch switched to idle_task with a process in the readyqueue");
    sched_next_rr(idle_task);
    break;
  }
}
