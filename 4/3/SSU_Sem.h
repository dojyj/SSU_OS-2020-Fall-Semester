#include <pthread.h>

#define MAX_SEM 1024

typedef struct SSU_Sem {
	int count;				// 관리하는 값
	int wait_list;			// 대기중인 쓰레드 수
	pthread_mutex_t mutex;	
	pthread_cond_t cond;
} SSU_Sem;

void SSU_Sem_init(SSU_Sem *, int);
void SSU_Sem_up(SSU_Sem *);
void SSU_Sem_down(SSU_Sem *);
