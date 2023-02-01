#include "malloc.h"

#define ALIGN sizeof(void *)

/**
 * adjust_prgm_brk - uses sbrk() to move program break
 *
 * Return: memory address of newly allocated page, or NULL
 */
void *adjust_prgm_brk(void)
{
	void *prev_break = NULL;
	long int page_size = sysconf(_SC_PAGESIZE);

	if (page_size == -1)
	{
		fprintf(stderr, "Sysconf failed: from adjust_prgm_brk()\n");
		return (NULL);
	}

	prev_break = sbrk((intptr_t)page_size);
	if (prev_break == (void *)-1)
	{

		perror("sbrk() failure");
		return (NULL);
	}

	return (prev_break);
}

/**
 * find_fit - searches for a free block on the heap
 * @heapstart: pointer to the original program break
 * @size: memory size requested by user
 * @used_block: account of already allocated mem chunks
 * @free_block_addr: holds addresses of unused heap regions
 * @free_block_size: bite size of free block
 * Return: 0 on success, 1 on failure
 */
int find_fit(void *heapstart, size_t *size, size_t used_block,
	     void **free_block_addr, size_t *free_block_size)
{
	void *chunk;
	size_t chunk_size, i;
	long int page_size;

	if (heapstart == NULL || size == NULL || free_block_addr == NULL ||
	    free_block_size == NULL)
		return (1);

	page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1)
	{
		fprintf(stderr, "Sysconf failed: from find_fit()\n");
		return (1);
	}

	for (chunk = heapstart, i = 0; i < used_block; i++)
	{
		chunk_size = *((size_t *)chunk);
		chunk = (unsigned char *)chunk + chunk_size;
	}

	*free_block_addr = chunk;
	*free_block_size = used_block ?
		*((size_t *)*free_block_addr) : (size_t)page_size;

	*size += (ALIGN - (*size % ALIGN));

	while (*free_block_size < (sizeof(size_t) * 2) + *size)
	{
		if (adjust_prgm_brk() == NULL)
			return (1);
		*free_block_size += page_size;
		*((size_t *)*free_block_addr) = *free_block_size;
	}

	return (0);
}

/**
 * naive_malloc - simple implementation of malloc where blocks of memory
 * are allocated but not freed blocks are NOT reused.
 * @size: memory size requested by user
 * Return: address of newly allocated block on success, or NULL on failure
 */
void *naive_malloc(size_t size)
{
	size_t block_size, free_block_size;
	void *free_block_addr, *prgm_break, *block_addr, *payload;
	static void *heapstart;
	static size_t alloc_chunks;

	if (!heapstart)
	{
		heapstart = adjust_prgm_brk();
		if (heapstart == NULL)
			return (NULL);
	}
	prgm_break = sbrk(0);
	if (prgm_break == (void *)-1)
	{
		perror("sbrk(): from naive_malloc()");
		return (NULL);
	}
	if (find_fit(heapstart, &size, alloc_chunks, &free_block_addr,
			&free_block_size)
	    == 1)
		return (NULL);

	block_addr = free_block_addr;
	block_size = sizeof(size_t) + size;
	*((size_t *)block_addr) = block_size;
	alloc_chunks++;

	free_block_size -= block_size;
	free_block_addr = (unsigned char *)free_block_addr + block_size;
	*((size_t *)free_block_addr) = free_block_size;

	payload = (unsigned char *)block_addr + sizeof(size_t);
	return (payload);
}
