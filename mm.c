/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20211180",
    /* Your full name*/
    "Kichan Nam",
    /* Your email address */
    "tyler1123@sogang.ac.kr",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define PACK(size,alloc) ((size)|(alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))

#define GET_SIZE(p) (GET(p) & ~0X7)
#define GET_ALLOC(p) (GET(p) & 0X1)

#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define FBLNEXT_P(bp) (*(char**)(bp + WSIZE))
#define FBLPREV_P(bp) (*(char**)(bp))

void* heap_listp = NULL;
void* free_p = NULL;


void FreeInsert(void* bp) // Insert at front
{
    FBLNEXT_P(bp) = free_p;
    FBLPREV_P(free_p) = bp;
    FBLPREV_P(bp) = NULL;
    free_p = bp;
    return;
}
void FreeDelete(void *bp)
{
    if (bp == free_p)
    {
        FBLPREV_P(FBLNEXT_P(bp)) = NULL;
        free_p = FBLNEXT_P(bp);
    }
    else
    {
        void* p1 = FBLNEXT_P(bp);
        void* p2 = FBLPREV_P(bp);
        FBLPREV_P(FBLNEXT_P(bp)) = p2;
        FBLNEXT_P(FBLPREV_P(bp)) = p1;
    }
    return;
}
static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
        ;
    else if (prev_alloc && !next_alloc)
    {
        FreeDelete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc)
    {
        FreeDelete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else
    {
        FreeDelete(NEXT_BLKP(bp));
        FreeDelete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

    }

    FreeInsert(bp);
    return bp;
}


static void* extend_heap(size_t words)
{
    char* bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : (words) * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void* find_fit(size_t asize) // first-fit search
{
    void* bp;
    for (bp = free_p; GET_ALLOC(HDRP(bp)) != 1; bp = FBLNEXT_P(bp))
    {
        if (asize <= GET_SIZE(HDRP(bp)))
        {
            return bp;
        }
    }

    return NULL;
}

static void place(void* bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2*DSIZE))
    {
        FreeDelete(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        FreeInsert(bp);
    }
    else
    {
        FreeDelete(bp);
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(6*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0); 
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE*2, 1)); 
    PUT(heap_listp + (2*WSIZE), NULL);  
    PUT(heap_listp + (3*WSIZE), NULL); 
    PUT(heap_listp + (4*WSIZE), PACK(DSIZE*2, 1)); 
    PUT(heap_listp + (5*WSIZE), PACK(0, 1));   

    free_p = heap_listp + 2*WSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; 
    size_t extendsize; 
    char* bp;


    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL)
    { 
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE); 
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{

    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));  
    coalesce(bp);
}
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;  
    size_t copySize; 

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
