#ifndef __TEST_H__
#define __TEST_H__

/* Fork TestCase */
int fork_success(void);
int fork_EPERM(void);

/* Exit TestCase */
int exit_success(void);
int exit_calling_thread_is_not_master(void);

/* Pthread_create TestCase */
int pthread_create_success(void);
int pthread_create_EFAULT(void);
int pthread_create_EAGAIN(void);

/* Pthread_exit TestCase */
int pthread_exit_success(void);

/* Pthread_join TestCase */
int pthread_join_success(void);
int pthread_join_EDEADLK(void);
int pthread_join_EINVAL(void);
int pthread_join_ESRCH(void);
int pthread_join_EFAULT(void);

/* Mutex_init TestCase */
int mutex_init_success_EAGAIN(void);

/* Mutex_destroy TestCase */
int mutex_destroy_success(void);
int mutex_destroy_EINVAL(void);
int mutex_destroy_EBUSY(void);
int mutex_destroy_EPERM(void);

/* Mutex_lock and unlock TestCase */
int mutex_lock_unlock_success(void);
int mutex_lock_EINVAL(void);
int mutex_lock_EDEADLK(void);
int mutex_unlock_EINVAL(void);
int mutex_unlock_EPERM(void);

/* Pthread_key_create TestCase */
int pthread_key_create_success_EAGAIN(void);

/* Pthread_key_delete TestCase */
int pthread_key_delete_success(void);
int pthread_key_delete_EINVAL(void);

/* Pthread_getspecific and setspecific TestCase */
int pthread_getspecific_setspecific_success(void);
int pthread_getspecific_setspecific_EINVAL(void);

#endif /* __TEST_H__ */
