// mm-naive.c - 가장 빠르지만 메모리 효율은 가장 낮은 malloc 패키지.
// 이 단순한(naive) 방식에서는 brk 포인터를 단순히 증가시키는 것만으로 블록을 할당합니다.
// 블록은 순수 페이로드(payload)로만 구성되며, 헤더나 푸터가 없습니다.
// 블록의 병합(coalescing)이나 재사용은 이루어지지 않습니다.
// Realloc은 mm_malloc과 mm_free를 직접 사용하여 구현됩니다.
// 학생 참고 사항: 이 헤더 주석을 자신의 솔루션에 대한 개괄적인 설명을 담은
// 주석으로 교체하십시오.

// mem_init 함수는
// 힙에 가용한 가상메모리를 큰 더블 워드로 정렬된 바이트의 배열로 모델한 것
// mem_heap과 mem_brk 사이의 바이트들은 할당된 가상메모리를 나타낸다

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    // 팀 이름
    "ateam",

    // 첫 번째 팀원 전체 이름
    "Harry Bovik",

    // 첫 번째 팀원 이메일 주소
    "bovik@cs.cmu.edu",

    // 두 번째 팀원 전체 이름
    "",

    // 두 번째 팀원 이메일 주소
    ""
};

// 싱글 워드(4) 또는 더블 워드(8) 정렬
#define ALIGNMENT 8

// ALIGNMENT의 가장 가까운 배수로 올림
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// size_t의 크기를 8바이트 단위로 정렬
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 워드와 헤더/푸터의 크기 = 4바이트
#define WSIZE 4

// 더블 워드의 크기 = 8바이트
#define DSIZE 8

// 힙을 한 번에 늘릴 크기 = 4096바이트
#define CHUNKSIZE (1 << 12)

// 두 값 중 더 큰 값을 반환
#define MAX(x, y) ((x) > (y) ? (x) : (y))

// 블록 크기와 할당 여부를 하나의 값으로 합침
#define PACK(size, alloc) ((size) | (alloc))

// 주소 p에서 4바이트 값을 읽음
#define GET(p) (*(unsigned int *)(p))

// 주소 p에 val 값을 4바이트로 저장
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// 헤더/푸터에서 블록의 크기를 가져옴
#define GET_SIZE(p) (GET(p) & ~0x7)

// 헤더/푸터에서 블록의 할당 여부를 가져옴
#define GET_ALLOC(p) (GET(p) & 0x1)

// 블록 포인터 bp를 이용해 헤더의 주소를 계산
#define HDRP(bp) ((char *)(bp) - WSIZE)

// 블록 포인터 bp를 이용해 푸터의 주소를 계산
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 블록 포인터 bp를 이용해 다음 블록의 주소를 계산
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))

// 블록 포인터 bp를 이용해 이전 블록의 주소를 계산
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// 힙의 시작 위치를 가리키는 포인터
static char *heap_listp;

// words 단위로 힙을 확장하고 새로운 가용 블록을 만드는 함수
static void *extend_heap(size_t words);

// 현재 블록과 인접한 가용 블록을 하나로 합치는 함수
static void *coalesce(void *bp);

// 가용 블록 중에서 요청한 크기에 맞는 블록을 찾는 함수
static void *find_fit(size_t asize);

// 찾은 가용 블록에 메모리를 할당하고 필요한 경우 블록을 나누는 함수
static void place(void *bp, size_t asize);


// mm_init - malloc 패키지 초기화
// 초기 힙을 생성하고 초기화하는 함수
int mm_init(void)
{
    // 초기 힙에 4개의 워드(16바이트) 공간을 확보
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    // 정렬을 위한 패딩
    PUT(heap_listp, 0);

    // 프롤로그 블록의 헤더를 생성
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));

    // 프롤로그 블록의 푸터를 생성
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));

    // 에필로그 블록의 헤더를 생성
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));

    // heap_listp를 프롤로그 블록의 payload 위치로 이동
    heap_listp += (2 * WSIZE);

    // CHUNKSIZE만큼 빈 블록을 추가하여 힙을 확장
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    // 초기화 성공
    return 0;
}


// 힙을 words만큼 확장하고 새로운 가용 블록을 만드는 함수
static void *extend_heap(size_t words)
{
    // 새로 만든 블록의 시작 주소를 저장
    char *bp;

    // 새로 확보할 블록의 크기를 저장
    size_t size;

    // 8바이트 정렬을 유지하기 위해 짝수 개의 워드로 크기를 맞춤
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    // size 바이트만큼 힙을 확장하고 새 영역의 시작 주소를 저장
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // 새 가용 블록의 헤더에 크기와 가용 상태를 저장
    PUT(HDRP(bp), PACK(size, 0));

    // 새 가용 블록의 푸터에 크기와 가용 상태를 저장
    PUT(FTRP(bp), PACK(size, 0));

    // 새 가용 블록 뒤에 에필로그 헤더를 생성
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // 이전 블록이 가용 상태라면 두 블록을 하나로 합침
    return coalesce(bp);
}


// mm_malloc - 요청한 크기의 메모리를 할당하는 함수
void *mm_malloc(size_t size)
{
    // 헤더와 푸터, 정렬을 포함한 실제 블록 크기
    size_t asize;

    // 가용 블록이 없을 때 힙을 확장할 크기
    size_t extendsize;

    // 할당할 블록의 시작 주소
    char *bp;

    // 요청 크기가 0이면 할당하지 않음
    if (size == 0)
        return NULL;

    // 헤더와 푸터 공간을 포함하고 8바이트 단위로 정렬
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

    // 가용 블록 중에서 요청 크기에 맞는 블록을 찾음
    if ((bp = find_fit(asize)) != NULL)
    {
        // 찾은 가용 블록에 메모리를 할당
        place(bp, asize);

        // 할당된 블록의 시작 주소를 반환
        return bp;
    }

    // 맞는 가용 블록이 없으면 힙을 확장할 크기를 결정
    extendsize = MAX(asize, CHUNKSIZE);

    // 결정한 크기만큼 힙을 확장
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;

    // 새로 확보한 가용 블록에 메모리를 할당
    place(bp, asize);

    // 할당된 블록의 시작 주소를 반환
    return bp;
}


