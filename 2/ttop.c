#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <time.h>
#include <utmp.h>
#include <signal.h>
#include <termios.h>
#include <ncurses.h>

#define NAME_LEN_MAX 20
#define FNAME_LEN 300
#define MAX_NUM_TOKENS 64
#define MAX_TOKEN_SIZE 64

// ttop header부분 정보들의 구조체
typedef struct {
	char curtime[9];
	int uptime;
	int user_num;
	float load_av[3];
	int total_t;
	int run_t;
	int sleep_t;
	int stopped_t;
	int zombie_t;
	int cpu[9];
	int mem_total;
	int mem_free;
	int mem_used;
	int mem_buff_cache;
	int swap_total;
	int swap_free;
	int swap_used;
	int swap_avail;
}header_inform;

//	각 프로세스들의 정보 구조체
typedef struct{
	int pid;					// PID
	char user[NAME_LEN_MAX]; 	// USER
	char PR[3];					// PR
	int NI;						// NI
	int VIRT;					// VIRT
	int RES;					// RES
	int SHR;					// SHR
	char stat[2];				// STAT
	float cpu;					// %CPU
	float mem;					// %MEM
	int time;					// TIME
	int time_dot;				// TIME 소수점
	char cmd[1024];				// COMMAND
}ssu_proc;

ssu_proc process[99999];
header_inform h;

int save_cpu[9]; // 갱신 전 cpu값을 저장하는 배열
float print_cpu[8]; // cpu 사용량을 사용률로 변환하여 이곳에 저장.. 출력용

// ttop header정보들 중 갱신되면서 초기화해줘야 하는 값들 초기화
void ttop_init()
{
	memset(h.curtime, 0, sizeof(h.curtime));
	h.user_num = 0;
	h.total_t = 0;
	h.run_t = 0;
	h.sleep_t = 0;
	h.stopped_t = 0;
	h.zombie_t = 0;
	
	for (int i = 0; i < 8; i++){
		save_cpu[i] = h.cpu[i];
	}
}

// 문자열 " ", '\n', '\0'로 구분하여 토큰화
char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
 	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  	int i, tokenIndex = 0, tokenNo = 0;

  	for(i =0; i < strlen(line); i++){

    	char readChar = line[i];

    	if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
     	 	token[tokenIndex] = '\0';
  		if (tokenIndex != 0){
			tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
			strcpy(tokens[tokenNo++], token);
			tokenIndex = 0;
 	  	}
    	} else {
      		token[tokenIndex++] = readChar;
    	}
  	}

  	free(token);
  	tokens[tokenNo] = NULL;
  	return tokens;
}

