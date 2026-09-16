#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
    简单的LED闪烁程序，序通过操作/sys/class/leds/sys-led/brightness文件来控制LED的亮灭状态。程序会不断循环，先点亮LED持续1秒，然后熄灭LED持续2秒。
*/

#define LED_BRIGHTNESS_PATH "/sys/class/leds/sys-led/brightness"

int main()
{
    int fd = open(LED_BRIGHTNESS_PATH, O_WRONLY);
    if (fd < 0)
    {
        perror("Failed to open brightness file");
        return 1;   
    }
    while(1)
    {
        write(fd, "1", 1); // Turn on the LED
        sleep(1); // Wait for 1 second

        write(fd, "0", 1); // Turn off the LED
        sleep(2); // Wait for 2 seconds
    }
    
    return 0;

}