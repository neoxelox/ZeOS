/*
 * io.c - 
 */

#include <io.h>

#include <types.h>

/**************/
/** Screen  ***/
/**************/

#define NUM_COLUMNS 80
#define NUM_ROWS 25

Byte x, y = 19;

char buffer[12];

/* Read a byte from 'port' */
Byte inb(unsigned short port)
{
  Byte v;

  __asm__ __volatile__("inb %w1,%0"
                       : "=a"(v)
                       : "Nd"(port));
  return v;
}

void printc(char c)
{
  __asm__ __volatile__("movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c == '\n')
  {
    x = 0;
    y = (y + 1) % NUM_ROWS;
  }
  else
  {
    Word ch = (Word)(c & 0x00FF) | 0x0200;
    Word *screen = (Word *)0xb8000;
    screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y = (y + 1) % NUM_ROWS;
    }
  }
}

void printc_color(Byte color, char c)
{
  __asm__ __volatile__("movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c == '\n')
  {
    x = 0;
    y = (y + 1) % NUM_ROWS;
  }
  else
  {
    Word ch = (Word)(c & 0x00FF) | (color << 8);
    Word *screen = (Word *)0xb8000;
    screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y = (y + 1) % NUM_ROWS;
    }
  }
}

void printc_scroll(char c, Byte color)
{
  __asm__ __volatile__("movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */

  if (y == NUM_ROWS)
  {
    Word *screen = (Word *)0xb8000;
    for (int i = 0; i < NUM_ROWS - 1; i++)
    {
      for (int j = 0; j < NUM_COLUMNS; j++)
      {
        screen[(i * NUM_COLUMNS + j)] = screen[((i + 1) * NUM_COLUMNS + j)];
      }
    }

    for (int j = 0; j < NUM_COLUMNS; j++)
    {
      screen[((NUM_ROWS - 1) * NUM_COLUMNS + j)] = 0;
    }

    --y;
  }

  if (c == '\n')
  {
    x = 0;
    y++;
  }
  else
  {
    Word ch = (Word)(c & 0x00FF) | (color << 8); // (Word)(c & 0x00FF) | 0x0200;
    Word *screen = (Word *)0xb8000;
    screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y++;
    }
  }
}

void printc_xy(Byte mx, Byte my, char c)
{
  Byte cx, cy;
  cx = x;
  cy = y;
  x = mx;
  y = my;
  printc(c);
  x = cx;
  y = cy;
}

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc_scroll(string[i], 0x02);
}

void println(char *string)
{
  printk("\n");
  printk(string);
}

void printk_color(char *string, Byte color)
{
  int i;
  for (i = 0; string[i]; i++)
    printc_scroll(string[i], color);
}

void panic(char *string)
{
  printk_color("\n[ERROR]: ", 0x04);
  printk_color(string, 0x04);
  breakpoint;
}

void int_to_string(int a, char *b)
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

void printvar(int var)
{
  int_to_string(var, buffer);
  printk_color("\n[VARIABLE]: ", 0x03);
  printk_color(buffer, 0x03);
}