// os 부팅 후, 얼마의 시간이 경과했는지 확인..
float uptime()
{
	FILE *fp;
	char temp[100];
	float stime;
	float idletime;

	if ((fp = fopen("/proc/uptime", "r")) == NULL){
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	fgets(temp, sizeof(temp), fp);
	sscanf(temp, "%f %f", &stime, &idletime);
	fclose(fp);

	return stime;
}

// 해당 파일에서 key값을 토대로 숫자값을 찾아오는 함수
int get_value_in_file(char *filename, char *key, int cnt)
{
	char line[1024];
	FILE *fp;
	int v = 0;

	memset(line, 0, sizeof(line));
	fp = fopen(filename, "r");
	while (fgets(line, sizeof(line), fp) != NULL){
		if (!strncmp(line, key, cnt)){
			int s = strlen(line);

			char t[100] = {0, };
			int i = 0;
			for (int idx = 0; idx < s; idx++){
				if (atoi(&line[idx]) != 0 || line[idx] == '0'){
					t[i] = line[idx];
					i++;
				} else {
					if (i > 0){
						break;
					}
				}
			}
			t[strlen(t)] = '\0';
			v = atoi(t);
		}
	}
	fclose(fp);

	return v;
}

// ttop header에 채워질 정보를 가져오는 함수
void get_header_inform()
{
	char temp_line[1024] = {0, };
	char **tok;
	FILE *fp;

	// curtime 구하기
	time_t cur;
	struct tm *tm;

	cur = time(NULL);
	tm = localtime(&cur);
	strftime(temp_line, sizeof(temp_line), "%H:%M:%S", tm);
	strcpy(h.curtime, temp_line);
	
	// uptime 구하기
	h.uptime = (int)uptime();

	// user 수 구하기
	struct utmp *utmpfp;

	utmpfp = malloc(sizeof(struct utmp));
	setutent();
	
	while ((utmpfp = getutent()) != NULL){
		if (utmpfp->ut_type == USER_PROCESS)
			h.user_num++;
	}

	// load average 구하기
	memset(temp_line, 0, sizeof(temp_line));
	fp = fopen("/proc/loadavg", "r");
	fgets(temp_line, sizeof(temp_line), fp);
	tok = tokenize(temp_line);
	for (int i = 0; i < 3; i++){
		h.load_av[i] = atof(tok[i]);
	}
	fclose(fp);
	for (int i = 0; tok[i] != NULL; i++)
		free(tok[i]);
	free(tok);

	// CPU USAGE 구하기
	memset(temp_line, 0, sizeof(temp_line));
	fp = fopen("/proc/stat", "r");
	fgets(temp_line, sizeof(temp_line), fp);
	tok = tokenize(temp_line);

	int total_cpu = 0;
	for (int i = 1; i < 9; i++){
		h.cpu[i] = atoi(tok[i]);
		total_cpu += h.cpu[i];
	}
	h.cpu[0] = total_cpu;

	for (int i = 1; i < 9; i++){
		print_cpu[i - 1] = (float)(h.cpu[i] - save_cpu[i])/ (float)(h.cpu[0] - save_cpu[0]) * 100;
	}

	for (int i = 0; i < 9; i++){
		save_cpu[i] = h.cpu[i];
	}

	fclose(fp);
	for (int i = 0; tok[i] != NULL; i++)
		free(tok[i]);
	free(tok);

	// mem 구하기
	h.mem_total = get_value_in_file("/proc/meminfo", "MemTotal", 8);
	h.mem_free = get_value_in_file("/proc/meminfo", "MemFree", 7);
	h.mem_buff_cache = get_value_in_file("/proc/meminfo", "Buffers", 7) + get_value_in_file("/proc/meminfo", "Cached", 6) + get_value_in_file("/proc/meminfo", "SReclaimable", 12);
	h.mem_used = h.mem_total - h.mem_free - h.mem_buff_cache; 

	// swap 구하기
	h.swap_total = get_value_in_file("/proc/meminfo", "SwapTotal", 9);
	h.swap_free = get_value_in_file("/proc/meminfo", "SwapFree", 8);
	h.swap_used = h.swap_total - h.swap_free; 
	h.swap_avail = get_value_in_file("/proc/meminfo", "MemAvailable", 12);
}

// 문자열이 전부 숫자로 이루어져 있는지 확인...
int isDigit(char *str)
{
	for(int i = 0; i < strlen(str); i++){
		if (isdigit(str[i]) == 0)
			return 0;
	}

	return 1;
}

// 각 프로세스의 정보들을 가져와 구조체에 저장해주는 함수
void parsing_inform(ssu_proc *p, char *statfile, char *process_id)
{
	FILE *fp;
	struct stat st;
	struct passwd *pwd;
	char status[10000] = {0, };
	char **inform;
	char temp[64] = {0, };
	char temp_line[1024] = {0, };

	// stat파일 읽기
	fp = fopen(statfile, "r");
	if (fp < 0){
		fprintf(stderr, "%s doesn't exist\n", statfile);
		exit(1);
	}

	// stat파일 정보 tokenize
	fgets(status, sizeof(status), fp);
	status[strlen(status)] = '\n';
	inform = tokenize(status);
	fclose(fp);

	// pid 가져오기
	p->pid = atoi(inform[0]);

	// USER 가져오기.
	sprintf(temp, "/proc/%s/status", process_id);
	uid_t ssu_uid = get_value_in_file(temp, "Uid", 3);
	pwd = getpwuid(ssu_uid);
	if (strlen(pwd->pw_name) > 8){
		strncpy(p->user, pwd->pw_name, 7);
		p->user[7] = '+';
		p->user[8] = '\0';
	}else
		strcpy(p->user, pwd->pw_name);

	// PR 가져오기.
	if (!strcmp(inform[17], "-100"))
		strcpy(p->PR, "rt");
	else
		strcpy(p->PR, inform[17]);

	// NI 가져오기.
	p->NI = atoi(inform[18]);

	// %cpu 가져오기
	float p_time = (atoi(inform[13]) + atoi(inform[14])) / sysconf(_SC_CLK_TCK);
	p->cpu = ((float)p_time / uptime()) * 100;
	p->cpu = (int)(p->cpu * 10);
	p->cpu = p->cpu / 10;

	// VIRT 가져오기
	p->VIRT = atol(inform[22]) / 1024;

	// RES 가져오기
	p->RES = atoi(inform[23]) * 4;

	// SHR 가져오기
	memset(temp, 0, sizeof(temp));
	sprintf(temp, "/proc/%s/status", process_id);
	p->SHR = get_value_in_file(temp, "RssFile", 7) + get_value_in_file(temp, "RssShmem", 8);

	// %mem 가져오기
	// (RSS값 / proc/stat의 전체 메모리) * 100
	int total_mem = (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)) / 1024;
	p->mem = ((float)p->RES / (float)total_mem) * 100;
	p->mem = (int)(p->mem * 10);
	p->mem = p->mem / 10;

	// STAT 가져오기
	strcpy(p->stat, inform[2]);
	if (!strcmp(inform[2], "R"))
		h.run_t++;
	else if (!strcmp(inform[2], "S"))
		h.sleep_t++;
	else if (!strcmp(inform[2], "T"))
		h.stopped_t++;
	else if (!strcmp(inform[2], "Z"))
		h.zombie_t++;

	// TIME 가져오기
	p->time = (int)p_time;
	float t = (float)(atoi(inform[13]) + atoi(inform[14])) / sysconf(_SC_CLK_TCK);
	p->time_dot = (int)((t - p_time) * 100);

	// COMMAND 가져오기
	memset(temp_line, 0, sizeof(temp_line));
	int s = strlen(inform[1]);
	int idx = 0;
	for (int i = 0; i < s; i++){
		if (inform[1][i] == '(' || inform[1][i] == ')')
			continue;
		if (inform[1][i] == '\0')
			break;
		temp_line[idx] = inform[1][i];
		idx++;
	}
	if (idx > 14)
		temp_line[15] = '\0';
	else
		temp_line[idx] = '\0';

	strcpy(p->cmd, temp_line);

	// 사용한 inform free
	for (int i = 0; inform[i] != NULL; i++)
		free(inform[i]);
	free(inform);
}

