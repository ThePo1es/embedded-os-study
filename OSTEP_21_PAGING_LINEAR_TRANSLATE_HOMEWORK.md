# OSTEP 21장 숙제 해설: `paging-linear-translate.py` 완전 분석

이 문서는 OSTEP 21장(페이징 개요) 숙제의 `paging-linear-translate.py` 문제를 처음 보는 사람도 따라올 수 있도록, 실행 결과 + 수식 + 코드 근거까지 포함해 정리한 상세 해설이다.

## 0) 참고 원문 / 소스

- 숙제 목록: https://pages.cs.wisc.edu/~remzi/OSTEP/Homework/homework.html
- 시뮬레이터 원본: https://github.com/remzi-arpacidusseau/ostep-homework/blob/master/vm-paging/paging-linear-translate.py

## 1) 시뮬레이터가 실제로 하는 일 (코드 기준)

`paging-linear-translate.py`는 선형 페이지 테이블(linear page table)을 만들고, 가상 주소를 물리 주소로 변환하는 과정을 출력한다.

핵심 동작:

1. 입력 크기 파싱 (`k`, `m`, `g`)
1. 제약 검증
1. 페이지 테이블 생성
1. 가상 주소 트레이스 생성/변환

핵심 제약:

- `physical memory size > address space size` 여야 한다.
- `asize % pagesize == 0`
- `psize % pagesize == 0`
- `asize`와 `pagesize`는 2의 거듭제곱
- `asize`, `psize`는 1GB 미만

이 제약 때문에 실제 OS와는 다르게 보일 수 있다. (현실 OS는 `asize > psize`도 가능하고 demand paging으로 해결)

## 2) 선형 페이지 테이블 계산 공식

기호:

- `A`: address space size
- `P`: page size
- `M`: physical memory size

공식:

- 가상 페이지 수(VPN 개수) = `A / P`
- 물리 프레임 수(PFN 개수) = `M / P`
- 페이지 테이블 엔트리 수(PTE 수) = `A / P`
- 페이지 테이블 메모리 오버헤드 = `(A / P) * sizeof(PTE)`

이 시뮬레이터는 32비트 정수 엔트리를 출력(`0xXXXXXXXX`)하므로 보통 `sizeof(PTE)=4B`로 본다.

## 3) Q1 해설: 주소공간/페이지크기 변화에 따른 페이지 테이블 크기

### Q1-1 주소공간 증가 (`P=1K` 고정)

실행:

```bash
python3 paging-linear-translate.py -P 1K -a 1m -p 512m -v -n 0
python3 paging-linear-translate.py -P 1K -a 2m -p 512m -v -n 0
python3 paging-linear-translate.py -P 1K -a 4m -p 512m -v -n 0
```

결과 요약(동일 seed=0 기본값 기준):

| Command | PTE 수 | 테이블 크기(4B/PTE 가정) | valid | invalid |
|---|---:|---:|---:|---:|
| `-a 1m` | 1024 | 4096 B (4 KiB) | 519 | 505 |
| `-a 2m` | 2048 | 8192 B (8 KiB) | 1035 | 1013 |
| `-a 4m` | 4096 | 16384 B (16 KiB) | 2062 | 2034 |

해석:

- 주소공간을 2배로 키우면 VPN 수가 2배, PTE 수도 2배.
- 즉 선형 페이지 테이블 크기는 주소공간에 선형 비례한다.

### Q1-2 페이지 크기 증가 (`A=1M` 고정)

실행:

```bash
python3 paging-linear-translate.py -P 1k -a 1m -p 512m -v -n 0
python3 paging-linear-translate.py -P 2k -a 1m -p 512m -v -n 0
python3 paging-linear-translate.py -P 4k -a 1m -p 512m -v -n 0
```

결과 요약:

| Command | PTE 수 | 테이블 크기(4B/PTE 가정) | valid | invalid |
|---|---:|---:|---:|---:|
| `-P 1k` | 1024 | 4096 B (4 KiB) | 519 | 505 |
| `-P 2k` | 512 | 2048 B (2 KiB) | 259 | 253 |
| `-P 4k` | 256 | 1024 B (1 KiB) | 133 | 123 |

해석:

- 페이지 크기를 2배로 키우면 페이지 수는 절반.
- 선형 페이지 테이블 크기도 절반으로 감소.

### Q1-3 큰 페이지를 무조건 쓰면 안 되는 이유

- 내부 단편화 증가: 실제 사용량보다 큰 페이지를 통째로 잡아 낭비
- 보호/공유 단위가 거칠어짐: 세밀한 권한 관리 어렵다
- I/O/캐시/TLB 관점에서 워크로드별 성능 트레이드오프 존재

요약: 페이지를 크게 하면 테이블 오버헤드는 줄지만, 메모리 효율과 관리 정밀도가 나빠질 수 있다.

## 4) Q2 해설: `-u`(주소공간 사용률) 변화

실행:

```bash
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 0
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 25
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 50
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 75
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 100
```

조건:

- `A=16K`, `P=1K` 이므로 총 가상 페이지 16개

