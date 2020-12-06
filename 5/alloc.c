#include "alloc.h"

typedef struct {
	int loc;
	int next_loc;
	int size;
}memory_manager;

memory_manager m_m[512];

int num; // mem chunk num
char *page;

int init_alloc()
{
	page = mmap(0, PAGESIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (page == MAP_FAILED){
		fprintf(stderr,"map failed\n");
		return -1;
	}

	for(int i = 0; i < 512; i++){
		m_m[i].loc = 0;
		m_m[i].next_loc = -1;
		m_m[i].size = 0;
	}
	num = 0;

	return 0;
}

int cleanup()
{
	//printf("=========== before unmap =========\n");
	//system("ps aux | grep a.out");
	if (munmap(page, PAGESIZE) == -1)
		return -1;

	memset(m_m, 0, sizeof(m_m));
	num = 0;

	//printf("=========== after unmap =========\n");
	//system("ps aux | grep a.out");
	return 0;
}

char *alloc(int buf_size)
{
	// 1. alloc 할 위치를 찾는다 ( memory manager 순환하면서..)
	// 2. alloc 하고 memory manager update

	// error check
	if (buf_size < MINALLOC || buf_size % MINALLOC != 0){
		fprintf(stderr, "request size must be 8byte or multiple of 8\n");
		return NULL;
	}

	if (num == 0){
		// data chunk가 하나도 없을때
		m_m[num].size = buf_size;
		num++;
		return page + m_m[num-1].loc;
	}

	for (int i = 0; i < num; i++){
		// 다음 memory chunk까지의 여백이 존재하고 buf_size보다 크거나 같다면
		if (m_m[i].next_loc - (m_m[i].loc + m_m[i].size) >= buf_size){
			for (int idx = num; idx > i; idx--){
				m_m[idx].loc = m_m[idx-1].loc;
				m_m[idx].size = m_m[idx-1].size;
				m_m[idx].next_loc = m_m[idx-1].next_loc;
			}
			
			m_m[i].loc = m_m[i - 1].loc + m_m[i - 1].size;
			m_m[i].size = buf_size;
			m_m[i - 1].next_loc = m_m[i].loc;
			m_m[i].next_loc = m_m[i+1].loc;
			num++;
			return page + m_m[i].loc;
		}
	}

	m_m[num].loc = m_m[num - 1].loc + m_m[num - 1].size;
	m_m[num].size = buf_size;
	m_m[num - 1].next_loc = m_m[num].loc;
	num++;
	return page + m_m[num-1].loc;
}

void dealloc(char *loc)
{
	if (num == 1){
		for(int i = 0; i < 512; i++){
			m_m[i].loc = 0;
			m_m[i].next_loc = -1;
			m_m[i].size = 0;
		}
		num = 0;

		return;
	}

	for (int i = 0; i < num; i++){
		// 해당 위치의 memory chunk index를 찾음.
		if (loc == page + m_m[i].loc){
			if (m_m[i+1].loc == 0){
				m_m[i-1].next_loc = -1; // 마지막 memory chunk였다면 다음 loc은 없음.
				m_m[i].loc = 0;
				m_m[i].size = 0;
				m_m[i].next_loc = -1;
			}
			else{
				if (i != 0)
					m_m[i-1].next_loc = m_m[i+1].loc;	// 다음 memory chunk의 위치를 가리킴.
				for (int idx = i; idx < num - 1; idx++){
					m_m[idx].loc = m_m[idx+1].loc;
					m_m[idx].size = m_m[idx+1].size;
					m_m[idx].next_loc = m_m[idx+1].next_loc;
				}
				m_m[num-1].loc = 0;
				m_m[num-1].size = 0;
				m_m[num-1].next_loc = -1;
			}

			num--;
		}
	}
}


