#include "ssufs-ops.h"

int main()
{
    char str[] = "!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!";
    char buf[300];
    memset(buf,0,sizeof(buf));
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

    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd1, str, 16));
        ssufs_dump();
    }
    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd2, str, 16));
        ssufs_dump();
    }
    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd3, str, 16));
        ssufs_dump();
    }
    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd4, str, 16));
        ssufs_dump();
    }
    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd5, str, 16));
        ssufs_dump();
    }
    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd6, str, 16));
        ssufs_dump();
    }
    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd7, str, 16));
        ssufs_dump();
    }
    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd8, str, 16));
        ssufs_dump();
    }

    printf("Seek: %d\n", ssufs_lseek(fd1, -256));
    for(int i=0;i<20;i++){
   int ret = ssufs_read(fd1,buf,16);
        printf("%d Read : %d\n",i, ret);
    if(ret < 0)
       printf("No Read Data\n");
   else
            printf("Read Data: %s\n", buf);
    }
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

