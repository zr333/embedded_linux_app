#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/*
    增加triger功能,可以通过传入triger参数来控制LED灯的亮灭状态。
*/

#define LED_BRIGHTNESS_PATH "/sys/class/leds/sys-led/brightness"
#define LED_TRIGGER_PATH "/sys/class/leds/sys-led/trigger"
#define USAGE() fprintf(stderr, "Usage:\n"               \
                                " %s <on|off>\n"         \
                                " %s <triger> <type>\n", \
                        argv[0], argv[0]); // 提示输入参数错误

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        USAGE();
        return 1;
    }

    int fd1 = open(LED_BRIGHTNESS_PATH, O_RDWR);
    if (fd1 < 0)
    {
        perror("Failed to open brightness file");
        return 1;
    }
    int fd2 = open(LED_TRIGGER_PATH, O_RDWR);
    if (fd2 < 0)
    {
        perror("Failed to open trigger file");
        close(fd1);
        return 1;
    }

    if (!strcmp(argv[1], "on"))
    {
        // 先将触发器设置为none，然后再点亮LED
        write(fd2, "none", 4);
        write(fd1, "1", 1);
    }
    else if (!strcmp(argv[1], "off"))
    {
        // 先将触发器设置为none，然后再熄灭LED
        write(fd2, "none", 4);
        write(fd1, "0", 1);
    }
    else if (!strcmp(argv[1], "trigger"))
    {
        if (argc < 3)
        {
            USAGE();
            exit(1);
        }
        if (write(fd2, argv[2], strlen(argv[2])) < 0)
        {
            perror("Failed to set trigger");
            close(fd1);
            close(fd2);
            return 1;
        }
    }

    else
    {
        USAGE();
        close(fd1);
        close(fd2);
        return 1;
    }

    close(fd1);
    close(fd2);

    USAGE();
    return 0;
}