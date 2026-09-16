# Linux Input 子系统按键事件学习笔记

## 1. 典型用法（已按键事件为例）：

先看系统的按键配置` cat /proc/bus/input、devices`：
    - 查看当前 Linux 系统中注册的输入设备
    - 看设备名称、总线类型，以及它对应的 eventX 节点
    - 方便确认应该读取哪个 /dev/input/event* 文件
    
```bash
./key /dev/input/event0
```

其中：
- `key` 是编译后的程序名
- `/dev/input/event0` 是输入设备节点

---

## 2. 关键结构体：`struct input_event`

在 Linux 中，输入设备事件通常用下面这个结构体表示：

```c
struct input_event {
    struct timeval time; // 事件发生时间
    __u16 type;  // 事件类型
    __u16 code;  // 事件代码（例如按键代码(回车还是其他按键等)）
    __s32 value; //事件值（例如按下/释放、相对位移、绝对位置）
};
```

---

## 3. 常见 `type` 类型

`type` 表示事件大类，常见值包括：

```c
#define EV_SYN       0x00 // 同步事件，通常用于事件分组结束

#define EV_KEY       0x01 // 按键事件

#define EV_REL       0x02 // 相对坐标事件（例如鼠标移动、滚轮）
#define EV_ABS       0x03 // 绝对坐标事件（例如触摸屏、触控板）
#define EV_MSC       0x04
#define EV_SW        0x05
#define EV_LED       0x11
#define EV_SND       0x12
#define EV_REP       0x14
#define EV_FF        0x15
#define EV_PWR       0x16
```

---

## 4. 常见 `code` 代码

`code` 表示具体哪个键或哪个轴。常见示例：

- `KEY_A`：A 键
- `KEY_ENTER`：回车键
- `KEY_SPACE`：空格键
- `KEY_UP`：上方向键
- `KEY_DOWN`：下方向键
- `KEY_LEFT`：左方向键
- `KEY_RIGHT`：右方向键
- `BTN_LEFT`：鼠标左键
- `BTN_RIGHT`：鼠标右键
- `REL_X`：X 轴相对移动
- `REL_Y`：Y 轴相对移动
- `REL_WHEEL`：滚轮

---

## 5. `value` 含义

`value` 的含义和 `type` 有关：

### 5.1 对按键事件 `EV_KEY`

- `0`：按键释放
- `1`：按键按下
- `2`：按键重复触发（长按重复）

例如：

```c
type = EV_KEY
code = KEY_A
value = 1
```

表示：A 键按下。

```c
type = EV_KEY
code = KEY_A
value = 0
```

表示：A 键释放。






## 6. 结论

Linux 输入子系统的事件读取核心是：

- `type`：事件类型
- `code`：哪个键/轴
- `value`：状态或变化值


