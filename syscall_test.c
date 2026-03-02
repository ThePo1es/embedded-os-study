#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define NLOOPS 1000000

double measure_getpid_gettimeofday() {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int i = 0; i < NLOOPS; i++) {
        getpid();
    }
    gettimeofday(&end, NULL);
    long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    return (double)diff / NLOOPS;
}

double measure_getpid_clock_gettime() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NLOOPS; i++) {
        getpid();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    return (double)diff / NLOOPS;
}

double measure_read_clock_gettime(int fd) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NLOOPS; i++) {
        read(fd, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    return (double)diff / NLOOPS;
}

int main() {
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    int n_runs = 5;
    
    printf("=== 시스템 콜 오버헤드 측정 ===\n");
    printf("반복 횟수: %d회, 각 측정당 루프: %d회\n\n", n_runs, NLOOPS);

    double sum1 = 0, sum2 = 0, sum3 = 0;

    for (int run = 1; run <= n_runs; run++) {
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

    printf("\n=== 평균 오버헤드 결과 (5회 평균) ===\n");
    printf("1. getpid() [gettimeofday 기점]:  %.5f us\n", sum1 / n_runs);
    printf("2. getpid() [clock_gettime 기점]: %.5f us\n", sum2 / n_runs);
    printf("3. read()   [clock_gettime 기점]: %.5f us\n", sum3 / n_runs);

    close(fd);
    return 0;
}
