/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */

#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>
#include <errno.h>

#define true 1
#define false 0
#define NULL 0

extern int errno;

void print(char *s);
void println(char *s);
void printvar(int var);

#define breakpoint           \
    println("[BREAKPOINT]"); \
    while (1)                \
    {                        \
    }

void delay(unsigned long ticks);

int write(int fd, char *buffer, int size);

void itoa(int a, char *b);

int strlen(char *a);

void perror();

int getpid();

int gettime();

int fork();

void exit();

int yield();

int get_stats(int pid, struct stats *st);

int pthread_create(int *TID, void *(*start_routine)(), void *arg);

void pthread_exit(int retval);

int pthread_join(int TID, int *retval);

int mutex_init();

int mutex_destroy(int mutex_id);

int mutex_lock(int mutex_id);

int mutex_unlock(int mutex_id);

int pthread_key_create();

int pthread_key_delete(int key);

void *pthread_getspecific(int key);

int pthread_setspecific(int key, void *value);

#endif /* __LIBC_H__ */
