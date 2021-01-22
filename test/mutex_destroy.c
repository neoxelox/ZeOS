#include <libc.h>

/*
What:
    The current thread initializes and then destroys lots of mutexes.

Expected:
    There are no errors as we are destroying the initialized mutex right after
    it is initialized.
*/
int mutex_destroy_success(void)
{
    int ret = -1;
    int mutex_id;

    for (int i = 0; i < 1000; i++)
    {
        mutex_id = mutex_init();
        if (mutex_id < 0)
            return false;

        ret = mutex_destroy(mutex_id);
        if (ret < 0)
            return false;
    }

    return true;
}

/*
What:
    The current thread tries to perform the mutex_destroy syscall with wrong params,
    one param wrong at a time.

Expected:
    No mutex is destroyed when passing invalid parameters to the mutex_destroy syscall.
    The errno variable is set to EINVAL as we passed invalid parameters.

    Notice that our system supports 20 mutexes and we are passing smaller or greater
    mutex ids from the range 0-19. Also a mutex cannot be destroyed if it was not
    initialized or was already destroyed before.
*/
int mutex_destroy_EINVAL(void)
{
    int ret = -1;
    int mutex_id;

    mutex_id = mutex_init();
    if (mutex_id < 0)
        return false;

    ret = mutex_destroy(mutex_id);
    if (ret < 0)
        return false;

    ret = mutex_destroy(mutex_id);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_destroy(11);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_destroy(-31);
    if (ret >= 0 || errno != EINVAL)
        return false;

    ret = mutex_destroy(31);
    if (ret >= 0 || errno != EINVAL)
        return false;

    return true;
}

/*
What:
    The current thread tries to destroy a mutex that is being used at the moment.

Expected:
    The mutex is not destroyed and the errno variable is set to EBUSY as mutexes
    cannot be destroyed if they are being used at the moment (locked).
*/
int mutex_destroy_EBUSY(void)
{
    int ret = -1;
    int mutex_id;

    mutex_id = mutex_init();
    if (mutex_id < 0)
        return false;

    ret = mutex_lock(mutex_id);
    if (ret < 0)
        return false;

    ret = mutex_destroy(mutex_id);
    if (ret >= 0 || errno != EBUSY)
        return false;

    // Unlock and destroy initialized mutex in order to execute other tests correctly
    mutex_unlock(mutex_id);

    ret = mutex_destroy(mutex_id);
    if (ret < 0)
        return false;

    return true;
}

/*
What:
    A new process is created which creates a new mutex and the parent process tries to
    destroy it.

Expected:
    The mutex is not destroyed and the errno variable is set to EPERM as mutexes
    cannot be destroyed if you are not the process which initialized it.

    Notice that we hardcoded the mutex_id in the mutex_destroy syscall on the parent
    process in order to make the test simpler.
*/
int mutex_destroy_EPERM(void)
{
    int ret = -1;

    ret = fork();
    if (ret < 0)
        return false;

    if (ret == 0)
    {
        int mutex_id = mutex_init();
        delay(500); // Let enough time so that the parent process restarts execution and tries to destroy the new mutex

        // Destroy mutex initialized in order to execute other tests correctly
        mutex_destroy(mutex_id);
        exit();
    }
    else
    {
        delay(250); // Let enough time so that the new process starts execution and initializes a new mutex
        ret = mutex_destroy(0);
        if (ret >= 0 || errno != EPERM)
            return false;
    }

    return true;
}