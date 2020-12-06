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

#define NAME_LEN_MAX 20
#define FNAME_LEN 300
#define MAX_NUM_TOKENS 64
#define MAX_TOKEN_SIZE 64

// process정보를 담는 구조체
typedef struct{
	char user[NAME_LEN_MAX]; 	// USER
	int pid;					// PID
	float cpu;					// %CPU
	float mem;					// %MEM
	int VSZ;					// VSZ
	int RSS;					// RSS
	char tty[10];				// TTY
	char stat[5];				// STAT
	char start[10];				// START
	int time;					// TIME
	char cmd[1024];				// COMMAND
}ssu_proc;

ssu_proc process[99999];
long btime;

/* Splits the string by space and returns the array of tokens
*
*/
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
  tokens[tokenNo] = NULL ;
  return tokens;
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

// 주어진 파일명의 파일에서 key문자열이 포함되어있는 문자열을 찾아 첫번째 숫자 문자열을 찾아 int값으로 반환해주는 함수 
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
			char *t;
			t = (char *)malloc(s * sizeof(char));
			int i = 0;
			for (int idx = 0; idx < s; idx++){
				if (atoi(&line[idx]) != 0 || line[idx] == '0'){
					t[i] = line[idx];
					i++;
				}
			}
			v = atoi(t);
			free(t);
		}
	}
	fclose(fp);

	return v;
}

// 해당하는 프로세스의 정보를 찾아 구조체에 넣어주는 함수
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

	// pid 가져오기
	p->pid = atoi(inform[0]);

	// TIME 가져오기
	time_t p_time = (atoi(inform[13]) + atoi(inform[14])) / sysconf(_SC_CLK_TCK);
	p->time = (int)p_time;

	// %cpu 가져오기
	p->cpu = ((float)p_time / uptime()) * 100;
	p->cpu = (int)(p->cpu * 10);
	p->cpu = p->cpu / 10;

	// VSZ 가져오기
	p->VSZ = atol(inform[22]) / 1024;

	// RSS 가져오기
	p->RSS = atoi(inform[23]) * 4;

	// %mem 가져오기
	// (RSS값 / proc/stat의 전체 메모리) * 100
	int total_mem = (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)) / 1024;
	p->mem = ((float)p->RSS / (float)total_mem) * 100;
	p->mem = (int)(p->mem * 10);
	p->mem = p->mem / 10;

	// TTY 가져오기
	int tty = atoi(inform[6]);
	if (tty >= 1024 && tty < 34816)
		sprintf(p->tty, "tty%d", tty - 1024);
	else if (tty >= 34816)
		sprintf(p->tty, "pts/%d", tty - 34816);
	else
		sprintf(p->tty, "?");

	// STAT 가져오기
	strcpy(p->stat, inform[2]);
	// nice값이 양수이면 N.. 음수이면 <..
	if (atoi(inform[18]) > 0)
		sprintf(p->stat, "%sN", p->stat);
	else if (atoi(inform[18]) < 0)
		sprintf(p->stat, "%s<", p->stat);
	// lock된 메모리가 있다면 L..
	memset(temp, 0, sizeof(temp));
	sprintf(temp, "/proc/%s/status", process_id);
	if (get_value_in_file(temp, "VmLck", 5) > 0)
		sprintf(p->stat, "%sL", p->stat);
	// sid == pid라면 s..
	if (!strcmp(inform[0], inform[5]))
		sprintf(p->stat, "%ss", p->stat);
	// thread갯수가 1개보다 많다면 l..
	if (atoi(inform[19]) > 1)
		sprintf(p->stat, "%sl", p->stat);
	// 해당 프로세스 fg에서 실행중이라면 +..
	if (!strcmp(inform[4], inform[7]))
		sprintf(p->stat, "%s+", p->stat);

	// START 가져오기
	time_t start_t;
	time_t curtime;
	struct tm *start_tm;
	struct tm *cur_tm;
	char timestring[1024];
	struct tm temp_tm;

	memset(timestring, 0, sizeof(timestring));
	start_t = btime + (atoi(inform[21]) / sysconf(_SC_CLK_TCK));
	start_tm = localtime(&start_t);
	memcpy(&temp_tm, start_tm, sizeof(struct tm));
	curtime = time(NULL);
	cur_tm = localtime(&curtime);

	if (start_t < curtime - ((cur_tm->tm_hour * 3600) + (cur_tm->tm_min * 60) + (cur_tm->tm_sec)))
		strftime(timestring, sizeof(timestring), "%m월%d", &temp_tm);
	else
		strftime(timestring, sizeof(timestring), "%H:%M", &temp_tm);
	strcpy(p->start, timestring);

	// COMMAND 가져오기
	memset(temp, 0, sizeof(temp));
	memset(temp_line, 0, sizeof(temp_line));
	sprintf(temp, "/proc/%s/cmdline", process_id);
	fp = fopen(temp, "r");
	if (fp < 0){
		fprintf(stderr, "fopen error for %s\n", temp);
		exit(1);
	}
	fgets(temp_line, sizeof(temp_line), fp);
	char *ptr = temp_line;
	int cursor = 0;
	
	while (1){
		if (*ptr == '\0'){
			if (*(ptr + 1) != '\0')
				*ptr = ' ';
			else
				break;
		}
		ptr = ptr + 1;
		cursor++;
	}
	fclose(fp);
	if (strlen(temp_line) == 0){ // cmdline 파일에 프로세스명이 없다면..
		strcpy(temp_line, inform[1]);
		temp_line[0] = '[';
		if (strlen(temp_line) > 15){
			temp_line[16] = ']';
			temp_line[17] = '\0';
		}else
			temp_line[strlen(temp_line) - 1] = ']';
	}

	strcpy(p->cmd, temp_line);
	
	// 사용한 inform free
	for (int i = 0; inform[i] != NULL; i++)
		free(inform[i]);
	free(inform);
}

