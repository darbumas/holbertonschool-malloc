#include "malloc.h"

/**
 * _free - deallocates memory block
 * @ptr: pointer to mem space to be free
 */
void _free(void *ptr)
{
	char *p, *next;

	if (!ptr)
		return;
	p = ptr;
	p -= HDR_SZ;
	next = p + GET_SIZE(p);
	if (_GET_SIZE(next) & 1)
	{
		putchar('\n');
		kill(getpid(), SIGABRT);
	}
	((block_info *)next)->prev = GET_SIZE(p);
	((block_info *)next)->size |= 1;
}
