# test_ts — 触摸屏（tslib）

使用 tslib 库读取触摸屏数据，演示单点触摸和多点触摸（MT）。

## 文件说明

| 文件 | 功能 |
|------|------|
| `ts_read.c` | 单点触摸：读取采样数据，根据 `pressure` 判断按下/移动/松开 |
| `ts_read_mt.c` | 多点触摸：按 slot 读取并打印多个触摸点的状态 |
| `编译指令.md` | 交叉编译命令说明 |

## 编译

需要先交叉编译并安装 tslib 库，然后指定头文件与库路径：

```bash
arm-linux-gnueabihf-gcc \
    -I /home/zr-arm/tools/tslib/include \
    -L /home/zr-arm/tools/tslib/lib \
    -l ts \
    -o ts_read ts_read.c

arm-linux-gnueabihf-gcc \
    -I /home/zr-arm/tools/tslib/include \
    -L /home/zr-arm/tools/tslib/lib \
    -l ts \
    -o ts_read_mt ts_read_mt.c
```

## 运行

```bash
# 可通过环境变量指定设备节点（可选）
export TSLIB_TSDEVICE=/dev/input/event1

./ts_read       # 单点触摸
./ts_read_mt    # 多点触摸
```

## 原理要点

- `ts_setup(NULL, 0)` 初始化并打开触摸设备。
- `ts_read()` 阻塞读取单点采样；多点则使用 `struct ts_sample_mt` 数组。
- 通过 `pressure` 字段判断触摸状态，`ioctl(EVIOCGABS(ABS_MT_SLOT))` 获取支持的触点数量。

详见同目录下的 `编译指令.md`。
