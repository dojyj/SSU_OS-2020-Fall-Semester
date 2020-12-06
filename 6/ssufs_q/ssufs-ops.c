#include "ssufs-ops.h"

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int ssufs_allocFileHandle() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if (file_handle_array[i].inode_number == -1) {
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename){
	int inode_idx;
	struct inode_t *tmp = (struct inode_t *)malloc(sizeof(struct inode_t));

	// 동일한 이름의 파일이 파일 시스템에 존재한다면, return -1
	if (open_namei(filename) != -1){
		fprintf(stderr, "create error : filename \"%s\" already exists\n", filename);
		free(tmp);
		return -1;
	}

	// inode_freelist에 비어있는 노드가 없다면, return -1
	if ((inode_idx = ssufs_allocInode()) == -1){
		fprintf(stderr, "create error : there is no index to alloc inode\n");
		return -1;
	}

	// 할당한 inode 정보 채우기
	ssufs_readInode(inode_idx, tmp);
	tmp->status = INODE_IN_USE;
	memcpy(tmp->name, filename, strlen(filename));
	ssufs_writeInode(inode_idx, tmp);

	return inode_idx;
}

void ssufs_delete(char *filename){
	int inode_idx;

	// 해당 파일이 존재하지 않는다면.., return -1
	if ((inode_idx = open_namei(filename)) == -1){
		fprintf(stderr, "delete error : filename \"%s\" doesn't exist\n", filename);
		return;
	}

	// 해당 inode 구조체 free
	ssufs_freeInode(inode_idx);

	return;
}

int ssufs_open(char *filename){
	int inode_idx, handle_idx;

	// 해당 파일이 존재하지 않는다면.., return -1
	if ((inode_idx = open_namei(filename)) == -1){
		fprintf(stderr, "open error : filename \"%s\" doesn't exist\n", filename);
		return -1;
	}

	// 사용하지 않는 파일 핸들 찾기, 빈 핸들이 없다면 에러처리
	if ((handle_idx = ssufs_allocFileHandle()) == -1){
		fprintf(stderr, "open error : there is no handler\n");
		return -1;
	}

	file_handle_array[handle_idx].inode_number = inode_idx;

	// 파일의 inode번호 리턴
	return handle_idx;  
}


void ssufs_close(int file_handle){
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes){

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	int offset;
	int cur_offset;
	int have_to_read = nbytes;
	int have_read = 0;
	int cur_idx = 0;
	int datablock_idx[NUM_INODE_BLOCKS];

	// inodeptr 가져오기
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	// 데이터가 여러 블록에 걸쳐 있을 수 있음.. => 데이터 블록 경계를 넘어가는 경우.. 읽기 반복
	while (have_to_read != 0){
		// 읽어야 할 offset 가져오기
		offset = file_handle_array[file_handle].offset;
		cur_offset = offset % BLOCKSIZE;

		// 요청된 nbytes를 읽으면 파일 끝을 넘어갈 경우.. 에러 리턴
		if (offset + have_to_read > BLOCKSIZE * MAX_FILE_SIZE){
			fprintf(stderr, "read error : there are only %d bytes for read\n", (BLOCKSIZE * MAX_FILE_SIZE) - offset);
			return -1;
		}
		
		// 이번 datablock에서 읽을 크기 구하기
		int will_read;
		if (have_to_read + cur_offset > BLOCKSIZE)
			will_read = BLOCKSIZE - cur_offset;
		else
			will_read = have_to_read;

		have_to_read -= will_read;
		char *temp_buf;

		// 현재 datablock읽어서 지금까지 읽어온 문자열(buf)에 붙이기
		if (cur_offset != 0){
			datablock_idx[cur_idx] = tmp->direct_blocks[offset / BLOCKSIZE];
			temp_buf = (char *)malloc(sizeof(char) * BLOCKSIZE);
		}else{
			datablock_idx[cur_idx] = tmp->direct_blocks[offset / (BLOCKSIZE + 1)];
			temp_buf = (char *)malloc(sizeof(char) * BLOCKSIZE);
		}
		ssufs_readDataBlock(datablock_idx[cur_idx], temp_buf);
		memcpy(buf + have_read, temp_buf + cur_offset, will_read);
		buf[have_read + will_read] = '\0';
		have_read += will_read;

		// offset 갱신
		file_handle_array[file_handle].offset += will_read;

		free(temp_buf);
		cur_idx++;
	}
		
	return 0;
}


