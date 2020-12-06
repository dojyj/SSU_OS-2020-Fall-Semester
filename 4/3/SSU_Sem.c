#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include "SSU_Sem.h"

#define MAX_SEMS 100

// 변수값들 초기화
void SSU_Sem_init(SSU_Sem *s, int value) {
  	pthread_mutex_init(&(s->mutex), NULL);
  	pthread_cond_init(&(s->cond), NULL);
  	s->count = value;
	s->wait_list = 0;
}

void SSU_Sem_down(SSU_Sem *s) {
	
	pthread_mutex_lock(&(s->mutex)); 
	
	s->count--;

	// count값이 음수가 되면 대기 queue에 추가. signal 올 때 까지 대기. 다른 쓰레드도 대기할수 있게 count값 초기화
	while (s->count < 0){
		s->wait_list++;
		s->count = 0;
		pthread_cond_wait(&(s->cond), &(s->mutex));
	}

	pthread_mutex_unlock(&(s->mutex));
}

void SSU_Sem_up(SSU_Sem *s) {
	pthread_mutex_lock(&(s->mutex));

	if (s->wait_list == 0)	// 대기하고 있는 쓰레드가 없다면 count값 ++
		s->count++;
	else if (s->wait_list > 0){	// 대기하고 있는 쓰레드가 있다면 깨우고, 대기 queue애서 제거
		s->wait_list--;
		pthread_cond_signal(&(s->cond));
	}

	pthread_mutex_unlock(&(s->mutex));
}