// 헤더부분 출력
void print_header()
{
	printw("top - %8s up %d day, %2d:%02d, %d user, load average: %.2f, %.2f, %.2f\n", h.curtime, h.uptime / (3600 * 24), (h.uptime % (3600 * 24))/ 3600, (h.uptime % 3600) / 60, h.user_num, h.load_av[0], h.load_av[1], h.load_av[2]);
	printw("Tasks: %d total,   %d running, %d sleeping,   %d stopped,   %d zombie\n", h.total_t, h.run_t, h.sleep_t, h.stopped_t, h.zombie_t);
	printw("%%Cpu(s):  %.1f us,  %.1f sy,  %.1f ni, %.1f id,  %.1f wa,  %.1f hi,  %.1f si,  %.1f st\n", print_cpu[0], print_cpu[1], print_cpu[2], print_cpu[3], print_cpu[4], print_cpu[5], print_cpu[6], print_cpu[7]);
	printw("KiB Mem : %8d total, %8d free, %8d used, %8d buff/cache\n", h.mem_total, h.mem_free, h.mem_used, h.mem_buff_cache);
	printw("KiB Swap: %8d total, %8d free, %8d used, %8d avail Mem\n\n", h.swap_total, h.swap_free, h.swap_used, h.swap_avail);
}

// process들의 정보 출력
void print_process(int cnt, int row, int width)
{
	char line[2048];

	printw(" %4s %-8s %3s %3s %7s %6s %6s %-1s  %4s %4s   %7s %s\n", "PID", "USER", "PR", "NI", "VIRT", "RES", "SHR", "S", "\%CPU", "\%MEM", "TIME+", "COMMAND");
	
	for(int i = 0; i < row - 4; i++){
		memset(line, 0, sizeof(line));
		if (process[i].pid == 0)
			continue;
		sprintf(line, " %4d %-8s %3s %3d %7d %6d %6d %-1s  %4.1f %4.1f   %2d:%02d.%02d %s\n", process[i].pid, process[i].user, process[i].PR, process[i].NI, process[i].VIRT, process[i].RES, process[i].SHR, process[i].stat, process[i].cpu, process[i].mem, process[i].time / 60, process[i].time % 60, process[i].time_dot, process[i].cmd);
		line[width - 1] = '\0';
		printw("%s", line);
	}
}

void run()
{
	DIR *dir;
	struct dirent *dentry = NULL;
	int cnt = 0;

	int btime = get_value_in_file("/proc/stat", "btime", 5);

	if ((dir = opendir("/proc")) == NULL){
		fprintf(stderr, "opendir error\n");
		exit(1);
	}

	get_header_inform();

	while ((dentry = readdir(dir)) != NULL){
		char statfile[FNAME_LEN];

		// . , .. 디렉토리 넘기기
		if (!strcmp(dentry->d_name, ".") && !strcmp(dentry->d_name,"..")){
			continue;
		}
		
		// 해당 파일이 stat을 가진 디렉토리가 아니라면.. 볼 필요 없음.
		sprintf(statfile, "/proc/%s/stat", dentry->d_name);
		if (access(statfile, F_OK) != 0)
				continue;

		// 해당 디렉토리 이름이 숫자(pid) 라면..
		if (isDigit(dentry->d_name)){
			parsing_inform(&process[cnt], statfile, dentry->d_name);
		}

		cnt++;
	}

	// total task
	h.total_t = cnt -1;

	// 현재 터미널 가로크기 구하기
	struct winsize w;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	//3초마다 terminal 초기화.. 
	print_header();
	print_process(cnt, w.ws_row, w.ws_col);

}

int main()
{
	initscr(); // ncurses 용 스크린 생성
	noecho();  // 사용자가 입력하는 문자 보여주지 않음
	keypad(stdscr, TRUE); // 방향키 입력 가능하게 설정

	while (1){
		clear(); // 화면 클리어

		ttop_init();	// ttop header값들 초기화
		run();			// ttop inform 가져오기

		refresh(); // printw 한것들 출력
		nodelay(stdscr, TRUE); // getch non-blocking 설정
		int key = getch();

		if (key == 'q'){
			endwin(); // ncurse 용 스크린 해제
			exit(0);
		}
		else if (key == KEY_UP)
			continue;
		else if (key == KEY_DOWN)
			continue;
		nodelay(stdscr, FALSE);
		sleep(3);
	}
	return 0;
}

