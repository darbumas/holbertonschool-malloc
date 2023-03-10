#include "malloc.h"

static heap_info heap;

/**
 * add_header - header info add
 * @addr: hader of the address
 * @size: size of the memory block
 * @prev: previous chunk offset
 */
void add_header(void *addr, size_t size, size_t prev)
{
	block_info *block = NULL;

	block = addr;
	block->size = prev ? size + HDR_SZ + 1 : size + HDR_SZ;
	block->prev = prev;
}

/**
 * addNewPage - use sbrk()
 * @size: size needed by user
 * Return: pointer to memory block header
 */
void *addNewPage(size_t size)
{
	size_t pg_sz;
	char *p = NULL, *tmp = NULL;

	pg_sz = 2 * HDR_SZ + MIN_SIZE + size;
	pg_sz += heap.heap_start ? 0 : HDR_SZ;
	pg_sz = align_up(pg_sz, PAGESIZE);
	p = sbrk(pg_sz);
	if (p == (void *)-1 && errno == ENOMEM)
		return (NULL);
	if (heap.heap_start == NULL)
	{
		/* Calling addNewPage() for the first time */
		heap.heap_start = p;
		/* Set the first chunk, returned to USER */
		add_header(p, size, 0);
		/* Set middle and sentinel chunks */
		add_header(p + GET_SIZE(p), pg_sz - size - 3 * HDR_SZ, 0);
		add_header(p + pg_sz - HDR_SZ, 0, pg_sz - size - 2 * HDR_SZ);
		return (p);
	}
	else
	{
		/*
		 * Subsequent calls to addNewPage()
		 * Make p point to sentinel chunk (block) and convert
		 * that sentinel to regular chunk, which will be returned to
		 * a user
		 */
		p -= HDR_SZ;
		tmp = p;
		/* Change previous sentinel chunk, returned to USER */
		add_header(p, size, GET_PREV(p));
		p += GET_SIZE(p);
		add_header(p, pg_sz - size - 2 * HDR_SZ, 0);
		add_header(p + GET_SIZE(p), 0, pg_sz - size - HDR_SZ);
		return (tmp);
	}
}

/**
 * find_block - find an unused memory block to return to a user
 * @size: size needed by a user
 * Return: pointer to a memory block header
 */
void *find_block(size_t size)
{
	char *p = heap.heap_start, *tmp;
	size_t req_sz = size + HDR_SZ;

	if (!p)
		return (NULL);

	while (GET_SIZE(p) >= HDR_SZ + MIN_SIZE)
	{
		p += GET_SIZE(p);
		if ((_GET_SIZE(p) & 1) && GET_SIZE(p - GET_PREV(p)) >= req_sz)
		{
			tmp = p - GET_PREV(p);
			if (GET_SIZE(tmp) >= req_sz + HDR_SZ + MIN_SIZE)
			{
				/* Split the chunk */
				add_header(tmp + req_sz, GET_SIZE(tmp) - req_sz - HDR_SZ, 0);
				((block_info *)tmp)->size = req_sz;
				((block_info *)p)->prev = GET_SIZE(tmp + req_sz);
			}
			else
			{
				/* Don't split the chunk */
				((block_info *)p)->size &= LSB_ZERO_MASK;
				((block_info *)p)->prev = 0;
			}

			return (tmp);
		}
	}
	return (NULL);
}

/**
 * _malloc - malloc function implementation
 * @size: size need by a user
 * Return: a pointer to heap with at least @size usable memory,
 * NULL on failure or when @size is 0
 */
void *_malloc(size_t size)
{
	char *p;

	size = align_up(size, ALIGNMENT);
	if (size == 0)
		return (NULL);
	p = find_block(size);
	if (!p)
		p = addNewPage(size);
	if (!p)
		return (NULL);
	return (p + HDR_SZ);
}
