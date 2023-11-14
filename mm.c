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

team_t team = {
    "krafton_j_3_6",
    "Hyun JaeHoon",
    "hnjogo@gmail.com",
    "",
    ""
};

/* Basic constants and macros */
#define WSIZE 4 /* WORD and header / footer size*/
#define DSIZE 8 /* Double Word Size*/
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x,y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size,alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) /* 하위 3비트를 제외한 녀석들이 블록의 크기 비트를 표현한다  */
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);

void* heap_listp = NULL;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 초기화 */
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void*) -1)
        return -1;

    PUT(heap_listp,0); /* 정렬 패딩 */
    PUT(heap_listp + (1*WSIZE),PACK(DSIZE,1));  /* 프롤로그 헤더 */
    PUT(heap_listp + (2*WSIZE),PACK(DSIZE,1));  /* 프롤로그 푸터 */
    PUT(heap_listp + (3*WSIZE),PACK(0,1));  /* 에필로그 헤더 */
    heap_listp += (2*WSIZE);

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

/* 새 가용 블록으로 힙 확장 */
static void* extend_heap(size_t words)
{
    char * bp;
    size_t size;

    /* size 홀수이면 1 더해서 WSIZE 곱해주고 대입*/
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    /* 힙 할당 해주고 bp에 넣어줌*/
    if((long)(bp = mem_sbrk(size)) == -1) 
        return NULL; /* 힙 할당은... 실패다...*/

    /* 새로운 가용블록의 헤더랑 푸터랑 초기화해주기 */
    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));

    /* 새로운 에필로그 헤더 생성*/
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1)); 

    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;
    
    if (size <= DSIZE)
    {
        /* 최소 할당 크기 : 16*/
        asize = DSIZE * 2;
    }
    else
    {
        /* 8 바이트 정렬 요건 만족을 위한 할당 사이즈*/
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);
    }

    if((bp = find_fit(asize)) != NULL)
    {
        place(bp,asize);
        return bp;
    }

    extendsize = MAX(asize,CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    
    place(bp,asize);
    return bp;
}

static void* find_fit(size_t asize)
{
    /* next fit*/
    void * bp;

    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0 ; bp = NEXT_BLKP(bp))
    {
        /* 헤더 확인하니 가용 상태, 해당 블록 사이즈가 asize보다 큼 */
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }

    /* 할당 안됨 */
    return NULL;
}

/* 요청한 블록을 가용 블록의 시작 부분에 배치, 
나머지 부분의 크기가 최소 블록 크기와 같거나 큰 경우에만 분할 */
// 할당된 블록에서 호출

static void place(void* bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2*DSIZE))
    {
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));

        // 여유 공간 반환하기
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp),PACK(csize - asize,0));
        PUT(FTRP(bp),PACK(csize - asize,0));
    }
    else /* 할당 사이즈와 시작 블록 사이즈의 차이가, 최소 할당크기보다 작음 */
    {
        PUT(HDRP(bp),PACK(csize,1));
        PUT(FTRP(bp),PACK(csize,1));
    }
}

void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    coalesce(bp);
}

static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc)
    {
        /* case 1 이전과 다음이 모두 할당된 블록이다 */
        
        //return bp;
    }
    else if(prev_alloc && !next_alloc)
    {
        /* case 2 이전 블록은 할당되었고, 다음 블록은 가용상태 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); /* 다음 블록의 사이즈만큼 더함 (다음 헤더에서 가지고 온다) */
        PUT(HDRP(bp),PACK(size,0)); /*size | 0 은 size와 같음, 그러나 가독성과 통일성을 위해 사용*/
        PUT(FTRP(bp),PACK(size,0));
        
    }
    else if(!prev_alloc && next_alloc)
    {
        /* case 3 이전 블록은 가용 상태, 다음 블록은 할당상태 */
        size += GET_SIZE(FTRP(PREV_BLKP(bp))); /* 다음 블록의 사이즈만큼 더함 (다음 헤더에서 가지고 온다) */
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0)); /* 이전 블록과 자신을 통합해야 하기에, 이전 블록의 헤더에 사이즈 변경 */
        bp = PREV_BLKP(bp); /* 이전 블록으로 bp를 옮긴다 */
    }
    else
    {
        /* case 4 이전 블록과 다음 블록 모두 가용 상태*/
        size += GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));  /* 다음 블록의 footer에 값을 써줌 */
        bp = PREV_BLKP(bp); /* 이전 블록으로 bp를 옮긴다 */
    }

    return bp;
}

void *mm_realloc(void *ptr, size_t size)
{
    size_t copySize = GET_SIZE(HDRP(ptr));
    size_t needSize = size + 2*WSIZE; // 추가 요구 사이즈 + 새로 할당할 헤더와 푸터
    // 현재 블록 사이즈가 요구 사이즈보다 이미 크다
    if (copySize >= needSize)
        return ptr;

    // 다음 블록이 가용 블록이다
    if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
    {
        // 제자리에서 재할당 하는 경우,
        // 새로 할당될 헤더와 푸터를 신경쓸 필요는 없음
        size_t sumSize = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + copySize;
        if(sumSize >= size)
        {
            PUT(HDRP(ptr),PACK(sumSize,1));
            PUT(FTRP(ptr),PACK(sumSize,1));
            return ptr;
        }
    }

    void *newptr = mm_malloc(needSize);
    if (newptr == NULL)
        return NULL;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);

    return newptr;
}
