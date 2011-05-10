#include <bubl/types.h>
/* Crappy compat stuff */
typedef u8 uchar;
typedef u16 ushort;
typedef u32 uint;
#define ulong unsigned long /* This way (not typedef) to prevent warnings */
typedef ulong lbaint_t;
#define printf printk
typedef struct {int unused;} bd_t;

#define min(X, Y)                               \
        ({ typeof (X) __x = (X), __y = (Y);     \
                (__x < __y) ? __x : __y; })

#define max(X, Y)                               \
        ({ typeof (X) __x = (X), __y = (Y);     \
                (__x > __y) ? __x : __y; })

#define MIN(x, y)  min(x, y)
#define MAX(x, y)  max(x, y)

