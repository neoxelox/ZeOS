/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>

#define NR_TASKS 10
#define KERNEL_STACK_SIZE 1024
#define USER_STACK_SIZE (PAGE_SIZE / sizeof(int))

#define DEFAULT_QUANTUM_PROCESS 30
#define DEFAULT_QUANTUM_THREAD 10

#define MAX_MUTEXES 20

#define TLS_SIZE 64

#define perrno (int *)0x109000 // Address of errno (never changes)

enum state_t
{
  ST_RUN,
  ST_READY,
  ST_BLOCKED,
  ST_ZOMBIE
};

struct tls_t
{
  void *value;
  int used;
};

struct task_struct
{
  int PID; /* Process ID. This MUST be the first field of the struct. */
  page_table_entry *dir_pages_baseAddr;
  struct list_head list;                        /* Task struct enqueuing */
  int register_esp; /* position in the stack */ //Es el kernel_esp de mi ZeOS.
  enum state_t state;                           /* State of the process */
  int total_quantum;                            /* Total quantum of the process */
  struct stats p_stats;                         /* Process stats */

  int TID;
  int quantum_thread;
  struct task_struct *joined;
  int errno;
  struct tls_t TLS[TLS_SIZE];
  struct list_head *threads_process;
  struct list_head list_threads;
  int retval;
};

union task_union
{
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE]; /* pila de sistema, per procés */
};

struct mutex_t
{
  int pid_owner;
  int tid_owner;
  struct list_head blockedqueue;
  int initialized;
  int pid_initializer;
};

extern struct mutex_t mutexes[MAX_MUTEXES];

extern union task_union protected_tasks[NR_TASKS + 2];
extern union task_union *task; /* Vector de tasques */
extern struct task_struct *idle_task;

extern struct list_head threads_processes[NR_TASKS]; // Sentinels vector of thread queues of the same process

#define KERNEL_ESP(t) (DWord) & (t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP KERNEL_ESP(&task[1])

extern struct list_head blockedqueue;
extern struct list_head freequeue;
extern struct list_head readyqueue;

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

void schedule(void);

struct task_struct *current();

void task_switch(union task_union *t);
void switch_stack(int *save_sp, int new_sp);

void force_task_switch(void);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry *get_PT(struct task_struct *t);

page_table_entry *get_DIR(struct task_struct *t);

/* Headers for the scheduling policy */
void sched_next_rr(struct task_struct *new);
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();
void sched_next_rr_level1(void);
void sched_next_rr_level2(void);

int sched_next_decide_level(void);

void init_stats(struct stats *s);
void init_tls(struct tls_t *TLS);

#endif /* __SCHED_H__ */
