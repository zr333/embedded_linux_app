#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>

/*
读取 /dev/input/event* 设备的按键事件，并打印按键代码和状态。
*/

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input_event_device>\n", argv[0]);
        exit(-1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("Failed to open input event device");
        exit(-1);
    }

    // 循环读取按键事件
    for(;;)
    {
        struct input_event in_ev;
        if (sizeof(struct input_event) !=
            read(fd, &in_ev, sizeof(struct input_event))) // 阻塞读取
        {
            perror("Failed to read input event");
            close(fd);
            exit(-1);
        }

        // 无限循环读取按键事件

        if (EV_KEY == in_ev.type) // 按键事件判断
        {
            switch (in_ev.value)
            {
            case 0: // 按键释放
                printf("释放: %d\n", in_ev.code);
                break;
            case 1: // 按键按下
                printf("按下: %d\n", in_ev.code);
                break;
            case 2: // 按键长按
                printf("长按: %d\n", in_ev.code);
                break;
            default:
                printf("Unknown Key State: Code: %d, Value: %d\n", in_ev.code, in_ev.value);
                break;
            }
        }
    }

    return 0;
}