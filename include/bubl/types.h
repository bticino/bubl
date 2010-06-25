#ifndef __BUBL_TYPES_H__
#define __BUBL_TYPES_H__
#include <stdint.h>

typedef uint8_t		u8;
typedef int8_t		s8;
typedef uint16_t	u16;
typedef int16_t		s16;
typedef uint32_t	u32;
typedef int32_t		s32;
typedef uint64_t	u64;
typedef int64_t		s64;

typedef u32 size_t;
typedef s32 ptrdiff_t;

struct ubl_header {
	u32 magic;
	void *entry_point;
	u32 size;
	u32 params;
	void *startaddr;
	void *loadaddr;
};

#endif
