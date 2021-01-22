#include <libc.h>

/*EXPLICACIÓN TEST:
Se va a hacer 1000 veces la secuencia de key_create - key_delete. Si no acaba todo ello con éxito implicaría que no funciona correctamente la 
liberación de keys (es decir, que key_delete falla) así que el test no se superaría. Si se acaban las 1000 iteraciones sin problemas, el test
habrá sido un éxito. Notad que nuestra TLS es de 64 posiciones.
*/
int pthread_key_delete_success(void)
{
	int ret = -1;
	int key;

	for (int i = 0; i < 1000; i++)
	{
		key = pthread_key_create();
		if (key < 0)
			return false;

		ret = pthread_key_delete(key);
		if (ret < 0)
			return false;
	}

	return true;
}

/*EXPLICACIÓN TEST:
se va a intentar hacer key_delete sobre posiciones que no "estan activas/inicializadas", es decir, sobre posiciones que no se ha hecho un key_create previo.
También se va a intentar sobre posiciones que sí se ha hecho key_create pero luego tiene 2 key_deletes seguidos (el 1ero debería funcionar y el 2o fallar).
También se va a intentar sobre posiciones que se van más allá del rango de nuestra TLS (nuestra TLS acepta desde la posición 0 hasta la 63).
*/
int pthread_key_delete_EINVAL(void)
{
	int ret = -1;
	int key;

	ret = pthread_key_delete(32); // Como 32 es una posición que nunca se esta desinicializada debe dar error
	if (ret >= 0 || errno != EINVAL)
		return false;

	key = pthread_key_create();
	if (key < 0)
		return false;

	ret = pthread_key_delete(key);
	if (ret < 0)
		return false;

	ret = pthread_key_delete(key);
	if (ret >= 0 || errno != EINVAL)
		return false;

	ret = pthread_key_delete(-6);
	if (ret >= 0 || errno != EINVAL)
		return false;

	ret = pthread_key_delete(70);
	if (ret >= 0 || errno != EINVAL)
		return false;

	return true;
}
