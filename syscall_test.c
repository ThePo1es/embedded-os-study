#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

// 100만 번 루프를 돌아 1회당 평균 시간을 계산함
#define NLOOPS 1000000

/*
 * 시스템 전역의 Wall Clock Time을 사용하는 gettimeofday를 이용한 getpid() 호출 비용 측정
 */
double measure_getpid_gettimeofday() {
    struct timeval start, end;
    
    // 측정 시작 시간 기록
    gettimeofday(&start, NULL);
    
    // NLOOPS 만큼 getpid() 호출 (getpid는 인자가 없어 순수 Context Switch 오버헤드 측정에 유리)
    for (int i = 0; i < NLOOPS; i++) {
        getpid();
    }
    
    // 측정 종료 시간 기록
    gettimeofday(&end, NULL);
    
    // (종료 - 시작) 시간을 마이크로초(us) 단위로 변환
    long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    
    // 평균 비용 반환 (1회당 소요되는 마이크로초)
    return (double)diff / NLOOPS;
}

/*
 * 단조 증가하는 시간(CLOCK_MONOTONIC)을 사용하는 clock_gettime을 이용한 getpid() 호출 비용 측정
 * (해상도가 나노초 ns 단위이므로 더 정밀한 편)
 */
double measure_getpid_clock_gettime() {
    struct timespec start, end;
    
    // 측정 시작 시간 기록
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // getpid() 100만 회 호출
    for (int i = 0; i < NLOOPS; i++) {
        getpid();
    }
    
    // 측정 종료 시간 기록
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // (종료 - 시작) 시간을 마이크로초(us) 단위로 변환 후, 나노초 차이도 마이크로초로 변환해 더함
    long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    
    return (double)diff / NLOOPS;
}

/*
 * clock_gettime을 이용한 0바이트 read() 시스템 콜 비용 측정
 * getpid()와 달리 파일 디스크립터 검증 등의 추가 커널 로직(VFS 계층 접근 등)이 포함됨
 */
double measure_read_clock_gettime(int fd) {
    struct timespec start, end;
    
    // 측정 시작 시간 기록
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 읽기 크기를 0으로 주어 I/O 동작(하드 디스크 접근)을 배제하고 VFS 처리 및 Context Switch 시간만 순수하게 측정
    for (int i = 0; i < NLOOPS; i++) {
        read(fd, NULL, 0);
    }
    
    // 측정 종료 시간 기록
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // 마이크로초 단위로 변환
    long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    
    return (double)diff / NLOOPS;
}

int main() {
    // /dev/null 장치를 읽기 전용으로 열어 read 시스템 콜의 인자로 넘길 fd(파일 디스크립터) 확보
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
        perror("open");  // 장치를 여는 데 실패할 경우 에러 출력 후 종료
        exit(1);
    }

    // 캐시 워밍업 및 평균을 내기 위한 5번 전체 측정 세트
    int n_runs = 5;
    
    printf("=== 시스템 콜 오버헤드 측정 ===\n");
    printf("반복 횟수: %d회, 각 측정당 루프: %d회\n\n", n_runs, NLOOPS);

    // 각 방식별 총합 저장 변수
    double sum1 = 0, sum2 = 0, sum3 = 0;

    for (int run = 1; run <= n_runs; run++) {
        // 측정 진행
        double cost1 = measure_getpid_gettimeofday();
        double cost2 = measure_getpid_clock_gettime();
        double cost3 = measure_read_clock_gettime(fd);
        
        printf("[Run %d] getpid() w/ gettimeofday: %.5f us\n", run, cost1);
        printf("[Run %d] getpid() w/ clock_gettime: %.5f us\n", run, cost2);
        printf("[Run %d] read() w/ clock_gettime:   %.5f us\n", run, cost3);
        printf("----------------------------------------\n");

        sum1 += cost1;
        sum2 += cost2;
        sum3 += cost3;
    }

    // 최종 5회 측정 평균 결과 산출
    printf("\n=== 평균 오버헤드 결과 (5회 평균) ===\n");
    printf("1. getpid() [gettimeofday 기점]:  %.5f us\n", sum1 / n_runs);
    printf("2. getpid() [clock_gettime 기점]: %.5f us\n", sum2 / n_runs);
    printf("3. read()   [clock_gettime 기점]: %.5f us\n", sum3 / n_runs);

    // 사용이 끝난 파일 디스크립터 반환
    close(fd);
    return 0;
}
