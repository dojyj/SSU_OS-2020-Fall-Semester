#include "ssufs-ops.h"

int main()
{
    char str[] = "!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!!-16 Bytes Data!";
	char str2[] = "!-16 Bytes Data!";
	char read_str[BLOCKSIZE] = {0, };
    ssufs_formatDisk();

    ssufs_create("f1.txt");
	ssufs_create("f2.txt");
    ssufs_create("f3.txt");
	ssufs_create("f4.txt");
    ssufs_create("f5.txt");
	ssufs_create("f6.txt");
    ssufs_create("f7.txt");
	ssufs_create("f8.txt");
    int fd1 = ssufs_open("f1.txt");
	int fd2 = ssufs_open("f2.txt");
    int fd3 = ssufs_open("f3.txt");
	int fd4 = ssufs_open("f4.txt");
    int fd5 = ssufs_open("f5.txt");
	int fd6 = ssufs_open("f6.txt");
    int fd7 = ssufs_open("f7.txt");
	int fd8 = ssufs_open("f8.txt");

	for (int i = 0; i < 3; i++){
    	printf("Write Data: %d\n", ssufs_write(fd1, str, strlen(str)));
    	printf("Write Data: %d\n", ssufs_write(fd2, str, strlen(str)));
    	printf("Write Data: %d\n", ssufs_write(fd3, str, strlen(str)));
    	printf("Write Data: %d\n", ssufs_write(fd4, str, strlen(str)));
    	printf("Write Data: %d\n", ssufs_write(fd5, str, strlen(str)));
    	printf("Write Data: %d\n", ssufs_write(fd6, str, strlen(str)));
    	printf("Write Data: %d\n", ssufs_write(fd7, str, strlen(str)));
    	printf("Write Data: %d\n", ssufs_write(fd8, str, strlen(str)));
	}

   	printf("Write Data: %d\n", ssufs_write(fd1, str2, strlen(str2)));
   	printf("Write Data: %d\n", ssufs_write(fd2, str2, strlen(str2)));
   	printf("Write Data: %d\n", ssufs_write(fd3, str2, strlen(str2)));
   	printf("Write Data: %d\n", ssufs_write(fd4, str2, strlen(str2)));
   	printf("Write Data: %d\n", ssufs_write(fd5, str2, strlen(str2)));
   	printf("Write Data: %d\n", ssufs_write(fd6, str2, strlen(str2)));
   	printf("Write Data: %d\n", ssufs_write(fd7, str2, strlen(str2)));
   	printf("Write Data: %d\n", ssufs_write(fd8, str2, strlen(str2)));

    printf("Seek: %d\n", ssufs_lseek(fd1, -128));
    printf("Seek: %d\n", ssufs_lseek(fd2, -128));
    printf("Seek: %d\n", ssufs_lseek(fd3, -128));
    printf("Seek: %d\n", ssufs_lseek(fd4, -128));
    printf("Seek: %d\n", ssufs_lseek(fd5, -128));
    printf("Seek: %d\n", ssufs_lseek(fd6, -128));
    printf("Seek: %d\n", ssufs_lseek(fd7, -128));
    printf("Seek: %d\n", ssufs_lseek(fd8, -128));

	printf("read: %d %s\n", ssufs_read(fd1, read_str, 2), read_str);
	printf("read: %d %s\n", ssufs_read(fd2, read_str, 4), read_str);
	printf("read: %d %s\n", ssufs_read(fd3, read_str, 6), read_str);
	printf("read: %d %s\n", ssufs_read(fd4, read_str, 8), read_str);
	printf("read: %d %s\n", ssufs_read(fd5, read_str, 10), read_str);
	printf("read: %d %s\n", ssufs_read(fd6, read_str, 12), read_str);
	printf("read: %d %s\n", ssufs_read(fd7, read_str, 14), read_str);
	printf("read: %d %s\n", ssufs_read(fd8, read_str, 16), read_str);

    ssufs_dump();
    ssufs_delete("f1.txt");
    ssufs_delete("f2.txt");
    ssufs_delete("f3.txt");
    ssufs_delete("f4.txt");
    ssufs_delete("f5.txt");
    ssufs_delete("f6.txt");
    ssufs_delete("f7.txt");
    ssufs_delete("f8.txt");
    ssufs_dump();
}
