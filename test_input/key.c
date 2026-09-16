#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<linux/input.h>


/*
读取 /dev/input/event* 设备的按键事件，并打印按键代码和状态。
*/

int main(int argc,char *argv[])
{
    if(argc<2)
    {
        fprintf(stderr,"Usage: %s <input_event_device>\n",argv[0]);
        exit(-1);
    }

    int fd=open(argv[1],O_RDONLY);
    if(fd<0)
    {
        perror("Failed to open input event device");
        exit(-1);
    }

    struct input_event in_ev;
    // 无限循环读取按键事件
    for(;;) // 相当于while(1) 无限循环
    {
        if (sizeof(struct input_event) != read(fd, &in_ev, sizeof(struct input_event))) // 阻塞读取
        {
            perror("Failed to read input event");
            close(fd);
            exit(-1);
        }

        printf("Event Type: %d, Code: %d, Value: %d\n", in_ev.type, in_ev.code, in_ev.value);
    }

    printf("Event Type: %d, Code: %d, Value: %d\n", in_ev.type, in_ev.code, in_ev.value);
    close(fd);
    return 0;
}