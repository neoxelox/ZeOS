#include <libc.h>
#include <test.h>

char buff[256];

int pid;

int test(char *name, int (*func)(), int output);

int __attribute__((__section__(".text.main")))
main(void)
{
  /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
  /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  /* !! PLEASE READ BELOW !! */

  // AS THERE IS NOT ENOUGH SPACE IN THE SCREEN FOR ALL THE TESTS OUTPUTS PLEASE
  // ENABLE THE TESTS YOU WANT IN ORDER TO SEE THEM ALL (UNCOMMENT THEM).
  // IF YOU WANT TO SEE THE IMPLEMENTATION OF EACH TEST, JUST NAVIGATE TO: test/<feature>.c
  // IF A TEST PASSES IT WILL OUTPUT SOMETHING LIKE: [PASSED] : <test title>
  // OR: [FAILED] : <test title> IF IT DOES NOT PASS.
  // IF A TEST REQUIRES SCREEN OUTPUT, THAT OUTPUT WILL BE SORROUNDED BY A BOX
  // LIKE: ======= [OUTPUT] : <test title> =======

  // NOTICE THAT THERE ARE EXPLANATIONS FOR EACH TEST ABOVE THEIR IMPLEMENTATION, READ IT!

  /* !! PLEASE READ ABOVE !! */

  /* Fork TestCase */
  // test("Fork success", &fork_success, true);
  // test("Fork EPERM", &fork_EPERM, false);

  /* Exit TestCase */
  // test("Exit success", &exit_success, true);
  // test("Exit calling thread is not master", &exit_calling_thread_is_not_master, false);

  /* Pthread_create TestCase */
  // test("Pthread_create success", &pthread_create_success, false);
  // test("Pthread_create EFAULT", &pthread_create_EFAULT, false);
  // test("Pthread_create EAGAIN", &pthread_create_EAGAIN, false);

  /* Pthread_exit TestCase */
  // test("Pthread_exit success", &pthread_exit_success, false);

  /* Pthread_join TestCase (THIS TEST CASE, PLEASE, ONE AT A TIME, IT DOES NOT FOLLOW THE CONVENTION MENTIONED BEFORE) */
  // test("Pthread_join success", &pthread_join_success, false);
  // test("Pthread_join EDEADLK", &pthread_join_EDEADLK, false);
  // test("Pthread_join EINVAL", &pthread_join_EINVAL, false);
  // test("Pthread_join ESRCH", &pthread_join_ESRCH, false);
  // test("Pthread_join EFAULT", &pthread_join_EFAULT, false);

  /* Mutex_init TestCase */
  // test("Mutex_init success and EAGAIN", &mutex_init_success_EAGAIN, false);

  /* Mutex_destroy TestCase */
  // test("Mutex_destroy success", &mutex_destroy_success, false);
  // test("Mutex_destroy EINVAL", &mutex_destroy_EINVAL, false);
  // test("Mutex_destroy EBUSY", &mutex_destroy_EBUSY, false);
  // test("Mutex_destroy EPERM", &mutex_destroy_EPERM, false);

  /* Mutex_lock and unlock TestCase */
  // test("Mutex_lock and unlock success", &mutex_lock_unlock_success, false);
  // test("Mutex_lock EINVAL", &mutex_lock_EINVAL, false);
  // test("Mutex_lock EDEADLK", &mutex_lock_EDEADLK, false);
  // test("Mutex_unlock EINVAL", &mutex_unlock_EINVAL, false);
  // test("Mutex_unlock EPERM", &mutex_unlock_EPERM, false);

  /* Pthread_key_create TestCase */
  // test("Pthread_key_create success and EAGAIN", &pthread_key_create_success_EAGAIN, false);

  /* Pthread_key_delete TestCase */
  // test("Pthread_key_delete success", &pthread_key_delete_success, false);
  // test("Pthread_key_delete EINVAL", &pthread_key_delete_EINVAL, false);

  /* Pthread_getspecific and setspecific TestCase */
  // test("Pthread_getspecific and setspecific success", &pthread_getspecific_setspecific_success, false);
  // test("Pthread_setspecific and setspecific EINVAL", &pthread_getspecific_setspecific_EINVAL, false);

  while (1)
  {
  }
}

int test(char *name, int (*func)(), int output)
{
  if (output == true)
  {
    println("====== [OUTPUT] : ");
    print(name);
    print(" ======");
  }
  else
  {
    println("");
  }

  int passed = func();

  if (output == true)
  {
    println("=========================");
    for (int i = 0; i < strlen(name); i++)
    {
      buff[i] = '=';
    }
    print(buff);
    println("");
  }

  if (passed == true)
  {
    print("[PASSED] : ");
    print(name);
    return true;
  }

  print("[FAILED] : ");
  print(name);
  return false;
}
