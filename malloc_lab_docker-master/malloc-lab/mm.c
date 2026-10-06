
//mm-naive.c - 가장 빠르지만 메모리 효율은 가장 낮은 malloc 패키지.
//이 단순한(naive) 방식에서는 brk 포인터를 단순히 증가시키는 것만으로 블록을 할당합니다.
//블록은 순수 페이로드(payload)로만 구성되며, 헤더나 푸터가 없습니다.
//블록의 병합(coalescing)이나 재사용은 이루어지지 않습니다.
//Realloc은 mm_malloc과 mm_free를 직접 사용하여 구현됩니다.
//학생 참고 사항: 이 헤더 주석을 자신의 솔루션에 대한 개괄적인 설명을 담은
//주석으로 교체하십시오.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
     //팀 이름
    "ateam",
     //첫 번째 팀원 전체 이름
    "Harry Bovik",
     //첫 번째 팀원 이메일 주소
    "bovik@cs.cmu.edu",
     //두 번째 팀원 전체 이름 (없으면 비워둘 것)
    "",
     //두 번째 팀원 이메일 주소 (없으면 비워둘 것)
    ""
};

 //싱글 워드(4) 또는 더블 워드(8) 정렬
#define ALIGNMENT 8

 //ALIGNMENT의 가장 가까운 배수로 올림
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

 
// mm_init - malloc 패키지 초기화.

int mm_init(void)
{
    return 0;
}

 
//mm_malloc - brk 포인터를 증가시켜 블록 할당.
//    항상 정렬(alignment)의 배수인 크기를 가진 블록을 할당합니다.

void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}


// mm_free - 블록을 해제해도 아무런 동작을 수행하지 않습니다.

void mm_free(void *ptr)
{
}


// mm_realloc - mm_malloc과 mm_free를 사용하여 간단하게 구현되었습니다.

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}