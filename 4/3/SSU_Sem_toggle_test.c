#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>
#include "SSU_Sem.h"

#define NUM_THREADS 3
#define NUM_ITER 10

SSU_Sem S[NUM_THREADS]; // 쓰레드 수만큼 세마포어 생성

void *justprint(void *data)
{
  int thread_id = *((int *)data);

  // thread 0말곤 전부 대기
  if (thread_id != 0)
  	SSU_Sem_down(&S[thread_id]);

  for(int i=0; i < NUM_ITER; i++)
    {
      printf("This is thread %d\n", thread_id);

	  // 다음 순서의 스레드를 깨우고 현재 스레드는 대기
	  if(thread_id == NUM_THREADS - 1)
	  	SSU_Sem_up(&S[0]);
	  else
	  	SSU_Sem_up(&S[thread_id + 1]);
	  SSU_Sem_down(&S[thread_id]);
    }

  // for문이 모든 스레드애서 종료되면 thread 0 말고는 대기상태이기에, 종료를 위해 모두 깨워줌
  for(int i = 1; i < NUM_THREADS; i++){
  	SSU_Sem_up(&S[i]);
  }
  
  return 0;
}

int main(int argc, char *argv[])
{

  pthread_t mythreads[NUM_THREADS];
  int mythread_id[NUM_THREADS];
  
  // 세마포어 초기화
  for(int i = 0; i < NUM_THREADS; i++){
  	SSU_Sem_init(&S[i], 0);
  }

  for(int i =0; i < NUM_THREADS; i++)
    {
      mythread_id[i] = i;
      pthread_create(&mythreads[i], NULL, justprint, (void *)&mythread_id[i]);
    }
  
  for(int i =0; i < NUM_THREADS; i++)
    {
      pthread_join(mythreads[i], NULL);
    }
  
  return 0;
}