// ps 명령어처럼 출력해주는 함수, ps 옵션처리를 여기서 해준다.
void print_ps(char *option, int cnt, int width)
{
	int i;
	char username[64];
	struct passwd *pwd;
	char ttyline[2048];

	// 현재 유저 구하기
	pwd = getpwuid(geteuid());
	if (strlen(pwd->pw_name) > 8){
		strncpy(username, pwd->pw_name, 7);
		username[7] = '+';
		username[8] = '\0';
	}else
		strcpy(username, pwd->pw_name);

	if(option == NULL){ // ps
		printf(" %4s %-5s %11s %-s\n", "PID", "TTY", "TIME", "CMD");
		for (i = 0; i < cnt; i++){
			memset(ttyline, 0, sizeof(ttyline));
			if (!strcmp(process[cnt - 1].tty, process[i].tty)){
				sprintf(ttyline, " %4d %-5s    %02d:%02d:%02d %-s", process[i].pid, process[i].tty, process[i].time / 3600, process[i].time / 60, process[i].time % 60, process[i].cmd);
				if (strlen(ttyline) > width){
					ttyline[width] = '\n';
					ttyline[width + 1] = '\0';
				}
				else
					ttyline[strlen(ttyline)] = '\n';
				printf("%s", ttyline);
			}
		}
	}else if(!strcmp(option, "a")){ // ps a
		printf(" %4s %-5s    %-4s %6s %-s\n", "PID", "TTY", "STAT", "TIME", "CMD");
		for (i = 0; i < cnt; i++){
			memset(ttyline, 0, sizeof(ttyline));
			if (process[i].pid == 0)
				continue;
			
			if (!strcmp(process[i].tty, "?"))
				continue;
			else {
				sprintf(ttyline, " %4d %-5s    %-4s  %2d:%02d %-s", process[i].pid, process[i].tty, process[i].stat, process[i].time / 60, process[i].time % 60, process[i].cmd);
				if (strlen(ttyline) > width){
					ttyline[width] = '\n';
					ttyline[width + 1] = '\0';
				}
				else
					ttyline[strlen(ttyline)] = '\n';
				printf("%s", ttyline);
			}
		}
	}else if(!strcmp(option, "u")){ // ps u
		printf(" %-8s %5s %4s %4s %7s %6s %-8s %-5s %-5s %5s %-s\n", "USER", "PID", "\%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
		for (i = 0; i < cnt; i++){
			if (process[i].pid == 0)
				continue;
			if (strcmp(process[i].user, username))
				continue;
			if (!strcmp(process[i].tty, "?"))
				continue;
			else {
				memset(ttyline, 0, sizeof(ttyline));
				sprintf(ttyline, " %-8s %5d %4.1f %4.1f %7d %6d %-8s %-5s %-5s %2d:%02d %-s", process[i].user, process[i].pid, process[i].cpu, process[i].mem, process[i].VSZ, process[i].RSS, process[i].tty, process[i].stat, process[i].start, process[i].time / 60, process[i].time % 60, process[i].cmd);
				if (strlen(ttyline) > width){
					ttyline[width] = '\n';
					ttyline[width + 1] = '\0';
				}
				else
					ttyline[strlen(ttyline)] = '\n';
				printf("%s", ttyline);
			}
		}
	}else if(!strcmp(option, "x")){ // ps x
		printf(" %4s %-5s    %-4s %6s %-s\n", "PID", "TTY", "STAT", "TIME", "CMD");
		for (i = 0; i < cnt; i++){
			memset(ttyline, 0, sizeof(ttyline));
			if (process[i].pid == 0)
				continue;
			if (strcmp(process[i].user, username))
				continue;
			sprintf(ttyline, " %4d %-5s    %-4s  %2d:%02d %-s", process[i].pid, process[i].tty, process[i].stat, process[i].time / 60, process[i].time % 60, process[i].cmd);
			if (strlen(ttyline) > width){
				ttyline[width] = '\n';
				ttyline[width + 1] = '\0';
			}
			else
				ttyline[strlen(ttyline)] = '\n';
			printf("%s", ttyline);
		}
	}else if(!strcmp(option, "au")){ // ps au
		printf(" %-8s %5s %4s %4s %7s %6s %-8s %-5s %-5s %5s %-s\n", "USER", "PID", "\%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
		for (i = 0; i < cnt; i++){
			memset(ttyline, 0, sizeof(ttyline));
			if (process[i].pid == 0)
				continue;
			
			if (!strcmp(process[i].tty, "?"))
				continue;
			else{
				sprintf(ttyline, " %-8s %5d %4.1f %4.1f %7d %6d %-8s %-5s %-5s %2d:%02d %-s", process[i].user, process[i].pid, process[i].cpu, process[i].mem, process[i].VSZ, process[i].RSS, process[i].tty, process[i].stat, process[i].start, process[i].time / 60, process[i].time % 60, process[i].cmd);
				if (strlen(ttyline) > width){
					ttyline[width] = '\n';
					ttyline[width + 1] = '\0';
				}
				else
					ttyline[strlen(ttyline)] = '\n';
				printf("%s", ttyline);
			}
		}
	}else if(!strcmp(option, "ax")){ // ps ax
		printf(" %4s %-5s    %-4s %6s %-s\n", "PID", "TTY", "STAT", "TIME", "CMD");
		for (i = 0; i < cnt; i++){
			memset(ttyline, 0, sizeof(ttyline));
			if (process[i].pid == 0)
				continue;
			sprintf(ttyline, " %4d %-5s    %-4s  %2d:%02d %-s", process[i].pid, process[i].tty, process[i].stat, process[i].time / 60, process[i].time % 60, process[i].cmd);
			if (strlen(ttyline) > width){
				ttyline[width] = '\n';
				ttyline[width + 1] = '\0';
			}
			else
				ttyline[strlen(ttyline)] = '\n';
			printf("%s", ttyline);
		}

	}else if(!strcmp(option, "ux")){ // ps ux
		printf(" %-8s %5s %4s %4s %7s %6s %-8s %-5s %-5s %5s %-s\n", "USER", "PID", "\%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
		for (i = 0; i < cnt; i++){
			memset(ttyline, 0, sizeof(ttyline));
			if (process[i].pid == 0)
				continue;
			if (strcmp(process[i].user, username))
				continue;
			sprintf(ttyline, " %-8s %5d %4.1f %4.1f %7d %6d %-8s %-5s %-5s %2d:%02d %-s", process[i].user, process[i].pid, process[i].cpu, process[i].mem, process[i].VSZ, process[i].RSS, process[i].tty, process[i].stat, process[i].start, process[i].time / 60, process[i].time % 60, process[i].cmd);
			if (strlen(ttyline) > width){
				ttyline[width] = '\n';
				ttyline[width + 1] = '\0';
			}
			else
				ttyline[strlen(ttyline)] = '\n';
			printf("%s", ttyline);
		}
	}else if(!strcmp(option, "aux")){ // ps aux
		printf("%-8s %5s %4s %4s %7s %6s %-8s %-5s %-5s %5s %-s\n", "USER", "PID", "\%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
		for (i = 0; i < cnt; i++){
			memset(ttyline, 0, sizeof(ttyline));
			if (process[i].pid == 0)
				continue;
			sprintf(ttyline, "%-8s %5d %4.1f %4.1f %7d %6d %-8s %-5s %-5s %2d:%02d %-s", process[i].user, process[i].pid, process[i].cpu, process[i].mem, process[i].VSZ, process[i].RSS, process[i].tty, process[i].stat, process[i].start, process[i].time / 60, process[i].time % 60, process[i].cmd);
			if (strlen(ttyline) > width){
				ttyline[width] = '\n';
				ttyline[width + 1] = '\0';
			}
			else
				ttyline[strlen(ttyline)] = '\n';
			printf("%s", ttyline);
		}
	}else{
		printf("invalid option");
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	DIR *dir;
	struct dirent *dentry = NULL;
	int cnt = 0;

	btime = get_value_in_file("/proc/stat", "btime", 5);
	if ((dir = opendir("/proc")) == NULL){
		fprintf(stderr, "opendir error\n");
		exit(1);
	}

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

	// 현재 터미널 가로크기 구하기
	struct winsize w;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	print_ps(argv[1], cnt, w.ws_col);
	return 0;
}