int ssufs_write(int file_handle, char *buf, int nbytes){

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	int offset;
	int cur_offset;
	int datablock_idx[NUM_INODE_BLOCKS]; 
	int have_to_write = nbytes;  
	int have_written = 0;
	int cur_idx = 0;
	int overwrite = 0;

	// inodeptr 가져오기
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	// debug
	// printf("\n===== write start ===== \n");
	if (nbytes <= 0){
		fprintf(stderr, "warning : are you sure write %dbytes of data?\n", nbytes);
		return -1;
	}

	// 데이터가 여러 블록에 걸쳐 있을 수 있음 => 데이터 블록 경계를 넘어가는 경우.. 쓰기 반복
	while (have_to_write != 0){

		// 써야 할 offset 가져오기
		offset = file_handle_array[file_handle].offset;
		cur_offset = offset % BLOCKSIZE;

		// 요청된 nbytes를 쓸 수 없는 경우, 에러 리턴
		if (offset + have_to_write > BLOCKSIZE * MAX_FILE_SIZE){
			fprintf(stderr, "write error : there are only %d bytes for write\n", (BLOCKSIZE * MAX_FILE_SIZE) - offset);
			return -1;
		}

		// 이번 data block에 쓸 문자열 구하기
		int will_write;
		if (have_to_write + cur_offset > BLOCKSIZE)
			will_write = BLOCKSIZE - cur_offset;
		else 
			will_write = have_to_write;

		//debug
		// printf("have to write : %d, will write : %d\n", have_to_write, will_write);

		have_to_write -= will_write;

		char *temp_buf;

		// 쓰다만 데이터 블럭이 있다면 가져오기.. ( 블록 중간에서 시작 가능 )
		if (cur_offset != 0){
			datablock_idx[cur_idx] = tmp->direct_blocks[offset / BLOCKSIZE]; 
			temp_buf = (char *)malloc(sizeof(char) * BLOCKSIZE);
			ssufs_readDataBlock(datablock_idx[cur_idx], temp_buf);
			memcpy(temp_buf + cur_offset, buf + have_written, will_write);
			temp_buf[cur_offset + will_write] = '\0';
			overwrite++;
		}
		else {
			// 쓸 datablock이 남아있다면 가져오기.. 없다면? -> 지금까지 쓴 데이터 원상 복구 및 종료
			if ((datablock_idx[cur_idx] = ssufs_allocDataBlock()) == -1){
				offset -= have_written;
				file_handle_array[file_handle].offset = offset;
				cur_offset = offset % BLOCKSIZE;
				for (int i = 0; i < cur_idx; i++){
					// 작성한 데이터 원상복구
					temp_buf = (char *)malloc(sizeof(char) * BLOCKSIZE);
					ssufs_readDataBlock(datablock_idx[i], temp_buf);
					temp_buf[cur_offset] = '\0';
					ssufs_writeDataBlock(datablock_idx[i], temp_buf);
					free(temp_buf);
				}
				fprintf(stderr, "write error : there is no remain data block\n");
				return -1;
			}

			temp_buf = (char *)malloc(sizeof(char) * will_write);
			memcpy(temp_buf, buf + have_written,  will_write);
			temp_buf[will_write] = '\0';
		}
		have_written += will_write;
	
		// debug
		// printf("file%d cur offset : %d, now ( str : %s, size : %d ) will be written\n",file_handle, offset, temp_buf, cur_offset + will_write);

		// 쓰기 ( data block에 쓰고 indirect block이 참조하게 함 )
		ssufs_writeDataBlock(datablock_idx[cur_idx], temp_buf);
		cur_idx++;
		file_handle_array[file_handle].offset += will_write;
		
		// debug
		// printf("offset moved to %d, cur_idx : %d\n", offset + will_write, cur_idx);

		free(temp_buf);
	}

	// 모든 데이터를 data block에 쓰기 완료 후, inode update.
	int idx;
	for (int i = 0, idx = overwrite; idx < cur_idx; i++){
		if (tmp->direct_blocks[i] == -1){
			tmp->direct_blocks[i] = datablock_idx[idx];
			idx++;
		}
	}
	tmp->file_size += nbytes;
	ssufs_writeInode(file_handle_array[file_handle].inode_number, tmp);

	// debug
	// printf("===== write end ===== \n\n");

	free(tmp);
	return 0;
}

int ssufs_lseek(int file_handle, int nseek){
	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);
	
	int fsize = tmp->file_size;
	
	offset += nseek;

	// debug
	// printf("fsize : %d, offset : %d\n", fsize, offset);

	if ((fsize == -1) || (offset < 0) || (offset > fsize)) {
		free(tmp);
		return -1;
	}

	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}
