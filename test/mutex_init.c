#include <libc.h>

/*
What:
    The current thread initializes lots of mutexes while checking if the return
    value is an error or not.

Expected:
    Only the first 20 mutexes are initialized and do not give an error. The rest give
    an error and the errno variable is set to EAGAIN because all the mutex_t are being
    occupied and the system cannot initialize more mutexes.

    Notice that our system supports 20 simultaneous mutexes.
*/
int mutex_init_success_EAGAIN(void)
{
    int ret = -1;

    for (int i = 0; i < 25; i++)
    {
        ret = mutex_init();

        if (i < 20) // The maximum number of mutexes in the system is 20
        {
            if (ret < 0)
                return false;
        }
        else
        {
            if (ret >= 0 || errno != EAGAIN)
                return false;
        }
    }

    // Destroy mutexes initialized in order to execute other tests correctly
    for (int i = 0; i < 20; i++)
        mutex_destroy(i);

    return true;
}