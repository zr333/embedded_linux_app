# test_beep — 蜂鸣器控制

通过 sysfs 文件系统控制 i.MX6ULL 蜂鸣器（`/sys/class/leds/beep/`）的响停与触发模式。

## 源文件说明

| 文件 | 功能 |
|------|------|
| `beep.c` | 简单蜂鸣器：响 1 秒、停 2 秒循环 |
| `beep_main.c` | 蜂鸣器基本响停控制 |
| `beep_trigger.c` | 演示 trigger 机制，支持 `on/off` 和 `trigger <type>` 两种用法 |

## 编译

```bash
arm-linux-gnueabihf-gcc beep.c -o beep
arm-linux-gnueabihf-gcc beep_main.c -o beep_main
arm-linux-gnueabihf-gcc beep_trigger.c -o beep_trigger
```

## 运行

```bash
./beep                             # 循环响停
./beep_trigger on                  # 常响
./beep_trigger off                 # 停止
./beep_trigger trigger heartbeat   # 使用内核 heartbeat 触发器
```

## 原理要点

与 LED 类似，蜂鸣器也被抽象为 `/sys/class/leds/beep/` 下的文件：

- `brightness`：写入 `1` 响、`0` 停。
- `trigger`：设置触发模式（如 `heartbeat`、`timer`），由内核自动控制。
