/*****************************************
本程序实现按键控制LED灯的亮灭，按键按下时灯亮
按键松开时LED灯灭
*****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#define LED_BRIGHTNESS "/sys/class/leds/sys-led/brightness"

int main(int argc, char *argv[])
{
    struct input_event in_ev = {0};
    int fd_key = -1;
    int fd_LED = -1;
    int value = -1;
    /* 校验传参 */
    if (2 != argc)
    {
        fprintf(stderr, "usage: %s <input-dev>\n", argv[0]);
        exit(-1);
    }
    /* 打开文件 */
    if (0 > (fd_key = open(argv[1], O_RDONLY)))
    {
        perror("open error");
        exit(-1);
    }
    fd_LED = open(LED_BRIGHTNESS, O_RDWR); // 以可读可写的方式打开LED驱动文件
    if (fd_LED < 0)                        // 如果打不开设备就打印相应的错误
    {
        perror("open error");
        exit(-1);
    }

    write(fd_LED, "0", 1); // 初始化LED灯为灭状态，“0”代表灭，后面的1是指写入的字节数

    /* 循环读取数据 */
    for (;;)
    {
        if (sizeof(struct input_event) !=
            read(fd_key, &in_ev, sizeof(struct input_event)))
        {
            perror("read error");
            exit(-1);
        }
        if (EV_KEY == in_ev.type)
        { // 按键事件
            switch (in_ev.value)
            {
            case 0:
                printf("code<%d>: 松开\n", in_ev.code);
                write(fd_LED, "0", 1); // 点亮LED灯，“0”代表灭
                break;
            case 1:
                printf("code<%d>: 按开\n", in_ev.code);
                write(fd_LED, "1", 1); // 点亮LED灯，“1”代表点亮
                break;
            case 2:
                printf("code<%d>: 长按\n", in_ev.code);
                write(fd_LED, "1", 1); 
                break;
            }
        }
    }

    close(fd_key);
    close(fd_LED);
    return 0;
}
