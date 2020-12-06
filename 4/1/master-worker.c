#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>

int PCbit; // produce 상태와 consume 상태를 구분하는 bit
int mnum,wnum,master_last,worker_last;
int item_to_produce, curr_buf_size, item_counts;
int total_items, max_buf_size, num_workers, num_masters;

// pthread 구조체 선언
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t worker_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t master_cond = PTHREAD_COND_INITIALIZER;

int *buffer;

void print_produced(int num, int master) {

  printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {

  printf("Consumed %d by worker %d\n", num, worker);
  
}


//produce items and place in buffer
//modify code below to synchronize correctly
void *generate_requests_loop(void *data)
{
  int thread_id = *((int *)data);

  while(1)
    {
  		pthread_mutex_lock(&mutex); // 전역변수 사용전 lock

		// produce 전, consume thread모두 wait 처리
		if(item_to_produce == 0)
			pthread_cond_wait(&master_cond, &mutex);

		// 현재 버프 크기가 버퍼의 맥스 크기를 초과한다면 에러
		if (curr_buf_size > max_buf_size){
			fprintf(stderr, "buf index error\n");
			exit(1);
		}

		// 생성해야할 숫자보다 많이 생성했다면 에러
		if (item_to_produce > total_items){
			fprintf(stderr, "num exceed error\n");
			exit(1);
		}

		// buffer가 필요한 수 보다 작을 때, 버퍼단위로 produce - consume
		if ((curr_buf_size == max_buf_size) || (PCbit == 1 && wnum != 0) || (item_to_produce == total_items)){
			//printf("go to work!!!!!!!!!!!!\n");
			if (master_last == 1 || num_masters == 1){
			//	printf("this is master last\n");
				PCbit = 1;
				wnum = num_workers;
				worker_last = 0;
				pthread_cond_broadcast(&worker_cond);
				pthread_cond_broadcast(&cond);
			} else {
				printf("%d --\n", thread_id);
				mnum--;
				if (mnum == 1)
					master_last = 1;
			}

			// 생성할 숫자를 모두 생성했다면
			if(item_to_produce == total_items) {
	  			pthread_mutex_unlock(&mutex);						// break 전 unlock
			//	printf("master %d exit!!!!!!!!!!!!, mnum : %d\n", thread_id, mnum);
				break;
      		}

			// 실행할 숫자가 남았다면
			pthread_cond_wait(&master_cond, &mutex);
		}

		// M개의 마스터쓰레드중 정지된 게 없고, 버퍼에 자리가 있을때만 produce
		if ((mnum == num_masters) && (curr_buf_size < max_buf_size)){
      		buffer[curr_buf_size++] = item_to_produce;
      		print_produced(item_to_produce, thread_id);
			//printf("curr buf size : %d\n", curr_buf_size - 1);
      		item_to_produce++;
		}

	  	pthread_mutex_unlock(&mutex);
    }
  return 0;
}

//write function to be run by worker threads
//ensure that the workers call the function print_consumed when they consume an item
void *consume_item(void *data){
	int thread_id = *((int *)data);
	int item_to_consume;

  while(1)
    {
	  	pthread_mutex_lock(&mutex);

		// consumer thread의 균등한 소비를 위함. 각 쓰레드는 자신의 쓰레드id의 배수만 소비하도록 노력.
		// item을 모두 소비하거나, produce모드거나, 프로그램이 방금 시작한 경우, 여기서 대기하지않음.
		if (curr_buf_size % num_workers != thread_id && item_counts != 0 && PCbit == 1 && curr_buf_size != 0 && num_workers != 1){
			//printf("worker wait 1\n");
			pthread_cond_wait(&cond, &mutex);
		}
		//printf("buf: %d id: %d\n", curr_buf_size, thread_id);
      	
		// 전체 item 만큼 소비했다면 워커쓰레드 모두 종료
		if(item_counts <= 0) {
			if (item_counts < 0){
				fprintf(stderr, "consume error\n");
				exit(1);
			}
			//printf("worker %d exit!!!!!!!!!!!!!!!!!!!!!!!!\n", thread_id);
			pthread_cond_broadcast(&cond);
			pthread_mutex_unlock(&mutex);
			break;
		}

		// 잘못된 버퍼 인덱스에 접근한경우
		if (curr_buf_size < 0){
			fprintf(stderr, "consume buffer error\n");
			exit(1);
		}

		// 프로그램이 방금 시작했거나, 버퍼의 모든 값을 소비했을경우 모든 워커쓰레드는 대기
		if (curr_buf_size == 0 || (PCbit == 0 && mnum != 0)) {
			//printf("go to produce!!!!!!!!!!!!!!!!!!!!!!!!\n");
			if (worker_last == 1 || num_workers == 1){
			//	printf("this is worker last\n");
				PCbit = 0;
				mnum = num_masters;
				master_last = 0;
				pthread_cond_broadcast(&master_cond);
			} else {
				wnum--;
				if (wnum == 1)
					worker_last = 1;
			}
			pthread_cond_wait(&worker_cond, &mutex);
		}

		// 혹시 쓰레드 종료 안된게 있다면...
		if(item_counts <= 0) {
			printf("worker %d exit!!!!!!!!!!!!!!!!!!!!!!!!\n", thread_id);
			pthread_cond_broadcast(&cond);
			pthread_cond_broadcast(&master_cond);
			pthread_mutex_unlock(&mutex);
			break;
		}

		// 워커쓰레드가 모두 살아있고, 아직 버퍼에 데이터가 남아있다면
		if (wnum == num_workers && curr_buf_size > 0){
    		item_to_consume = buffer[--curr_buf_size];
      		print_consumed(item_to_consume, thread_id);
			item_counts--;
			pthread_cond_broadcast(&cond);
		}
		pthread_mutex_unlock(&mutex);
    }

  return 0;
}

int main(int argc, char *argv[])
{
  int *master_thread_id;
  pthread_t *master_thread;
  item_to_produce = 0;
  curr_buf_size = 0;
  PCbit = 0;
  master_last = 0;
  worker_last = 0;

  int *worker_thread_id;
  pthread_t *worker_thread;
  
  int i;
  
   if (argc < 5) {
    printf("./master-worker #total_items #max_buf_size #num_workers #masters e.g. ./exe 10000 1000 4 3\n");
    exit(1);
  }
  else {
    num_masters = atoi(argv[4]);
    num_workers = atoi(argv[3]);
    total_items = atoi(argv[1]);
    max_buf_size = atoi(argv[2]);
	item_counts = total_items;
	mnum = num_masters;
	wnum = num_workers;
  }
    

   buffer = (int *)malloc (sizeof(int) * max_buf_size);

   //create master producer threads
   master_thread_id = (int *)malloc(sizeof(int) * num_masters);
   master_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);
  for (i = 0; i < num_masters; i++)
    master_thread_id[i] = i;

  for (i = 0; i < num_masters; i++)
    pthread_create(&master_thread[i], NULL, generate_requests_loop, (void *)&master_thread_id[i]);
  
  //create worker consumer threads
   worker_thread_id = (int *)malloc(sizeof(int) * num_workers);
   worker_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);
  for (i = 0; i < num_workers; i++)
    worker_thread_id[i] = i;

  for (i = 0; i < num_workers; i++)
    pthread_create(&worker_thread[i], NULL, consume_item, (void *)&worker_thread_id[i]);

  
  //wait for all threads to complete
  for (i = 0; i < num_masters; i++)
    {
      pthread_join(master_thread[i], NULL);
      printf("master %d joined\n", i);
    }
  
  //wait for all threads to complete
  for (i = 0; i < num_workers; i++)
    {
      pthread_join(worker_thread[i], NULL);
      printf("worker %d joined\n", i);
    }
  
  /*----Deallocating Buffers---------------------*/
  free(buffer);
  free(master_thread_id);
  free(master_thread);
  free(worker_thread_id);
  free(worker_thread);
  
  return 0;
}
