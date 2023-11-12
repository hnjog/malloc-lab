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

/*
    전략에 대하여
    '묵시적 리스트'에서 '명시적 리스트'로?

    '분리 저장 장치'?

    first fit, next fit, best fit?

    일단 책에서 추천해준 방식은
    best fit + 분리 가용 리스트(분리 맞춤)

    (그 외에는 first fit + 명시적 가용 리스트(LIFO구조로))

    음... 분리 가용 리스트의 '분리 맞춤' 방식이
    malloc 함수에서 쓰이며,
    여러 가용 리스트를 만들어 관리하는 방식이라 함
    (특이점은 이 내부 가용 리스트를 first fit로 검색하는 것이
    best fit와 유사한 효과를 낸다고 한다)

    일단 명시적 리스트 + LIFO 를 사용해보자

    기본적으로 size 크기 자체는 신경 쓸 필요 없음
    그저 header의 위치 및
    새로 생긴 포인터 들에 대하여 잘 지정만 해주면 됨
    (size는 그대로이고, 그냥 블록의 크기가 늘어나는 거라 생각하면 된다)

    '할당 되어 있는 블록'은 연결 리스트에서 관리하지 않는다
    할당된 녀석들의 prev, next 정보는 필요하진 않는다

    명시적에서 할당 검사를 할 때,
    연결 리스트의 처음부터 순회하고
    해당 블록의 사이즈를 확인한다
    (연결 리스트 내의 블록은 '마지막'블록을 제외하곤
     이미 모두 할당히 해제된 블록들이다)
    (이때 마지막 블록은 프롤로그 블록을 가리킨다)
    (연결 리스트 적인 개념은 기본적으로 각 블록들이
     next와 prev를 가짐으로서 구현이 가능하다고 생각한다)

    유의할 점이 있다면,
    어떻게 리스트에 '추가'한다는 것을 알 수 있을까?

    요점이 있다면,
    결국 heaplist가 결국 '할당 해제 리스트'라는 점

    따라서 최근 만들어진 '할당 해제'블록만
    heaplist가 가리키며,
    가리키기 전의 블록 녀석에게 '네 이전은 누구다'라고 알려주는 점이다
    그리고 가리킨 후, 네 다음 녀석은 누구다 라고
    이런식으로 삽입하면 될듯한데

    그리고 삭제 시, place 이후에 가리키는 것이 옳게 보인다
    (place를 할 때, 현재 heaplist가 가리키고 있는 블록이
    바뀌지 않도록 유의해야 한다)
    삭제 시, 다음 노드가 비어 있는 경우는 크게 유의하지 않아도 되지만
    이전 노드가 비어있는 경우, 이전 노드의
    pred, succ에 설정해야 한다

    삭제 시에는 내 다음 노드의 '이전'은 나의 '이전'을 가리키며
    내 이전 노드의 '다음'은 내 '다음'을 가리켜야 한다

    또한 기본적으로는 PRED 로 이동하면 된다
    어차피 할당시에는 별 의미없는 공간이므로
    거기서부터 쓰면 되기에

    pred, succ 에 기본적으로
    들어가는 것은 bp의 위치(헤더 다음의 위치)

    블록의 사이즈가 본인보다 작다면, 할당할 수 없는 공간
    해당 블록의 next 포인터로 이동하여 다음 블록 검사

    블록의 사이즈가 크거나 같다면 할당 가능
    - 해당 블록을 연결 리스트에서 제거
    - 메모리가 할당될 수 있는 사이즈 공간을 블록을 분할하여 할당하고
      나머지 부분을 해제된 블록으로 바꿔준 후, 연결리스트에 추가하기
    - 메모리를 할당하고, 해당 블록의 block pointer 주소를 반환하기
      다만, 해당 사이즈가 일정 수준보다 작다면 패딩하여 추가 주소 반환

    할당할 수 있는 공간이 없어 연결리스트의 끝에 도달 시,
    힙을 확장하고 할당하기

    요점은 연결 리스트를 구현하는 점이며
    LIFO를 사용하려면
    가장 최근 할당된 녀석을 연결 리스트의 앞쪽으로 집어 넣는 점

    (LIFO 는 스택처럼 뽑게 되면,
     그 다음 녀석을 가리켜야 한다는 점도 잊지 말아야 함)

    '이중 연결 리스트'이므로
    제거하는 경우, 삭제된 블록의 전 후 를 각각 이전과 다음 노드에 넘겨줌

*/

/* Basic constants and macros */
#define WSIZE 4             /* WORD and header / footer size*/
#define DSIZE 8             /* Double Word Size*/
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) /* 하위 3비트를 제외한 녀석들이 블록의 크기 비트를 표현한다  */
#define GET_ALLOC(p) (GET(p) & 0x1)

size_t Get_Size(void* p)
{
    size_t a = (GET(p) & ~0x7);

    return a;
}

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 헤더 쪽에 위치시킨다
// 이 녀석들은 할당된 녀석에는 없어야 한다
// 어차피 이 녀석들은 할당할 때, bp에 덮어씌워야 하므로 그냥 이 위치에 쓴다
#define PREDP(bp) ((char *)(bp))
#define SUCCP(bp) ((char *)(bp) + WSIZE)

// 직접 써도 되지만, 실수 방지를 위해 define 설정한다
#define SET_PREDP(bp, targetP) (PUT(PREDP(bp), targetP))
#define SET_SUCCP(bp, targetP) (PUT(SUCCP(bp), targetP))

