#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
    该程序通过带参主程序, 通过传入参数"on"或"off"来控制LED灯的亮灭状态。
*/

#define LED_BRIGHTNESS_PATH "/sys/class/leds/sys-led/brightness"

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <on|off>\n", argv[0]); // 提示输入参数错误
        return 1;
    }

    int fd = open(LED_BRIGHTNESS_PATH, O_WRONLY);
    if (fd < 0)
    {
        perror("Failed to open brightness file");
        return 1;
    }

    if (!strcmp(argv[1], "on"))
    {
        if (write(fd, "1", 1) < 0)
        {
            perror("Failed to turn on LED");
            close(fd);
            return 1;
        }
    }
    else if (!strcmp(argv[1], "off"))
    {
        if (write(fd, "0", 1) < 0)
        {
            perror("Failed to turn off LED");
            close(fd);
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "Invalid argument: %s. Use 'on' or 'off'.\n", argv[1]);
        close(fd);
        return 1;
    }


    close(fd);

    return 0;
}