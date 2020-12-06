# SSU_OS-2020-Fall-Semester

## 			1. Linux의 명령어 실행을 통한 운영체제 기본 개념 이해

### - /proc 파일 시스템의 이해

```
$ more / proc / cpuinfo
```

### - 실행중인 프로세스 상태 모니터링

```
$ gcc cpu.c -o cpu
$ ./cpu
```

### - 새로운 자식 프로세스를 생성하여 사용자 명령을 실행하는 방법

```
$ gcc cpu-print.c -o cpu-print
$ ./cpu-print
```

### - 프로세스의 가상 메모리와 실제 메모리 비교

```
memory1.c memory2.c 실행 및 비교
```

### - 프로세스의 가상 메모리와 실제 메모리 비교

```
disk.c disk1.c 실행 및 비교
```

-----------------------------------------------------------------------------
## 			2. SSUSHELL 및 기본 명령어 구현	

### - Linux 내장 명령어를 실행하는 간단한 쉘 구현

입력 (대화식 또는 배치식 모드로 을 읽고 입력을 토큰화)
```
$ ./ssu_shell : 대화식 모드
$ ./ssu_shell commands.txt : 배치식 모드 
```

### - 쉘 명령어 pps, ttop 구현

-----------------------------------------------------------------------------
##			3. xv6에서 시스템 호출 추가 및 스케줄러 수정

### - qemu 설치

```
$ apt-get install qemu-kvm
$ make -> $ make qemu
```

### - 새로운 시스템 호출 추가 및 간단한 응용 프로그램 구현
```
hello_test
helloname_test xv6
getnumproc_test
getmaxpid_test
getprocinfo_test
```

### - 우선순위 기반 RR 스케줄러 구현

```
seqinc_prio 5
seqdec_prio 5
```

------------------------------------------------------------------------------
##			4. Pthread

### - Master-Worker Thread Pool 구현

```
$ gcc master-worker.c -lpthread
$ ./check.awk
```

### - Reader-Writer Locks 쓰레드 프로그램 구현

```
$ ./rw_lock_test.sh
```

### - pthread를 이용한 사용자 수준 세마포어(SSU_Sem)구현

```
$ ./SSU_Sem_test.sh
```

-----------------------------------------------------------------------------
##			5. 동적 메모리 관리

### - 간단한 메모리 관리자 (동적으로 메모리 할당 및 해제) 구현

```
$ gcc test_alloc.c alloc.c
$ ./a.out
```

### - 확장 가능한 Heap

```
$ gcc test_ealloc.c ealloc.c
$ ./a.out
```

-------------------------------------------------------------------------------
##			6. 파일 시스템

### - 가상 디스크(애뮬레이터)를 위한 간단한 파일 시스템 및 기본 파일 operation(create, delete, open, close, read, write, lseek) 구현

```
./ssufs_test.sh
```
