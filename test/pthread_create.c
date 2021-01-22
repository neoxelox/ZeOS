#include <libc.h>

void *return_arg(void *arg)
{
    return arg;
}

/*
What:
    Five threads are created sequentially with the number (31 + i) as their argument.
    The threads then return their argument.

Expected:
    There aren't any errors while creating the threads. The created threads must
    execute the given routine "return_arg" with (31 + i) as their only argument.
    Then, their return value must be the same as the argument we provided them earlier.
*/
int pthread_create_success(void)
{
    int ret = -1;
    int TID, retval;

    for (int i = 0; i < 5; i++)
    {
        ret = pthread_create(&TID, &return_arg, (void *)(31 + i));
        if (ret < 0 || TID <= 0)
            return false;

        ret = pthread_join(TID, &retval);
        if (ret < 0 || retval != (31 + i))
            return false;
    }

    return true;
}

/*
What:
    The current thread tries to perform the pthread_create syscall with wrong params,
    one param wrong at a time.

Expected:
    No thread is created when passing invalid parameters to the pthread_create syscall.
    The errno variable is set to EFAULT as we are passing NULLs or non-writable
    memory positions as parameters.

    Notice that passing "void *arg" as NULL is not invalid.
*/
int pthread_create_EFAULT(void)
{
    int ret = -1;
    int TID;

    ret = pthread_create(NULL, &return_arg, NULL);
    if (ret >= 0 || errno != EFAULT)
        return false;

    // Cannot pass the address of "return_arg" function as the parameter for "int *TID"
    // as it is a section of code and not a writable memory position.
    ret = pthread_create((int *)&return_arg, &return_arg, NULL);
    if (ret >= 0 || errno != EFAULT)
        return false;

    ret = pthread_create(&TID, NULL, NULL);
    if (ret >= 0 || errno != EFAULT)
        return false;

    return true;
}

/*
What:
    The current thread tries to create lots of threads while checking if the return
    value is an error or not.

Expected:
    Only the first 8 threads are created and do not give an error. The rest give an
    error and the errno variable is set to EAGAIN because all the task_structs are
    being occupied and the system cannot create more threads.

    Notice that our system supports 10 simultaneous task_structs but there are two
    that are already being used idle_task and task1.
*/
int pthread_create_EAGAIN(void)
{
    int ret = -1;
    int TID, TIDS[10];

    for (int i = 0; i < 10; i++)
    {
        ret = pthread_create(&TID, &return_arg, NULL);

        if (i < 8) // 2 task_structs are already being used, idle_task and task1
        {
            if (ret < 0 || TID <= 0)
                return false;

            TIDS[i] = TID;
        }
        else
        {
            if (ret >= 0 || errno != EAGAIN)
                return false;

            TIDS[i] = -1;
        }
    }

    // Free all the created threads in order to execute other tests correctly
    for (int i = 0; i < 10; i++)
    {
        if (TIDS[i] != -1)
        {
            pthread_join(TIDS[i], NULL);
        }
    }

    return true;
}