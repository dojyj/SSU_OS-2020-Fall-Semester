#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  // 입력받은 라인의 길이만큼 돌아가며 한글자씩 읽음.. 공백문자인경우 널로 채워 나누고 나눠진 토큰을 배열에 저장.. 토큰화
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

// 입력받은 명령어를 실행(fork + exec). 
void ssu_system(char **commands)
{
	int pid, i;
	int status;
	int p[2];
	int startPoint[MAX_NUM_TOKENS] = {0, };
	char cmd[][MAX_TOKEN_SIZE] = {0, };
	
	// '|' 단위로 명령어 분리. 각 명령어의 시작점.. 명령어 개수 구하기.

	int cnt = 0; // pipe 갯수

	startPoint[cnt] = 0;
	
	for(i = 0; commands[i] != NULL; i++){
		if (!strcmp(commands[i],"|")){
			cnt++;
		}
	}
	
	int j = 0;
	// (pipe 수 + 1) 만큼 for문..
	if (cnt > 0){

		int fd_in = 0;

		// 파이프마다 실행..
		for (i = 0; i <= cnt; i++,j++){
			char **cmd;
			char line[MAX_INPUT_SIZE];            
	
			// 파이프 개통
			if (pipe(p) < 0){
				fprintf(stderr, "pipe error\n");
				exit(1);
			}

			// 명령어 분할.. 
			bzero(line, sizeof(line));
			for (;commands[j]!= NULL; j++){
				if (!strcmp(commands[j], "|"))
					break;
				sprintf(line, "%s %s", line, commands[j]);
			}
			line[strlen(line)] = '\n';
			cmd = tokenize(line);
	
			// 프로세스 생성 및 실행
			if ((pid = fork()) < 0){
				fprintf(stderr, "fork error\n");
				exit(1);
			}else if (pid == 0){
				// 이전 프로세스에서 p[1]로 출력한 값을 p[0]으로 받아 입력받음.. 
				dup2(fd_in, 0);
				if (i != cnt)
					dup2(p[1], 1);
				close(p[0]);

				// 명령어 수행..
				if (execvp(cmd[0], cmd) < 0){
					printf("SSUShell : Incorrect command\n");
					exit(1);
				}
			}else{
				// exec 종료 상태값 받아오기 및 이전 프로세스 실행값 저장..
				wait(&status);
				close(p[1]);
				fd_in = p[0];
				if (WIFEXITED(status)){
					//printf("정상종료\n");
				}else{
					//printf("자식 프로세스가 비정상적으로 종료되었습니다.\n");
					//printf("신호 : %d\n", WTERMSIG(status));
				}
			}
		}
	}else{ // 파이프 없는 명령어일 경우,
		pid = fork();
		if (pid < 0){
			fprintf(stderr, "fork error\n");
			return;
		}else if (pid == 0){
			if (execvp(commands[0], commands) < 0){
				printf("SSUShell : Incorrect command\n");
				exit(1);
			}
		}else {
			wait(&status);
			if (WIFEXITED(status)){
//				printf("정상종료\n");
			}else{
//				printf("자식 프로세스가 비정상적으로 종료되었습니다.\n");
//				printf("신호 : %d\n", WTERMSIG(status));
				return;
			}
		}
	}
	return;
}
int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	// 환경변수PATH에 현재 디렉토리 추가
	char *env = getenv("PATH");
	if (env)
		sprintf(env, "%s:%s", env, getcwd(0,0));

	FILE* fp;
	if(argc == 2) {
		fp = fopen(argv[1],"r");
		if(fp < 0) {
			printf("File doesn't exists.");
			return -1;
		}
	}

	while(1) {			
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		if(argc == 2) { // batch mode
			if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
				break;	
			}
			line[strlen(line) - 1] = '\0';
			fflush(fp);

		} else { // interactive mode
			printf("$ ");
			scanf("%[^\n]", line);
			getchar();
		}

	//	printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
   
       //do whatever you want with the commands, here we just print them

	//	for(i=0;tokens[i]!=NULL;i++){
	//		printf("found token %s (remove this debug output later)\n", tokens[i]);
	//	}
       
		ssu_system(tokens);
		// 1. fork - exec - "$path" 으로 명령어 구현.. 나머지 뒤에 토큰을 인자로 삼음
		// 2. pipe 구현하여 여러 명령어 한번에 쓸 수 있도록..
		// 3. 사용자 명령어 구현

		// 1. 현재 입력받은 토큰배열에서 '|' 토큰 있는지 확인
		// 2. 없다면 현재 명령어 fork() -> exec() -> wait.. 실행 완료
		// 3. 있다면 (다중 파이프) '|' 까지의 배열들 문자열로 묶어 execlp 로 실행. 실행 결과 값을 파이프 이후 명령어에 전달
		// 4. 파이프 다음 명령을 실행. 입력fd에 이전 출력값을 넣어줌. 

		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);

	}
	return 0;
}
