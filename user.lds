/*
 *  ZeOS - jcosta septembre 2006
 *  user.lds - Linker Script to create user memory image
 */

ENTRY(main)
SECTIONS
{
  . = 0x100000; /* User CODE will start at this address */
    
  .text.main : { *(.text.main) }
  .text : { *(.text) }

  . = 0x108000; /* User DATA will start at this address */
  .rodata : { *(.rodata) }      /* Read Only Data */
  .data : { *(.data) }          /* Normal Data */
  .bss : { *(.bss) }            /* Uninitialized Data */


  . = 0x109000; /* Section added at the address 0x109000 in order to store errno. Now, the errno variable will always have this address. */
  .ErrnoSection : { *(.ErrnoSection) } /* Note that the errno initialization in lib.c is:
				      int errno __attribute__((section(".ErrnoSection")));*/
}