#define GET_PREDP(bp) (GET(PREDP(bp)))
#define GET_SUCCP(bp) (GET(SUCCP(bp)))

void* Get_SuccPtr(void* bp)
{
    char* succPtr = SUCCP(bp);

    void* resultPtr = GET(succPtr);

    return resultPtr;
}

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp)-WSIZE))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *arrageBlock(void *targetBp);

void *heap_listp = NULL;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 초기화 */
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);                            /* 정렬 패딩 */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* 프롤로그 헤더 */
    SET_PREDP(heap_listp + (2 * WSIZE), NULL);      //heap_listp + (6 * WSIZE));
    SET_SUCCP(heap_listp + (2 * WSIZE), NULL);
    PUT(heap_listp + (4 * WSIZE), PACK(DSIZE, 1)); /* 프롤로그 푸터 */
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));     /* 에필로그 헤더 */

    // 할당기의 처음은 에필로그 헤더 너머(원래는 bp가 있어야 하나)를 가리키도록 한다
    heap_listp += (2 * WSIZE);

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
    // SET_PREDP(bp, NULL); // 현재 시점 bp이지만 일단 초기화는 해준다
    // SET_SUCCP(bp, heap_listp);
    // SET_PREDP(heap_listp,bp);

    PUT(FTRP(bp), PACK(size, 0));

    //heap_listp = bp;

    /* 새로운 에필로그 헤더 생성*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

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
    /* first fit*/
    void *bp;

    // 정확히는 
    // list_heap 은 가장 처음의 헤더를 가리켜야 한다
    // 그렇기에 일단 list_heap자체가
    // 할당된 블록을 가리키는 일 자체가 없어야 한다
    for (bp = heap_listp;
     bp != NULL && Get_Size(HDRP(bp)) > 0; 
     bp = Get_SuccPtr(bp))
    {
        /* 헤더 확인하니 가용 상태, 해당 블록 사이즈가 asize보다 큼 */
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            arrageBlock(bp); // 할당 되었기에 연결 된 리스트들을 정리한다
            return bp;
        }
    }

    /* 할당 안됨 */
    return NULL;
}

/* 요청한 블록을 가용 블록의 시작 부분에 배치,
나머지 부분의 크기가 최소 블록 크기와 같거나 큰 경우에만 분할 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    /* 할당 사이즈(asize)가 시작 블록 사이즈(cSize)보다 16바이트(최소 할당 크기)보다 큰 경우*/
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        /* 불필요한 할당 인 경우, 잘라낸다 */
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        SET_PREDP(bp,NULL);
        SET_SUCCP(bp,heap_listp);
        SET_PREDP(heap_listp,bp);
        PUT(FTRP(bp), PACK(csize - asize, 0));
        heap_listp = bp;
    }
    else /* 할당 사이즈와 시작 블록 사이즈의 차이가, 최소 할당크기보다 작음 */
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        heap_listp = GET_SUCCP(bp); // 이전 bp로 이동한다
    }
}

static void *arrageBlock(void *targetBp)
{
    // 따라서 해당 블록의 이전은 '다음 블록'의 이전으로 보내고
    // 해당 블록의 '다음'은 '이전 블록'의 다음으로 보낸다

    void* predBp = GET_PREDP(targetBp);
    void* succBp = GET_SUCCP(targetBp);

    // 둘 중 하나가 비어 있다면 null이 들어간다
    if (succBp != NULL)
    {
        SET_PREDP(succBp, predBp);
    }

    if (predBp != NULL)
    {
        SET_SUCCP(predBp, succBp);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // 방금 막 할당 해제된 녀석에서
    // arrangeBlock으로 해제하면
    // 값이 이상해지지 않을까?
    coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        /* case 1 이전과 다음이 모두 할당된 블록이다 */

        //return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        /* case 2 이전 블록은 할당되었고, 다음 블록은 가용상태 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); /* 다음 블록의 사이즈만큼 더함 (다음 헤더에서 가지고 온다) */
        PUT(HDRP(bp), PACK(size, 0));          /*size | 0 은 size와 같음, 그러나 가독성과 통일성을 위해 사용*/
        PUT(FTRP(bp), PACK(size, 0));

        // 다음 블록이 가용 상태라면 pred와 succ 정리해준다
        arrageBlock(NEXT_BLKP(bp));
    }
    else if (!prev_alloc && next_alloc)
    {
        /* case 3 이전 블록은 가용 상태, 다음 블록은 할당상태 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); /* 다음 블록의 사이즈만큼 더함 (다음 헤더에서 가지고 온다) */
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); /* 이전 블록과 자신을 통합해야 하기에, 이전 블록의 헤더에 사이즈 변경 */
        arrageBlock(bp);
        bp = PREV_BLKP(bp);                      /* 이전 블록으로 bp를 옮긴다 */
    }
    else
    {
        /* case 4 이전 블록과 다음 블록 모두 가용 상태*/
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); /* 다음 블록의 footer에 값을 써줌 */
        arrageBlock(NEXT_BLKP(bp));
        arrageBlock(bp);
        bp = PREV_BLKP(bp);                      /* 이전 블록으로 bp를 옮긴다 */
    }

    SET_PREDP(bp,NULL);
    SET_SUCCP(bp,heap_listp);
    SET_PREDP(heap_listp,bp);
    heap_listp = bp;

    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
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
