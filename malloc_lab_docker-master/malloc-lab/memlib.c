
// memlib.c - 메모리 시스템을 시뮬레이션하는 모듈. 
//            학생이 구현한 malloc 패키지의 호출과 libc에 있는 
//            시스템 malloc 패키지의 호출을 혼용하여 사용할 수 있게 해줍니다.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

 private 변수
static char *mem_start_brk;   // 힙의 시작 주소
static char *mem_brk;         // 힙의 마지막 바이트를 가리킴
static char *mem_max_addr;    // 힙의 최대 한계

 
// mem_init - 메모리 시스템 모델 초기화
        // void mem_init(void)
        // {
        //     //가용 가상 메모리(VM)를 모델링하는 데 사용할 저장 공간 할당
        //     if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
        // 	fprintf(stderr, "mem_init_vm: malloc error\n");
        // 	exit(1);
        //     }

        //     mem_max_addr = mem_start_brk + MAX_HEAP;   //유효한 최대 힙 주소
        //     mem_brk = mem_start_brk;                   //초기에는 힙이 비어 있음
        // }

        // MAX_HEAP : 힙의 최대 크기
    void mem_init(void)
    {
        // 최대 힙 크기만큼 메모리를 할당하고 힙의 시작 주소를 저장
        mem_start_brk = (char *)Malloc(MAX_HEAP);

        // 현재 힙의 끝을 힙의 시작 위치로 설정
        mem_brk = (char *)mem_start_brk;

        // 힙에서 사용할 수 있는 최대 주소의 다음 위치를 저장
        mem_max_addr = (char *)(mem_start_brk + MAX_HEAP);
    }


 
// mem_deinit - 메모리 시스템 모델이 사용한 저장 공간 해제
void mem_deinit(void)
{
    free(mem_start_brk);
}


// mem_reset_brk - 시뮬레이션된 brk 포인터를 재설정하여 빈 힙 상태로 만듦
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

 
// mem_sbrk - sbrk 함수의 간단한 모델. incr 바이트만큼 힙을 확장하고
//    새로운 영역의 시작 주소를 반환합니다. 이
//    모델에서는 힙을 축소할 수 없습니다.
void *mem_sbrk(int incr)
{
    // 힙을 늘리기 전 현재 끝 주소를 저장
    char *old_brk = mem_brk;

    // incr이 음수이거나 힙의 최대 크기를 넘어가면 실패
    if ((incr < 0) || ((mem_brk + incr) > mem_max_addr)) {

        // 메모리가 부족하다는 오류를 저장
        errno = ENOMEM;

        // 오류 메시지 출력
        fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");

        // 실패를 의미하는 -1 반환
        return (void *)-1;
    }

    // 힙의 끝 위치를 incr 바이트만큼 이동
    mem_brk += incr;

    // 늘어나기 전 힙의 끝 주소를 새 영역의 시작 주소로 반환
    return (void *)old_brk;
}


// mem_heap_lo - 힙의 첫 번째 바이트 주소 반환
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

 
// mem_heap_hi - 힙의 마지막 바이트 주소 반환
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}


// mem_heapsize() - 힙 크기를 바이트 단위로 반환
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}


// mem_pagesize() - 시스템의 페이지 크기 반환
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}