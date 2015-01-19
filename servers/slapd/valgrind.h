#ifdef USE_VALGRIND
	/* Get debugging help from Valgrind */
#	include <valgrind/memcheck.h>
#else
#	define VALGRIND_CREATE_MEMPOOL(h,r,z)
#	define VALGRIND_DESTROY_MEMPOOL(h)
#	define VALGRIND_MEMPOOL_TRIM(h,a,s)
#	define VALGRIND_MEMPOOL_ALLOC(h,a,s)
#	define VALGRIND_MEMPOOL_FREE(h,a)
#	define VALGRIND_MEMPOOL_CHANGE(h,a,b,s)
#	define VALGRIND_MAKE_MEM_NOACCESS(a,s)
#	define VALGRIND_MAKE_MEM_DEFINED(a,s)
#	define VALGRIND_MAKE_MEM_UNDEFINED(a,s)
#	define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#	define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#endif