결과 요약:

| `-u` | 총 PTE | valid | invalid |
|---:|---:|---:|---:|
| 0 | 16 | 0 | 16 |
| 25 | 16 | 6 | 10 |
| 50 | 16 | 9 | 7 |
| 75 | 16 | 16 | 0 |
| 100 | 16 | 16 | 0 |

해석:

- `-u`를 올릴수록 valid 페이지 비중이 커진다.
- 따라서 랜덤 가상 주소 변환 시 invalid(페이지 미할당) 비율이 감소한다.
- 단, 페이지 수가 작을수록(여기선 16개) 난수 편차가 크기 때문에 `75%`에서도 운 좋게 전부 valid가 나올 수 있다.

추가 확인(`-c`, 5개 주소 변환):

| `-u` | invalid 번역 개수(5개 중) |
|---:|---:|
| 0 | 5 |
| 25 | 4 |
| 50 | 2 |
| 75 | 0 |
| 100 | 0 |

## 5) Q3 해설: 랜덤 조합 중 비현실적인 것

실행:

```bash
python3 paging-linear-translate.py -P 8 -a 32 -p 1024 -v -s 1
python3 paging-linear-translate.py -P 8K -a 32k -p 1m -v -s 2
python3 paging-linear-translate.py -P 1m -a 256m -p 512m -v -s 3
```

실행 결과 요약:

| 조합 | PTE 수 | valid | invalid | 현실성 평가 |
|---|---:|---:|---:|---|
| `-P 8 -a 32 -p 1024` | 4 | 1 | 3 | 매우 비현실적 (페이지 8바이트) |
| `-P 8K -a 32k -p 1m` | 4 | 2 | 2 | 실습용으로 무난 |
| `-P 1m -a 256m -p 512m` | 256 | 120 | 136 | 일반 목적 기본 페이지로는 비현실적 |

이유:

- `P=8B`: 페이지 관리 메타데이터/TLB/OS 오버헤드 관점에서 사실상 불가능한 수준
- `P=1MB`: 페이지 테이블은 작아지지만 내부 단편화가 매우 커지고 세밀한 매핑/보호가 어려움

## 6) Q4 해설: 프로그램이 실패하는 경계 조건 찾기

아래는 실제로 실패시켜 본 케이스와 에러 메시지다.

### 6-1 주소공간 >= 물리메모리

```bash
python3 paging-linear-translate.py -P 1k -a 64k -p 32k
```

출력:

```text
Error: physical memory size must be GREATER than address space size (for this simulation)
```

### 6-2 주소공간이 페이지 크기의 배수가 아님

```bash
python3 paging-linear-translate.py -P 6k -a 16k -p 64k
```

출력:

```text
Error in argument: address space must be a multiple of the pagesize
```

### 6-3 물리메모리가 페이지 크기의 배수가 아님

```bash
python3 paging-linear-translate.py -P 4k -a 16k -p 66k
```

출력:

```text
Error in argument: physical memory must be a multiple of the pagesize
```

### 6-4 주소공간/페이지크기가 2의 거듭제곱이 아님

```bash
python3 paging-linear-translate.py -P 1k -a 24k -p 64k
```

출력:

```text
Error in argument: address space must be a power of 2
```

### 6-5 1GB 이상 입력

```bash
python3 paging-linear-translate.py -P 4k -a 1g -p 2g
```

출력:

```text
Error: must use smaller sizes (less than 1 GB) for this simulation.
```

## 7) 최종 결론 (한 줄 요약 4개)

1. 선형 페이지 테이블 크기는 `A/P`에 비례한다.
1. 주소공간이 커지면 테이블은 선형으로 커진다.
1. 페이지를 키우면 테이블은 줄지만 내부 단편화/관리 정밀도 손해가 따른다.
1. 이 시뮬레이터는 교육용 단순화 모델이라 실제 OS보다 강한 제약(`M > A`)을 둔다.

## 8) 재현 커맨드 모음

아래 순서대로 실행하면 이 문서의 핵심 숫자를 재현할 수 있다.

```bash
# Q1
python3 paging-linear-translate.py -P 1K -a 1m -p 512m -v -n 0
python3 paging-linear-translate.py -P 1K -a 2m -p 512m -v -n 0
python3 paging-linear-translate.py -P 1K -a 4m -p 512m -v -n 0
python3 paging-linear-translate.py -P 1k -a 1m -p 512m -v -n 0
python3 paging-linear-translate.py -P 2k -a 1m -p 512m -v -n 0
python3 paging-linear-translate.py -P 4k -a 1m -p 512m -v -n 0

# Q2
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 0
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 25
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 50
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 75
python3 paging-linear-translate.py -P 1k -a 16k -p 32K -v -u 100

# Q3
python3 paging-linear-translate.py -P 8 -a 32 -p 1024 -v -s 1
python3 paging-linear-translate.py -P 8K -a 32k -p 1m -v -s 2
python3 paging-linear-translate.py -P 1m -a 256m -p 512m -v -s 3
```
