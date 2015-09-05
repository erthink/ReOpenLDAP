/* LY: map memcpy to 2.2.5 for compatibility with RHEL6. */

#define _GNU_SOURCE
#include <features.h>

#if __GLIBC_PREREQ(2,14)

__asm__(".symver memcpy, memcpy@@@GLIBC_2.2.5");

#include <string.h>
void* __attribute__((weak)) memcpy_compat(void *dest, const void *src, size_t n) {
	return memcpy(dest, src, n);
}

#endif /* __GLIBC_PREREQ(2,14) */
