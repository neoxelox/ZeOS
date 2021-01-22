#include <libc.h>

void *hola(void *argumento)
{
	if ((int)argumento == 3)
	{
		while (1)
			; //NOTA QUE THREAD3 NO MORIRA
	}
	else
	{
		print("\nsoy el thread ");
		printvar((int)argumento);
	}
	pthread_exit((int)argumento);
	println("ESTO NUNCA DEBERÍA ESCRIBIRSE");

	return (void *)1;
}

/* EXPLICACION TEST:
Crearemos los threads A y B y nos joinearemos a continuación con B y A. Imprimiremos el retval de B y A y deberá ser '2' y '1' respectivamente.
A continuación crearemos C y éste nunca acabará (por while(1)), así que masterthread deberá quedarse indefinidamente bloqueado en el join, sin que se
llegue a imprimir ningún otro mensaje*/
int pthread_join_success(void)
{
	int thread1_TID, thread2_TID, thread3_TID;
	int thread1_retval, thread2_retval, thread3_retval;

	pthread_create(&thread1_TID, hola, (void *)1);
	pthread_create(&thread2_TID, hola, (void *)2);

	pthread_join(thread2_TID, &thread2_retval);
	pthread_join(thread1_TID, &thread1_retval);

	print("\nthread 2 ha acabado con retval: ");
	printvar(thread2_retval);
	print("\nthread 1 ha acabado con retval: ");
	printvar(thread1_retval);

	pthread_create(&thread3_TID, hola, (void *)3);

	pthread_join(thread3_TID, &thread3_retval);

	println("ESTO NO DEBERÍA IMPRIMIRSE porque thread3 no acabará así que masterthread se bloquea");

	while (1)
		;
	return true;
}

void *while1()
{
	while (1)
		;
	return 0;
}

void *join_con_masterthread()
{
	int e = pthread_join(0, NULL); //JOIN CON MASTERTHREAD. Y masterthread se joineara con este. El objetivo es provocar un edeadlk
	if (e == -1)
		perror();
	return 0;
}

void *joinearme_con_quien_me_pasan_de_argumento(void *tid)
{
	int e = pthread_join((int)tid, NULL);
	if (e == -1)
		println("test superado, no se queda joineado");
	return 0;
}

/* EXPLICACION TEST:
Este test debe imprimir 2 veces el codigo de error EDEADLK. La primera cuando se retorna EDEADLK por intentar joinearse consigo mismo. La segunda es porque masterthread
está intentando joinearse con otro thread y ese otro thread está intentando joinearse con masterthread. El que haya llegado último a este joineo mútuo deberá retornar 
EDEADLK e imprimir ese codigo de error.
*/
int pthread_join_EDEADLK(void)
{
	int e = pthread_join(0, NULL); //JOIN CONMIGO MISMO
	if (e == -1)
		perror();
	println("");

	int TID;
	pthread_create(&TID, join_con_masterthread, (void *)1);
	int e2 = pthread_join(TID, NULL); //join con un thread y ese otro thread se joinea conmigo
	if (e2 == -1)
		perror();
	println("");

	return true;
}

/* EXPLICACIÓN TEST:
Crearemos los threads A y B. B hara join con A y masterthread hara join con A también. Uno de ellos (o B o masterthread) deberían salir del join con un EINVAL
por estar intentando joinearse 2 al mismo y el otro sí se deberá quedar en el join. Nota que 'A' no acabará nunca porque está en un while(1) */
int pthread_join_EINVAL(void)
{
	int TID1, TID2;
	pthread_create(&TID1, while1, (void *)1);
	pthread_create(&TID2, joinearme_con_quien_me_pasan_de_argumento, (void *)TID1);
	int e = pthread_join(TID1, NULL);
	if (e == -1)
		println("test superado, no se queda joineado");
	return true;
}

void* funcion_vacia(void* arg){
	return (void*)0;
}

/* EXPLICACIÓN TEST:
Nos intentaremos joinear con un thread que no existe. El test pasará si el pthread_join acaba con error (y ese error es ESRCH). Si acaba de cualquier otra
forma o simplemente se queda bloqueado indefinidamente será que no se ha superado el test.*/
int pthread_join_ESRCH(void)
{
	int TID;
	pthread_create(&TID, &funcion_vacia, (void *)1);
	int e = pthread_join(123, NULL); //nadie tiene el TID 123
	if (e == -1 && errno == ESRCH)
		return true;
	return false;
}

/* EXPLICACIÓN TEST:
El argumento "puntero a retval" de pthread_join que utilizaremos va a ser la dirección de una zona de código y también la dirección de una zona de kernel.
Ambos pthread_join deberán retornar error y que ese error sea EFAULT.
Si cualquiera de los 2 joins no retorna EFAULT (o se queda bloqueado) será que no se ha superado el test*/
int pthread_join_EFAULT(void)
{
	int TID, TID2;
	pthread_create(&TID, funcion_vacia, (void *)1);
	pthread_create(&TID2,funcion_vacia, (void *)1);
	int e = pthread_join(TID, (int *)&hola); //el puntero que estamos pasando es un puntero a una zona de codigo!
	if(e != -1 || errno != EFAULT)
		return false;
	int e2 = pthread_join(TID2, (int *)0x075); //el puntero que estamos pasando es un puntero a una direccion de kernel (el kernel va de 0x0 a 0xFFFFF)
	if(e2 != -1 || errno != EFAULT)
		return false;
	return true;
}
