#include <libc.h>

/*
What:
    The current thread tries to perform the mutex_unlock syscall with wrong params,
    one param wrong at a time.

Expected:
    No mutex is unlocked when passing invalid parameters to the mutex_unlock syscall.
    The errno variable is set to EINVAL as we passed invalid parameters.

    Notice that our system supports 20 mutexes and we are passing smaller or greater
    mutex ids from the range 0-19. Also a mutex cannot be unlocked if it was not
    initialized, was already destroyed before or was already unlocked.
*/
int mutex_unlock_EINVAL(void)
{
    int ret = -1;
    int mutex_id;

    mutex_id = mutex_init();
    if (mutex_id < 0)
        return false;

    ret = mutex_destroy(mutex_id);
    if (ret < 0)
        return false;

    ret = mutex_unlock(mutex_id);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_unlock(11);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_unlock(-31);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_unlock(31);
    if (ret >= 0 || errno != EINVAL)
        return false;

    // Unlock and destroy initialized mutex in order to execute other tests correctly
    mutex_unlock(mutex_id);
    mutex_destroy(mutex_id);

    return true;
}

void *call_mutex_lock(void *arg)
{
    mutex_lock((int)arg);

    delay(100);

    // Unlock and destroy initialized mutex in order to execute other tests correctly
    mutex_unlock((int)arg);
    mutex_destroy((int)arg);

    return (void *)0;
}

/*
What:
    The current thread initializes a new mutex and then creates a new thread, which
    will lock the new mutex. Then, the current thread tries to unlock the new mutex.

Expected:
    The mutex is not unlocked and the mutex_unlock syscall returns an error setting
    the errno variable to EPERM, as a mutex can only be unlocked by the thread that
    locked it.
*/
int mutex_unlock_EPERM(void)
{
    int ret = -1;
    int TID, mutex_id;

    mutex_id = mutex_init();
    if (mutex_id < 0)
        return false;

    ret = pthread_create(&TID, &call_mutex_lock, (void *)mutex_init);
    if (ret < 0 || TID <= 0)
        return false;

    // Let enough time so that the other thread start execution and locks the mutex
    delay(100);

    ret = mutex_unlock(mutex_id);
    if (ret >= 0 || errno != EPERM)
        return false;

    // Free the created thread in order to execute other tests correctly
    pthread_join(TID, NULL);

    return true;
}