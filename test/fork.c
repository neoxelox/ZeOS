#include <libc.h>

/*
What:
    A new process is created. Both, the current and the new process, print a different
    message to the screen with the return value of the fork syscall.

Expected:
    The current process (parent) prints "Padre: <ret>" being ret the child's PID.
    The child process prints "Hijo: 0".
*/
int fork_success(void)
{
    int ret = -1;

    if ((ret = fork()))
    {
        println("Padre: ");
        printvar(ret);
    }
    else
    {
        println("Hijo: ");
        printvar(ret);
        exit();
    }

    // Let enough time so that the new process starts execution and exits
    delay(50);

    return (ret > 0);
}

void *call_fork(void *arg)
{
    int ret = -1;

    ret = fork();
    if (ret != -1)
        return false;

    return (void *)errno;
}

/*
What:
    A new thread is created, tries to perform the fork syscall and returns it's errno variable.

Expected:
    The return value of the created thread, which is it's errno variable, is EPERM as only the
    masterthread of a process can execute the fork syscall.
*/
int fork_EPERM(void)
{
    int ret = -1;
    int TID, retval;

    ret = pthread_create(&TID, &call_fork, NULL);
    if (ret < 0 || TID <= 0)
        return false;

    ret = pthread_join(TID, &retval);
    if (ret < 0 || retval != EPERM)
        return false;

    return true;
}