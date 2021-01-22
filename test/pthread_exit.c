#include <libc.h>

void *call_pthread_exit(void *arg)
{
	int ret = 31;

	pthread_exit(ret);

	return false;
}

void *call_pthread_create(void *arg)
{
	int ret = -1;
	int TID, retval;

	ret = pthread_create(&TID, &call_pthread_exit, NULL);
	if (ret < 0 || TID <= 0)
		return false;

	ret = pthread_join(TID, &retval);
	if (ret < 0 || retval != 31)
		return false;

	ret = 31;

	return (void *)ret;
}

/*EXPLICACIÓN TEST:
Se va a crear un thread A que creará otro thread que tendrá que hacer exit con el valor 31. Este thread A, también tendrá que hacer exit
con el valor 31 pero esta vez de manera implícita ya que hará return 31 (lo que implicitamente deberia actuar como un pthread_exit).
Así que también comprobaremos si así ha ocurrido. Si ambos threads no finalizan con el valor 31 significa que no hemos pasado el test.
*/
int pthread_exit_success(void)
{
	int ret = -1;
	int TID, retval;

	ret = pthread_create(&TID, &call_pthread_create, NULL);
	if (ret < 0 || TID <= 0)
		return false;

	ret = pthread_join(TID, &retval);
	if (ret < 0 || retval != 31)
		return false;

	return true;
}