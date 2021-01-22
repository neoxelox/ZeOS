#include <libc.h>

/*
What:
    A new process is created. Both, the current and the new process, print a different
    message to the screen with the return value of the fork syscall, but the child
    process exits before printing it's message.

Expected:
    The current process (parent) prints "Padre: <ret>" being ret the child's PID.
    The child process does not print anything.
*/
int exit_success(void)
{
    int ret = -1;

    if ((ret = fork()))
    {
        println("Padre: ");
        printvar(ret);
    }
    else
    {
        exit();
        println("Hijo: ");
        printvar(ret);
    }

    // Let enough time so that the new process starts execution and exits
    delay(50);

    return (ret > 0);
}

void *call_exit(void *arg)
{
    int ret = false;

    exit();

    ret = 21;

    return (void *)ret;
}

/*
What:
    A new thread is created and performs the exit syscall with exit value 0. Then returns 21.

Expected:
    The return value of the created thread is 21 as only the masterthread of a process can
    execute the exit syscall. (The return statement will implicitly perform a pthread_exit)
*/
int exit_calling_thread_is_not_master(void)
{
    int ret = -1;
    int TID, retval;

    ret = pthread_create(&TID, &call_exit, NULL);
    if (ret < 0 || TID <= 0)
        return false;

    ret = pthread_join(TID, &retval);
    if (ret < 0 || retval != 21)
        return false;

    return true;
}