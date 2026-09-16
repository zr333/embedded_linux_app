#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/*
    beep_trigger.c

    通过命令行参数控制蜂鸣器（位于 /sys/class/leds/beep）
    功能：
      - on/off: 直接设置 brightness
      - trigger none|heartbeat|timer <delay_on_ms> <delay_off_ms>

    程序在每次写入后会关闭文件描述符（释放资源），并对错误进行检查。
*/

#define LED_BASE "/sys/class/leds/beep"
#define PATH(p) LED_BASE "/" p

/*
 * 将字符串写入指定 sysfs 路径并关闭 fd。
 * 返回 0 表示成功，-1 表示失败（并打印错误）。
 */
static int write_str_to(const char *path, const char *s)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        return -1;
    }

    /* 写入内容（不含终止符） */
    ssize_t n = write(fd, s, strlen(s));
    if (n < 0)
    {
        fprintf(stderr, "write %s: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    if ((size_t)n != strlen(s))
    {
        fprintf(stderr, "short write %s: wrote %zd of %zu\n", path, n, strlen(s));
        close(fd);
        return -1;
    }

    /* 关闭 fd，确保资源释放 */
    if (close(fd) < 0)
    {
        fprintf(stderr, "close %s: %s\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    /* 参数检查：至少需要一个子命令 */
    if (argc < 2)
    {
        fprintf(stderr,
                "Usage:\n"
                "  %s on\n"
                "  %s off\n"
                "  %s trigger none|heartbeat|timer <delay_on_ms> <delay_off_ms>\n",
                argv[0], argv[0], argv[0]);
        return 1;
    }

    /* "on"：先禁用 trigger，再写亮度为 1 */
    if (strcmp(argv[1], "on") == 0)
    {
        if (write_str_to(PATH("trigger"), "none") < 0)
            return 1;
        return write_str_to(PATH("brightness"), "1");
    }

    /* "off"：同上，写亮度为 0 */
    else if (strcmp(argv[1], "off") == 0)
    {
        if (write_str_to(PATH("trigger"), "none") < 0)
            return 1;
        return write_str_to(PATH("brightness"), "0");
    }

    /* "trigger"：支持 none/heartbeat/timer */
    else if (strcmp(argv[1], "trigger") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "trigger type required\n");
            return 1;
        }

        if (strcmp(argv[2], "none") == 0)
        {
            /* 清除触发器 */
            return write_str_to(PATH("trigger"), "none");
        }
        else if (strcmp(argv[2], "heartbeat") == 0)
        {
            /* 心跳效果：由驱动负责实现 */
            return write_str_to(PATH("trigger"), "heartbeat");
        }
        else if (strcmp(argv[2], "timer") == 0)
        {
            /* timer 需要两个参数，分别为 delay_on 和 delay_off（毫秒） */
            if (argc < 5)
            {
                fprintf(stderr, "timer requires <delay_on_ms> <delay_off_ms>\n");
                return 1;
            }
            /* 设置触发器为 timer，然后写延迟文件 */
            if (write_str_to(PATH("trigger"), "timer") < 0)
                return 1;
            if (write_str_to(PATH("delay_on"), argv[3]) < 0)
                return 1;
            if (write_str_to(PATH("delay_off"), argv[4]) < 0)
                return 1;
        }
        else
        {
            fprintf(stderr, "unknown trigger type: %s\n", argv[2]);
            return 1;
        }
    }

    else
    {
        fprintf(stderr, "unknown command\n");
        return 1;
    }

    return 0;
}
