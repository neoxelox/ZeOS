#include <libc.h>

int shared_vector[180];
int shared_index;
int shared_mutex;

void *modify_shared_vector(void *arg)
{
    int ret = -1;

    while (shared_index < 12)
    {
        ret = mutex_lock(shared_mutex);
        if (ret < 0)
            return (void *)0;

        shared_vector[shared_index] = (int)arg;
        shared_index++;

        ret = mutex_unlock(shared_mutex);
        if (ret < 0)
            return (void *)0;
    }

    return (void *)0;
}

/*
What:
    The current thread creates two more threads. Each one has a unique value 31, 21 and 11,
    that must be written to the global shared vector by all of them in this order:
    31, 21, 11, 31, 21, 11, 31, 21, 11... and so on. The ordering constraint is enforced by
    a global shared mutex.

    Notice that all the threads also access the shared vector using a shared index, which is
    incremented by all of them in order.

Expected:
    No errors appear when using the mutex "shared_mutex", locking and unlocking it. The resulting
    vector "shared_vector" has the unique values of each thread in the order described before.
*/
int mutex_lock_unlock_success(void)
{
    int ret = -1;

    shared_mutex = mutex_init();
    if (shared_mutex < 0)
        return false;

    int TID, TIDS[2];

    ret = pthread_create(&TID, &modify_shared_vector, (void *)21);
    if (ret < 0)
        return false;

    TIDS[0] = TID;

    ret = pthread_create(&TID, &modify_shared_vector, (void *)11);
    if (ret < 0)
        return false;

    TIDS[1] = TID;

    int wait_for_threads = true;

    while (shared_index < 12)
    {
        ret = mutex_lock(shared_mutex);
        if (ret < 0)
            return false;

        shared_vector[shared_index] = 31;
        shared_index++;

        // Let enough time at first in order that the other threads start execution and get blocked at the mutex
        if (wait_for_threads)
        {
            wait_for_threads = false;
            delay(100);
        }

        ret = mutex_unlock(shared_mutex);
        if (ret < 0)
            return false;
    }

    for (int i = 0; i < 12; i++)
    {
        switch (i % 3)
        {
        case 0:
            if (shared_vector[i] != 31)
                return false;
            break;
        case 1:
            if (shared_vector[i] != 21)
                return false;
            break;
        case 2:
            if (shared_vector[i] != 11)
                return false;
            break;
        default:
            break;
        }
    }

    // Free all the created threads and mutexes in order to execute other tests correctly
    pthread_join(TIDS[0], NULL);
    pthread_join(TIDS[1], NULL);
    mutex_unlock(shared_mutex);
    mutex_destroy(shared_mutex);

    return true;
}

/*
What:
    The current thread tries to perform the mutex_lock syscall with wrong params,
    one param wrong at a time.

Expected:
    No mutex is locked when passing invalid parameters to the mutex_lock syscall.
    The errno variable is set to EINVAL as we passed invalid parameters.

    Notice that our system supports 20 mutexes and we are passing smaller or greater
    mutex ids from the range 0-19. Also a mutex cannot be locked if it was not
    initialized or was already destroyed before.
*/
int mutex_lock_EINVAL(void)
{
    int ret = -1;
    int mutex_id;

    mutex_id = mutex_init();
    if (mutex_id < 0)
        return false;

    ret = mutex_destroy(mutex_id);
    if (ret < 0)
        return false;

    ret = mutex_lock(mutex_id);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_lock(11);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_lock(-31);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_lock(31);
    if (ret >= 0 || errno != EINVAL)
        return false;

    // Unlock and destroy initialized mutex in order to execute other tests correctly
    mutex_unlock(mutex_id);
    mutex_destroy(mutex_id);

    return true;
}

/*
What:
    The current thread tries to lock a mutex that has been locked before by itself.

Expected:
    The mutex_lock syscall returns error and sets the errno variable to EDEADLK as the
    mutex has been already locked by the current thread but not unlocked. A mutex cannot
    be locked by the same thread consecutively if the thread is the owner of the mutex,
    as that would result in a deadlock. (Because the only thread that could unblock other
    threads blocked at the mutex is also blocked)
*/
int mutex_lock_EDEADLK(void)
{
    int ret = -1;
    int mutex_id;

    mutex_id = mutex_init();
    if (mutex_id < 0)
        return false;

    ret = mutex_lock(mutex_id);
    if (ret < 0)
        return false;

    ret = mutex_lock(mutex_id);
    if (ret >= 0 || errno != EDEADLK)
        return false;

    // Unlock and destroy initialized mutex in order to execute other tests correctly
    mutex_unlock(mutex_id);
    mutex_destroy(mutex_id);

    return true;
}
