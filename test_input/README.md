# test_input — 按键输入（Input 子系统）

通过 Linux input 子系统读取 `/dev/input/event*` 按键事件，识别按键按下/松开，并演示按键控制 LED。

## 文件说明

| 文件 | 功能 |
|------|------|
| `key.c` | 读取按键事件并打印 `type`/`code`/`value` |
| `key_value.c` | 读取按键事件，重点演示 `value` 的按下/松开判断 |
| `keyControl_led.c` | 按键控制 LED：按下点亮、松开熄灭 |
| `input_event_notes.md` | Input 子系统事件结构学习笔记 |

## 编译

```bash
arm-linux-gnueabihf-gcc key.c -o key
arm-linux-gnueabihf-gcc key_value.c -o key_value
arm-linux-gnueabihf-gcc keyControl_led.c -o keyControl_led
```

## 运行

```bash
# 先确认按键对应的设备节点
cat /proc/bus/input/devices

./key /dev/input/event0
./key_value /dev/input/event0
./keyControl_led /dev/input/event0
```

## 原理要点

- 按键事件用 `struct input_event` 表示，含 `type`（事件类型）、`code`（具体按键）、`value`（按下/松开）。
- `EV_KEY` 类型中，`value=1` 表示按下，`value=0` 表示松开。
- 通过 `read()` 阻塞读取事件，解析后执行相应操作。

详见同目录下的 `input_event_notes.md`。
