/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno __attribute__((section(".ErrnoSection"))); // See changes made in user.lds

char buffer[256];

void itoa(int a, char *b)
{
  int i, i1;
  char c;

  if (a == 0)
  {
    b[0] = '0';
    b[1] = 0;
    return;
  }

  i = 0;
  while (a > 0)
  {
    b[i] = (a % 10) + '0';
    a = a / 10;
    i++;
  }

  for (i1 = 0; i1 < i / 2; i1++)
  {
    c = b[i1];
    b[i1] = b[i - i1 - 1];
    b[i - i1 - 1] = c;
  }
  b[i] = 0;
}

int strlen(char *a)
{
  int i;

  i = 0;

  while (a[i] != 0)
    i++;

  return i;
}

void perror()
{
  char buffer[256];

  itoa(errno, buffer);

  write(1, buffer, strlen(buffer));
}

void print(char *s)
{
  write(1, s, strlen(s));
}

void println(char *s)
{
  write(1, "\n", 1);
  write(1, s, strlen(s));
}

void printvar(int var)
{
  itoa(var, buffer);
  print(buffer);
}

void delay(unsigned long ticks)
{
  unsigned long now = gettime();
  // Users have to specifcy ticks because the time equivalence (seconds/ticks) is machine dependent in ZeOS
  while (now + ticks >= gettime())
  {
  }
}