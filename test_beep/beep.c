#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
    简单的蜂鸣器闪烁程序，通过操作/sys/class/leds/beep/brightness文件来控制蜂鸣器的响与不响状态。程序会不断循环，先响1秒，然后不响2秒。
*/

#define beep_BRIGHTNESS_PATH "/sys/class/leds/beep/brightness"

int main()
{
    int fd = open(beep_BRIGHTNESS_PATH, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open brightness file");
        return 1;
    }
    while (1)
    {
        write(fd, "1", 1); // Turn on the LED
        sleep(1);          // Wait for 1 second

        write(fd, "0", 1); // Turn off the LED
        sleep(2);          // Wait for 2 seconds
    }

    return 0;
}