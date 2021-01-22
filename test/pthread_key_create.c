#include <libc.h>

/*EXPLICACIÓN TEST:
Nuestra TLS es de 64 posiciones, así que se intentará hacer key_create 100 veces. Si alguna de las primeras 64 veces que se intenta hacer
key_create NO tiene éxito, el test no se superará. Si una vez la TLS ya está llena (a partir de la iteración 64) se intenta hacer un key_create
que NO acaba con error EAGAIN, el test no se superará. 
*/
int pthread_key_create_success_EAGAIN(void)
{
  int ret = -1;

  for (int i = 0; i < 100; i++)
  {
    ret = pthread_key_create();

    if (i < 64) // TLS size is 64
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

  // Clear TLS in order to execute other tests correctly
  for (int i = 0; i < 64; i++)
    pthread_key_delete(i);

  return true;
}
