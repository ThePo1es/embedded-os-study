#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// 64KB 물리 메모리 배열 (하드웨어 메모리 모사)
#define PHYS_MEM_SIZE (64 * 1024)

// 전역으로 64KB 물리 메모리 선언
uint8_t physical_memory[PHYS_MEM_SIZE];

/*
 * TrustZone Address Space Controller (TZASC) 설정 구조체
 * base: 보호(격리)할 영역의 시작 물리 주소
 * bounds: 보호(격리)할 영역의 크기 (한계점)
 */
typedef struct {
    unsigned int base;
    unsigned int bounds;
} TZASC;

/*
 * 가상 주소 -> 물리 주소 변환 및 권한 검증 (Hardware MMU 모사)
 */
int translate(TZASC config, unsigned int v_addr) {
    // 1. Bounds check (가상 주소가 자신에게 할당된 한계치 내에 있는지 확인)
    if (v_addr >= config.bounds) {
        printf("[Fault] 보안 폴트 발생! Address %u is Out of Bounds! (허용 크기: %u)\n", v_addr, config.bounds);
        exit(1); // 예외(Exception) 발생을 모사하여 즉시 종료
    }
    
    // 2. Base addition (물리 주소 = base + v_addr)
    unsigned int p_addr = config.base + v_addr;
    
    // 혹시라도 전체 물리 메모리를 초과하는 경우를 대비한 안전 장치
    if (p_addr >= PHYS_MEM_SIZE) {
        printf("[Fault] 하드웨어 폴트! OOM (물리 메모리 64KB 한계 초과): %u\n", p_addr);
        exit(1);
    }
    
    return p_addr;
}

int main() {
    // 물리 데이터 메모리 0으로 초기화
    memset(physical_memory, 0, PHYS_MEM_SIZE);

    /* 
     * Secure Partition (고립 영역) 환경 설정 
     * Base: 32768 (32KB 지점)
     * Bounds: 16384 (16KB 크기)
     * 역할: 해당 프로세스/세계는 32KB 지점부터 최대 16KB까지만 접근 가능하도록 한정됨
     */
    TZASC secure_partition = {32768, 16384};

    printf("\n=== Base & Bounds 격리 시뮬레이터 ===\n");
    printf(">> Secure 영역 Base: %u, Bounds: %u\n\n", secure_partition.base, secure_partition.bounds);

    // [Test 1] 정상 접근 (Bounds 범위 안)
    unsigned int v_addr1 = 100;
    unsigned int p_addr1 = translate(secure_partition, v_addr1);
    printf("[Success] Virtual Addr: %u -> Physical Addr: %u\n", v_addr1, p_addr1);

    // [Test 2] 정상 접근 (아슬아슬하게 통과하는 끝단 주소)
    unsigned int v_addr2 = 16383;
    unsigned int p_addr2 = translate(secure_partition, v_addr2);
    printf("[Success] Virtual Addr: %u -> Physical Addr: %u\n", v_addr2, p_addr2);

    // [Test 3] 비정상 접근 (Bounds 범위를 넘어감)
    unsigned int v_addr3 = 20000;
    printf("\n[Attempt] Virtual Addr: %u 접근 시도 중...\n", v_addr3);
    
    // 이 시점에서 Fault가 발생하며 프로그램은 종료됨
    unsigned int p_addr3 = translate(secure_partition, v_addr3);
    
    // 예외 발생으로 아래 코드는 원칙적으로 실행되지 않아야 함
    printf("[Success] Virtual Addr: %u -> Physical Addr: %u\n", v_addr3, p_addr3);

    return 0;
}