// mm_free - 블록을 해제하는 함수
void mm_free(void *bp)
{
    // 현재 블록의 전체 크기를 가져옴
    size_t size = GET_SIZE(HDRP(bp));

    // 현재 블록의 헤더를 가용 상태로 변경
    PUT(HDRP(bp), PACK(size, 0));

    // 현재 블록의 푸터를 가용 상태로 변경
    PUT(FTRP(bp), PACK(size, 0));

    // 앞뒤의 가용 블록과 병합
    coalesce(bp);
}


// 현재 블록과 인접한 가용 블록을 하나로 합치는 함수
static void *coalesce(void *bp)
{
    // 이전 블록의 가용 여부를 확인
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));

    // 다음 블록의 가용 여부를 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    // 현재 블록의 전체 크기를 가져옴
    size_t size = GET_SIZE(HDRP(bp));

    // Case 1: 이전과 다음 블록이 모두 할당된 경우
    if (prev_alloc && next_alloc)
    {
        return bp;
    }

    // Case 2: 이전 블록은 할당, 다음 블록은 가용인 경우
    else if (prev_alloc && !next_alloc)
    {
        // 다음 블록의 크기를 현재 블록 크기에 더함
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        // 합쳐진 블록의 헤더를 갱신
        PUT(HDRP(bp), PACK(size, 0));

        // 합쳐진 블록의 푸터를 갱신
        PUT(FTRP(bp), PACK(size, 0));
    }

    // Case 3: 이전 블록은 가용, 다음 블록은 할당된 경우
    else if (!prev_alloc && next_alloc)
    {
        // 이전 블록의 크기를 현재 블록 크기에 더함
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        // 합쳐진 블록의 푸터를 갱신
        PUT(FTRP(bp), PACK(size, 0));

        // 합쳐진 블록의 헤더를 이전 블록의 헤더에 기록
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        // bp를 합쳐진 블록의 시작 위치로 이동
        bp = PREV_BLKP(bp);
    }

    // Case 4: 이전과 다음 블록이 모두 가용인 경우
    else
    {
        // 이전 블록과 다음 블록의 크기를 현재 블록에 더함
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(HDRP(NEXT_BLKP(bp)));

        // 합쳐진 블록의 헤더를 이전 블록의 헤더에 기록
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        // 합쳐진 블록의 푸터를 다음 블록의 푸터에 기록
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        // bp를 합쳐진 블록의 시작 위치로 이동
        bp = PREV_BLKP(bp);
    }

    // 합쳐진 블록의 시작 주소를 반환
    return bp;
}


// mm_realloc - mm_malloc과 mm_free를 사용하여 간단하게 구현
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    // 새로운 크기의 블록을 할당
    newptr = mm_malloc(size);

    // 할당에 실패하면 NULL 반환
    if (newptr == NULL)
        return NULL;

    // 기존 블록의 헤더에 저장된 크기를 가져옴
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);

    // 요청한 크기보다 기존 데이터 크기가 크면 요청 크기만큼만 복사
    if (size < copySize)
        copySize = size;

    // 기존 블록의 데이터를 새로운 블록으로 복사
    memcpy(newptr, oldptr, copySize);

    // 기존 블록을 해제
    mm_free(oldptr);

    // 새로운 블록 반환
    return newptr;
}


// 가용 블록 중에서 요청한 크기에 맞는 블록을 찾는 함수 (First Fit)
static void *find_fit(size_t asize)
{
    // 현재 탐색 중인 블록
    void *bp;

    // 힙의 처음부터 에필로그 블록(크기 0)을 만날 때까지 탐색
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        // 가용 상태이고 요청한 크기 이상이면 바로 반환 (First Fit)
        if (!GET_ALLOC(HDRP(bp)) &&
            (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }

    // 맞는 블록이 없으면 NULL 반환
    return NULL;
}


// 찾은 가용 블록에 메모리를 할당하고 필요한 경우 블록을 나누는 함수
static void place(void *bp, size_t asize)
{
    // 현재 블록의 크기를 알아냄
    size_t csize = GET_SIZE(HDRP(bp));

    // 남은 공간이 충분히 클 경우 블록을 나눔
    if ((csize - asize) >= (2 * DSIZE))
    {
        // 사용할 블록의 헤더에 크기와 할당 상태를 저장
        PUT(HDRP(bp), PACK(asize, 1));

        // 사용할 블록의 푸터에 크기와 할당 상태를 저장
        PUT(FTRP(bp), PACK(asize, 1));

        // 나머지 블록으로 포인터 이동
        bp = NEXT_BLKP(bp);

        // 나머지 블록의 헤더에 크기와 가용 상태를 저장
        PUT(HDRP(bp), PACK(csize - asize, 0));

        // 나머지 블록의 푸터에 크기와 가용 상태를 저장
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else
    {
        // 현재 블록 전체를 할당 상태로 변경
        PUT(HDRP(bp), PACK(csize, 1));

        // 현재 블록 전체를 할당 상태로 변경
        PUT(FTRP(bp), PACK(csize, 1));
    }
}