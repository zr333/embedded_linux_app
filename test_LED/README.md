# test_LED — LED 控制

通过 sysfs 文件系统控制 i.MX6ULL 板载 LED（`/sys/class/leds/sys-led/`）的亮灭与闪烁模式。

## 源文件说明

| 文件 | 功能 |
|------|------|
| `led.c` | 简单 LED 闪烁：点亮 1 秒、熄灭 2 秒循环 |
| `led_main.c` | LED 基本亮灭控制 |
| `led_trigger.c` | 演示 trigger 机制，支持 `on/off` 和 `trigger <type>` 两种用法 |

## 编译

```bash
arm-linux-gnueabihf-gcc led.c -o led
arm-linux-gnueabihf-gcc led_main.c -o led_main
arm-linux-gnueabihf-gcc led_trigger.c -o led_trigger
```

## 运行

```bash
./led                              # 循环闪烁
./led_trigger on                   # 常亮
./led_trigger off                  # 熄灭
./led_trigger trigger heartbeat    # 使用内核 heartbeat 触发器
```

## 原理要点

LED 被抽象为 `/sys/class/leds/sys-led/` 下的文件：

- `brightness`：写入 `1` 点亮、`0` 熄灭。
- `trigger`：设置触发模式（如 `heartbeat`、`timer`），由内核自动控制 LED。
