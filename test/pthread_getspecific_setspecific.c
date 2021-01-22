#include <libc.h>

void *call_pthread_getspecific_setspecific(void *arg)
{
	int ret = -1;
	int key;

	key = pthread_key_create();
	if (key < 0)
		return (void *)0;

	ret = pthread_setspecific(key, (void *)21);
	if (ret < 0)
		return (void *)0;

	void *val = pthread_getspecific(key);
	if ((int)val != 21)
		return (void *)0;

	return (void *)val;
}

/* EXPLICACIÓN TEST:
Hacemos pthread_setspecific y get_specific en masterthread y también en un thread esclavo. Si no se coge el mismo dato que se ha escrito en la TLS
dentro de una cierta key, el test fallará. Se puede ver como el dato es distinto para el materthread y el thread esclavo porque la TLS es memoria
individual. Implícitamente también se estará testeando key_create y key_delete.
*/
int pthread_getspecific_setspecific_success(void)
{
	int ret = -1;
	int key;

	key = pthread_key_create();
	if (key < 0)
		return false;

	ret = pthread_setspecific(key, (void *)31);
	if (ret < 0)
		return false;

	void *val = pthread_getspecific(key);
	if ((int)val != 31)
		return false;

	int TID, retval;

	ret = pthread_create(&TID, &call_pthread_getspecific_setspecific, NULL);
	if (ret < 0 || TID <= 0)
		return false;

	ret = pthread_join(TID, &retval);
	if (ret < 0 || retval != 21)
		return false;

	// Delete used key in order to execute other tests correctly
	pthread_key_delete(key);

	return true;
}

/* EXPLICACIÓN TEST:
Se pone a prueba todas las posibles situaciones de EINVAL (acceso a una posición de la tls más pequeña o más grande que el tamaño de la tls y acceso a una
posición de la tls que no ha estado previamente "activada" con un key_create (sin que haya sido "desactivada" por un key_delete).
Si todo ello ha retornado -1 y el errno = EINVAL, se habrá superado el test
*/
int pthread_getspecific_setspecific_EINVAL(void)
{
	int ret = -1;

	ret = pthread_setspecific(-1, (void *)123);
	if (ret >= 0 || errno != EINVAL)
		return false;

	ret = pthread_setspecific(70, (void *)123);
	if (ret >= 0 || errno != EINVAL)
		return false;

	ret = (int)pthread_getspecific(-1);
	if (ret >= 0 || errno != EINVAL)
		return false;

	ret = (int)pthread_getspecific(70);
	if (ret >= 0 || errno != EINVAL)
		return false;

	int key = pthread_key_create();
	if (key < 0)
		return false;

	ret = pthread_key_delete(key);
	if (ret < 0)
		return false;

	ret = pthread_setspecific(key, (void *)123);
	if (ret >= 0 || errno != EINVAL)
		return false;

	ret = (int)pthread_getspecific(key);
	if (ret >= 0 || errno != EINVAL)
		return false;

	return true;
}
