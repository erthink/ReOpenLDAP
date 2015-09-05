/* LY: map memcpy to 2.2.5 for compatibility with RHEL6. */

#define _GNU_SOURCE
#include <features.h>

#if __GLIBC_PREREQ(2,14)

__asm__(".symver memcpy, memcpy@@@GLIBC_2.2.5");
__asm__(".symver clock_gettime, clock_gettime@@@GLIBC_2.2.5");

#if LTO_BUG_WORKAROUND
#include <string.h>
extern void *memcpy_compat(void *dest, const void *src, size_t n);
#define memcpy(d, s, n) \
	((__builtin_constant_p(n) && (n) < 65) ? \
		 __builtin_memcpy(d, s, n) : memcpy_compat(d, s, n) )
#endif /* LTO_BUG_WORKAROUND */


#endif /* __GLIBC_PREREQ(2,14) */
