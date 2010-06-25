#ifndef __BUBL_SYMBOLS_H__
#define __BUBL_SYMBOLS_H__

/*
 * These symbols are defined in the linker script. While they are not
 * really arrays of bytes, we want them defined such that no "&" is needed
 * in the code using them. They are bare constant addresses.
 */
extern u8 _start[];
extern u8 _end_header[];
extern u8 _end[];
extern u8 __size__[];

#endif
