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
    ""};

/* Basic constants and macros */
#define WSIZE 4             /* WORD and header / footer size*/
#define DSIZE 8             /* Double Word Size*/
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */ //4100

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) /* 하위 3비트를 제외한 녀석들이 블록의 크기 비트를 표현한다  */
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define PRED_P(bp) ((char *)(bp))
#define SUCC_P(bp) ((char *)(bp) + WSIZE)

#define SET_PRED_P(bp, targetBp) (PUT(PRED_P(bp), targetBp))
#define SET_SUCC_P(bp, targetBp) (PUT(SUCC_P(bp), targetBp))

#define GET_PRED_P(bp) ((void *)(GET(PRED_P(bp))))
#define GET_SUCC_P(bp) ((void *)(GET(SUCC_P(bp))))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp)-WSIZE))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void arrageBlock(void *targetBp);
static void headInsert(void *bp);
static int getSizeIndex(size_t size);

void *heap_listp = NULL;

#define LIST_SIZE 20

void *seg_list[LIST_SIZE] = {
    0,
}; // 밑의 사이즈 크기와 연계하여, 해당하는 가용 블록들의 헤더를 가리킬 녀석들
size_t size_list[LIST_SIZE] = {
    2,
    0,
}; // 해당하는 사이즈 이하의

int mm_init(void)
{
    /* 초기화 */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 header (총 사이즈 16바이트)
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 에필로그 header
    heap_listp += 2 * WSIZE;                       // 현재는 프롤로그의 '이전'을 첫번째 head로 가리킨다

    for (size_t i = 1; i < LIST_SIZE; i++)
    {
        size_list[i] = size_list[i - 1] * 2;
        seg_list[i] = NULL;
    }

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

/* 새 가용 블록으로 힙 확장 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* size 홀수이면 1 더해서 WSIZE 곱해주고 대입*/
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    /* 힙 할당 해주고 bp에 넣어줌*/
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL; /* 힙 할당은... 실패다...*/

    /* 새로운 가용블록의 헤더랑 푸터랑 초기화해주기 */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    /* 새로운 에필로그 헤더 생성*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

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

static void *find_fit(size_t asize)
{
    size_t index = getSizeIndex(asize);

    void *bp;

    for (size_t i = index; i < LIST_SIZE; i++)
    {
        for (bp = seg_list[i];
             bp != NULL && !GET_ALLOC(HDRP(bp));
             bp = GET_SUCC_P(bp))
        {
            if (asize <= GET_SIZE(HDRP(bp)))
            {
                return bp;
            }
        }
    }

    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    arrageBlock(bp);

    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        // 할당 해제할 블록들
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));

        // 새로 할당 해제 되었으니
        // '통합' 함수 호출 (내부에서 headInsert 존재)
        coalesce(bp);
    }
    else /* 할당 사이즈와 시작 블록 사이즈의 차이가, 최소 할당크기보다 작다면 내버려둠*/
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

// 세그먼트 리스트 이전에
// 해당 내용 설정시, 리스트 세팅을 해주는 방향으로
static void arrageBlock(void *targetBp)
{
    if (targetBp == NULL)
        return;

    // 이중 연결 리스트의 정리 과정이다
    // 해당 블록의 이전은 '다음 블록'의 이전으로 보내고
    // 해당 블록의 '다음'은 '이전 블록'의 다음으로 보낸다
    void *nextBp = GET_SUCC_P(targetBp);
    void *postBp = GET_PRED_P(targetBp);

    if (nextBp != NULL)
    {
        SET_PRED_P(nextBp, postBp);
    }

    if (postBp != NULL)
    {
        SET_SUCC_P(postBp, nextBp);
    }
    else // 이전 블록이 NULL인 경우, 얘가 head 였음
    {
        size_t index = getSizeIndex(GET_SIZE(HDRP(targetBp)));
        seg_list[index] = nextBp;
    }  
}

static void headInsert(void *bp)
{
    if (bp == NULL)
        return;

    size_t index = getSizeIndex(GET_SIZE(HDRP(bp)));

    void* headBlockPtr = seg_list[index];

    // 해당하는 리스트 포인터와 bp의 위치를 갱신한다
    SET_SUCC_P(bp, headBlockPtr);
    SET_PRED_P(bp, NULL);
    if (headBlockPtr != NULL)
        SET_PRED_P(headBlockPtr, bp);
    seg_list[index] = bp;
}

void mm_free(void *bp)
{
    if (bp == NULL)
        return;

    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

// 블록 병합
// 이쪽으로 들어오는 경우는
// place와는 반대로, '블록'이 '해제'되거나 새로 생기는 경우이다
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 이전 footer
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && !next_alloc)
    {
        /* case 2 이전 블록은 할당되었고, 다음 블록은 가용상태 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); /* 다음 블록의 사이즈만큼 더함 (다음 헤더에서 가지고 온다) */

        // 가용 상태인 다음 블록을 정리해준다
        arrageBlock(NEXT_BLKP(bp));

        PUT(HDRP(bp), PACK(size, 0)); /*size | 0 은 size와 같음, 그러나 가독성과 통일성을 위해 사용*/
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        /* case 3 이전 블록은 가용 상태, 다음 블록은 할당상태 */

        // 가용 상태인 이전 블록을 정리해준다
        arrageBlock(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        // 이전 블록과 합쳤으므로 이전 블록의 블록 포인터로 옮긴다
        bp = PREV_BLKP(bp);
    }
    else if (!prev_alloc && !next_alloc)
    {
        /* case 4 이전 블록과 다음 블록 모두 가용 상태*/
        // 두 블록 모두 정리해준다
        arrageBlock(PREV_BLKP(bp));
        arrageBlock(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        // 이전 블록의 블록 포인터로 옮긴다
        bp = PREV_BLKP(bp);
    }

    // 정리 완료된 블록을
    // head에 넣어준다
    headInsert(bp);

    return bp;
}

int getSizeIndex(size_t size)
{
    size_t index = LIST_SIZE - 1;

    for (size_t i = 0; i < LIST_SIZE; i++)
    {
        if (size < size_list[i])
        {
            index = i;
            break;
        }
    }

    return index;
}

void *mm_realloc(void *ptr, size_t size)
{
    size_t copySize = GET_SIZE(HDRP(ptr));
    size_t needSize = size + 2 * WSIZE; // 추가 요구 사이즈 + 새로 할당할 헤더와 푸터
    // 현재 블록 사이즈가 요구 사이즈보다 이미 크다
    if (copySize >= needSize)
        return ptr;

    if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
    {
        // 제자리에서 재할당 하는 경우,
        // 새로 할당될 헤더와 푸터를 신경쓸 필요는 없음
        size_t sumSize = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + copySize;
        if(sumSize >= size)
        {
            arrageBlock(NEXT_BLKP(ptr));
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