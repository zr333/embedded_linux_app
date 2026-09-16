# iM6ULL — i.MX6ULL 嵌入式 Linux 外设驱动学习

基于 **NXP i.MX6ULL** 平台的嵌入式 Linux 外设编程示例集合。

## 环境与工具链

- **目标平台**：NXP i.MX6ULL（Cortex-A7）
- **交叉编译器**：`arm-linux-gnueabihf-gcc`
- **系统**：Linux ATK-IMX6U 4.1.15-ge48931b1（官方自带）
- **第三方库**：`tslib`（触摸屏）、`freetype`（字体渲染）等

## 目录结构

| 目录 | 内容 | 关键技术 |
|------|------|----------|
| [`test_LED`](test_LED/README.md) | LED 亮灭控制 | sysfs `/sys/class/leds/`、trigger 机制 |
| [`test_beep`](test_beep/README.md) | 蜂鸣器控制 | sysfs `/sys/class/leds/beep/`、trigger 机制 |
| [`test_lcd`](test_lcd/README.md) | LCD 显示 | framebuffer `/dev/fb0`、mmap、RGB565 |
| [`test_input`](test_input/README.md) | 按键输入 | input 子系统 `/dev/input/event*` |
| [`test_ts`](test_ts/README.md) | 触摸屏 | tslib 库、单点/多点触摸 |
| [`test_freetype`](test_freetype/README.md) | 矢量字体渲染 | FreeType 库、中文显示 |

## 快速开始

各模块的详细编译命令见对应子目录的 `README.md`。以 LED 为例：

```bash
# 交叉编译
arm-linux-gnueabihf-gcc led.c -o led

# 拷贝到开发板运行
scp led root@<板子IP>:/root/
```


## 模块总览

1. **LED / 蜂鸣器**：通过读写 sysfs 中的 `brightness` 和 `trigger` 文件控制外设，是最简单的 Linux 字符设备编程入门。
2. **LCD**：打开 `/dev/fb0`，用 `ioctl` 获取屏幕参数，`mmap` 映射显存后直接写像素，实现清屏、画点画线、显示 BMP 图片。
3. **输入**：读取 `/dev/input/event*`，解析 `struct input_event`，识别按键按下/松开，并演示按键控制 LED。
4. **触摸屏**：借助 tslib 屏蔽底层细节，读取单点（`ts_read`）和多点（`ts_read_mt`）触摸坐标与压力。
5. **FreeType**：初始化字体引擎、加载字体文件、设置旋转矩阵与字号，把字符渲染成灰度位图后写入 framebuffer，支持中文显示。


本项目仅用于个人学习，示例代码可自由使用。
