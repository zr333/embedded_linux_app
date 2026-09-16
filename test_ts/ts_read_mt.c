#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <tslib.h>
#include <errno.h>
#include <string.h>

/*
 * 示例程序：读取多点触摸（MT）数据并按 slot 打印按下/移动/松开事件。
 */

int main(int argc, char *argv[])
{
    struct tsdev *ts = NULL;            /* tslib 触摸设备句柄 */
    struct ts_sample_mt *mt_ptr = NULL; /* 指向多点触摸样本数组的指针 */
    struct input_absinfo slot;          /* 用于 ioctl 获取 ABS_MT_SLOT 的范围信息 */
    int max_slots;                      /* 设备支持的 slot 数（最大触摸点数量） */
    unsigned int *pressure = NULL;      /* 每个 slot 上上一次的 pressure（初始为 0 表示松开） */

    /* 打开并配置触摸屏设备（默认设备与环境由 tslib 配置） */
    ts = ts_setup(NULL, 0);
    if (NULL == ts)
    {
        /* ts_setup 失败后直接退出，真实项目中可改为重试或更详细的错误信息 */
        fprintf(stderr, "ts_setup error");
        exit(EXIT_FAILURE);
    }

    /*
     * 通过 ioctl 获取触摸屏支持的 slot 信息。
     * EVIOCGABS(ABS_MT_SLOT) 会把 slot 的最小/最大 index 写入到 struct input_absinfo 中。
     * struct input_absinfo 的成员：
     *   value: 最近报告值
     *   minimum: 最小值（slot 最小索引）
     *   maximum: 最大值（slot 最大索引）
     * 注意：某些设备可能不支持多点或返回异常值，调用方应检查返回结果与后续使用时的边界。
     */
    if (0 > ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot))
    {
        perror("ioctl error");
        ts_close(ts);
        exit(EXIT_FAILURE);
    }

    /* 计算可用 slot 数量（inclusive 范围） */
    max_slots = slot.maximum + 1 - slot.minimum;

    /* 为 pressure 分配内存，保存每个 slot 上次的压力值（初始为 0） */
    pressure = calloc(max_slots, sizeof(unsigned int));
    if (pressure == NULL)
    {
        /* calloc 失败则释放资源并退出 */
        fprintf(stderr, "calloc pressure failed: %s\n", strerror(errno));
        ts_close(ts);
        return EXIT_FAILURE;
    }

    /* 为多点触摸样本分配数组，数组长度为 max_slots（每个 slot 一个 struct） */
    mt_ptr = calloc(max_slots, sizeof(struct ts_sample_mt));
    if (mt_ptr == NULL)
    {
        /* 若分配失败，释放之前的内存并退出 */
        perror("calloc");
        free(pressure);
        ts_close(ts);
        exit(EXIT_FAILURE);
    }

    printf("max_slots: %d\n", max_slots);

    /* 主循环：持续读取多点触摸样本并处理 */
    while (1)
    {
        /*
         * ts_read_mt 的常见原型：
         *   int ts_read_mt(struct tsdev *ts, struct ts_sample_mt **samp, int max_slots, int nr)
         * 说明（阅读本段注释即可，不修改调用）：
         * - ts: tslib 设备句柄
         * - samp: 指向 struct ts_sample_mt* 的指针，调用方可传入指向已分配数组的指针地址
         * - max_slots: 传入的数组最大元素数（避免越界）
         * - nr: 要读取的“帧”数量或调用约定参数（此例传 1 表示读取单次更新）
         *
         * 返回值约定：一般 <0 表示错误（perror 打印），>=0 表示读取成功（返回读取的样本数或事件数）。
         * 注意：不同版本的 tslib 实现细节略有差异，调用者应参考当前系统的 tslib 文档/源码确认精确语义。
         */
        if (0 > ts_read_mt(ts, &mt_ptr, max_slots, 1))
        {
            /* 读取出错，打印并清理后退出 */
            perror("ts_read_mt error");
            ts_close(ts);
            free(mt_ptr);
            exit(EXIT_FAILURE);
        }

        /* 遍历每个 slot（0..max_slots-1），检查是否有有效更新 */
        
        for (int i = 0; i < max_slots; i++)
        {
            /* valid 表示该 slot 本次有更新（按下/移动/松开等） */
            if (mt_ptr[i].valid)
            {
                /* pressure > 0 表示当前为按下或接触状态 */
                if (mt_ptr[i].pressure)
                {
                    if (pressure[mt_ptr[i].slot]) /* 上一次该 slot 也有压力，则判定为移动 */
                        printf("slot<%d>, 移动(%d, %d)\n", mt_ptr[i].slot, mt_ptr[i].x, mt_ptr[i].y);
                    else /* 上一次无压力，则为新按下事件 */
                        printf("slot<%d>, 按下(%d, %d)\n", mt_ptr[i].slot, mt_ptr[i].x, mt_ptr[i].y);
                }
                else
                    /* pressure == 0 表示该 slot 被松开 */
                    printf("slot<%d>, 松开\n", mt_ptr[i].slot);

                /* 更新保存的上一次压力值（用于下次判断按下/移动） */
                pressure[mt_ptr[i].slot] = mt_ptr[i].pressure;
            }
        }
    }
    /* 程序正常结束时释放资源 */
    free(mt_ptr);
    free(pressure);
    ts_close(ts);
    return 0;
}
