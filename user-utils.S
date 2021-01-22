#include <asm.h>

ENTRY(syscall_sysenter)
	push %ecx
	push %edx
	push $SYSENTER_RETURN
	push %ebp
	mov %esp, %ebp
	sysenter
ENTRY(SYSENTER_RETURN)
	pop %ebp
	pop %edx
	pop %edx
	pop %ecx
	ret

/* int write(int fd, char *buffer, int size) */
ENTRY(write)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $4, %eax
	movl 0x8(%ebp), %ebx;	//fd
	movl 0xC(%ebp), %ecx;	//buffer
	movl 0x10(%ebp), %edx;	//size
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -> 
	popl %ebp
	ret

/* Common code for negative return */
nok:
	neg %eax
	mov %eax, errno
	mov $-1, %eax
	popl %ebp
	ret

/* int gettime() */
ENTRY(gettime)
	pushl %ebp
	movl %esp, %ebp
	movl $10, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int getpid() */
ENTRY(getpid)
	pushl %ebp
	movl %esp, %ebp
	movl $20, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int fork() */
ENTRY(fork)
	pushl %ebp
	movl %esp, %ebp
	movl $2, %eax
	call syscall_sysenter
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* void exit() */
ENTRY(exit)
	pushl %ebp
	movl %esp, %ebp
	movl $1, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int yield() */
ENTRY(yield)
	pushl %ebp
	movl %esp, %ebp
	movl $13, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int get_stats(int pid, struct stats *st) */
ENTRY(get_stats)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $35, %eax
	movl 0x8(%ebp), %ebx;	//pid
	movl 0xC(%ebp), %ecx;	//st
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* int pthread_create(int *TID, void *(*start_routine)(), void *arg) */
ENTRY(pthread_create)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	pushl %esi
	movl $36, %eax
	movl 8(%ebp), %ebx // &TID
	leal thread_start_routine, %ecx // inject &wrap_routine
	movl 12(%ebp), %edx // &start_routine
	movl 16(%ebp), %esi // arg
	call syscall_sysenter
	popl %esi
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* void pthread_exit(int retval) */
ENTRY(pthread_exit)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $37, %eax
	movl 8(%ebp), %ebx  // retval
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* int pthread_join(int TID, int *retval) */
ENTRY(pthread_join)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $38, %eax
	movl 8(%ebp), %ebx  // TID
	movl 12(%ebp), %ecx // &retval
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* int mutex_init() */
ENTRY(mutex_init)
	pushl %ebp
	movl %esp, %ebp
	movl $39, %eax
	call syscall_sysenter
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* int mutex_destroy(int mutex_id) */
ENTRY(mutex_destroy)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $40, %eax
	movl 8(%ebp), %ebx  // mutex_id
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* int mutex_lock(int mutex_id) */
ENTRY(mutex_lock)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $41, %eax
	movl 8(%ebp), %ebx  // mutex_id
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* int mutex_unlock(int mutex_id) */
ENTRY(mutex_unlock)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $42, %eax
	movl 8(%ebp), %ebx  // mutex_id
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* int pthread_key_create() */
ENTRY(pthread_key_create)
	pushl %ebp
	movl %esp, %ebp
	movl $43, %eax
	call syscall_sysenter
	test %eax, %eax
	js nok	
	popl %ebp
	ret

/* int pthread_key_delete(int key) */
ENTRY(pthread_key_delete)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $44, %eax
	movl 8(%ebp), %ebx  // key
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* void* pthread_getspecific(int key) */
ENTRY(pthread_getspecific)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $45, %eax
	movl 8(%ebp), %ebx  // key
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* int pthread_setspecific(int key, void* value) */
ENTRY(pthread_setspecific)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx  // Save EBX, ESI and EDI if modified
	movl $46, %eax
	movl 8(%ebp), %ebx  // key
	movl 12(%ebp), %ecx // value
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) jump to nok
	popl %ebp
	ret

/* void thread_start_routine(void *(*start_routine)(), void *arg) */
ENTRY(thread_start_routine)
	pushl %ebp
	movl %esp, %ebp

	pushl %ebx
	pushl %esi
	pushl %edi

	movl 8(%ebp), %ebx // &start_routine
	movl 12(%ebp), %esi // arg

	pushl %esi
	call *%ebx // start_routine(arg)
	popl %esi

	// Terminate current thread calling pthread_exit
	pushl %eax
	call pthread_exit // pthread_exit(%eax)
	popl %eax

	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	ret
