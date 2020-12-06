#include "ealloc.h"

typedef struct {
	int loc;
	int next_loc;
	int size;
}memory_manager;

memory_manager m_m[64]; // 16 * 4

int cur_page_num;
int num; // mem chunk num
char *page;
char *page1;
char *page2;
char *page3;
char *page4;

void init_alloc()
{
	for(int i = 0; i < 64; i++){
		m_m[i].loc = 0;
		m_m[i].next_loc = -1;
		m_m[i].size = 0;
	}
	num = 0;
	cur_page_num = 0;

	return;
}

void cleanup()
{
	memset(m_m, 0, sizeof(m_m));
	num = 0;

	return;
}

char *alloc(int buf_size)
{
//	if (num != 0)
//		printf("num of chunks : %d, num of pages : %d, chunk's loc : %d, page addr : %p\n", num, cur_page_num, m_m[num-1].loc, page);
	// error check
	if (buf_size < MINALLOC || buf_size % MINALLOC != 0){
		fprintf(stderr, "request size must be 8byte or multiple of 8\n");
		return NULL;
	}


	if (num == 0 && cur_page_num == 0){
		// data chunk가 하나도 없을때
		page1 = mmap(0, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		page = page1;
		m_m[num].size = buf_size;
		num++;
		cur_page_num++;
		return page;
	}

	// 중간에 free된 memory chunk가 있는 경우..
	for (int i = 0; i < num; i++){
		// 다음 memory chunk까지의 여백이 존재하고 buf_size보다 크거나 같다면
		if (m_m[i].next_loc - (m_m[i].loc + m_m[i].size) >= buf_size){
//			printf("find freed chunk!\n");
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

	if (num == 0)
		m_m[num].loc = 0;
	else
		m_m[num].loc = m_m[num - 1].loc + m_m[num - 1].size;

	if (m_m[num].loc >= PAGESIZE * cur_page_num){
		//printf("page ++\n");
		if (cur_page_num == 1){
			page2 = mmap(0, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			page = page2;
		} else if (cur_page_num == 2){
			page3 = mmap(0, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			page = page3;
		} else if (cur_page_num == 3){
			page4 = mmap(0, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			page = page4;
		} else {
			fprintf(stderr, "page num cannot over 4\n");
			return NULL;
		}

		cur_page_num++;
	}

	if (cur_page_num == 4){
	if (m_m[num].loc >= PAGESIZE * 3)
		page = page4;
	else if (m_m[num].loc >= PAGESIZE * 2)
		page = page3;
	else if (m_m[num].loc >= PAGESIZE * 1)
		page = page2;
	else 
		page = page1;
	}
	m_m[num].size = buf_size;
	m_m[num - 1].next_loc = m_m[num].loc;
	num++;
	return page + (m_m[num-1].loc % PAGESIZE);
}

void dealloc(char *loc)
{
//	printf("please dealloc! %p\n", loc);
	if (num == 1){
//		printf("clean up !\n");
		for(int i = 0; i < 64; i++){
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

//				printf("%d dealloc.. loc : %d\n", i, m_m[i].loc);
				m_m[i].loc = 0;
				m_m[i].size = 0;
				m_m[i].next_loc = -1;
			}
			else{
				if (i != 0)
					m_m[i-1].next_loc = m_m[i+1].loc;	// 다음 memory chunk의 위치를 가리킴.

//				printf("%d dealloc.. loc : %d\n", i, m_m[i].loc);

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


