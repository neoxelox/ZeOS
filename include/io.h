/*
 * io.h - Definició de l'entrada/sortida per pantalla en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#include <types.h>

/** Screen functions **/
/**********************/

Byte inb(unsigned short port);

void printc(char c);
void printc_xy(Byte x, Byte y, char c);
void printc_color(Byte color, char c);
void printc_scroll(char c, Byte color);

void printk(char *string);
void printk_color(char *string, Byte color);

void println(char *string);
void panic(char *string);

void printvar(int var);

#define breakpoint                        \
    printk_color("\n[BREAKPOINT]", 0x03); \
    while (1)                             \
    {                                     \
    }

#endif /* __IO_H__ */
