#include <stdio.h>
#include <stdlib.h>
#include <tslib.h>

/*
 * 简要说明：
 * 该程序使用 tslib 库从触摸屏设备读取采样数据(单点触摸)，并根据采样的 pressure 字段
 * 判断触摸状态：按下、移动或松开。输出简要的坐标或状态信息到标准输出。
 *
 * 运行方式：可通过设置环境变量 TSLIB_TSDEVICE 指定设备节点，或让 tslib 使用默认。
 */

int main(int argc, char *argv[])
{
    /* 触摸设备句柄（由 tslib 提供的抽象结构） */
    struct tsdev *ts = NULL;
    /* 存放单次触摸采样数据（x, y, pressure, tv） */
    struct ts_sample samp;
    /* 记录上一次采样的 pressure，用于判断状态变化 */
    int last_pressure = 0;
    /* 当前采样的 pressure */
    int current_pressure = 0;

    /*
     * 打开并配置触摸屏设备句柄
     * ts_setup(NULL, 0) 会读取环境变量 TSLIB_TSDEVICE（如果存在）以确定设备节点，
     * nonblock=0 表示以阻塞方式打开设备（调用 ts_read 时会阻塞直到有数据）。
     */
    ts = ts_setup(NULL, 0);
    if (NULL == ts)
    {
        fprintf(stderr, "ts_setup error\n");
        exit(EXIT_FAILURE);
    }

    /* 主循环：持续读取触摸采样并根据 pressure 判断状态 */
    while (1)
    {
        /*
         * ts_read 用于读取单点触摸数据：
         *   int ts_read(struct tsdev *ts, struct ts_sample *samp, int nr)
         * 参数说明：
         *   ts   - 触摸设备句柄
         *   samp - 指向 struct ts_sample 的指针，存放读取到的数据
         *   nr   - 读取的采样数（单点触摸设为 1）
         * ts_read 在阻塞模式下会阻塞直到收到一条采样记录，返回值 < 0 表示出错。
         */
        if (0 > ts_read(ts, &samp, 1))
        {
            fprintf(stderr, "ts_read error\n");
            ts_close(ts);
            exit(EXIT_FAILURE);
        }

        current_pressure = samp.pressure;

        /* 直接打印本次和上一次 pressure 的值，方便观察状态变化 */
        printf("采样: x=%d, y=%d, pressure=%d | 上一次pressure=%d\n",
               samp.x, samp.y, current_pressure, last_pressure);
        // 松开后,samp.x, samp.y会保存最后一次接触时的坐标值,
        // 但 pressure 为 0

        /*
         * 状态判断：
         * 1. current_pressure > 0 且 last_pressure == 0  -> 按下
         * 2. current_pressure > 0 且 last_pressure > 0  -> 移动
         * 3. current_pressure == 0 且 last_pressure > 0 -> 松开
         * 4. current_pressure == 0 且 last_pressure == 0 -> 空闲/未触摸
         */
        if (current_pressure > 0 && last_pressure == 0)
        {
            printf("Status: Pressed\n");
        }
        else if (current_pressure > 0 && last_pressure > 0)
        {
            printf("Status: Moving\n");
        }
        else if (current_pressure == 0 && last_pressure > 0)
        {
            printf("Status: Released\n");
        }
        else
        {
            printf("Status: Idle (not touched)\n");
        }

        /* 保存当前 pressure，用于下一次判断 */
        last_pressure = current_pressure;
    }

    /* 理论上不会到达这里（因 while(1)），但关闭资源并退出以作保险 */
    ts_close(ts);
    exit(EXIT_FAILURE);
}
