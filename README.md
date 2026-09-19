# iM6ULL — i.MX6ULL 嵌入式 Linux 外设驱动学习

基于 **NXP i.MX6ULL** 平台的嵌入式 Linux 外设编程示例集合。
每个模块一个独立目录，包含 C 源码、使用说明与学习笔记。

## 环境与工具链

| 项目 | 说明 |
|------|------|
| 目标平台 | NXP i.MX6ULL（Cortex-A7） |
| 开发板系统 | Linux ATK-IMX6U 4.1.15-ge48931b1 |
| 交叉编译器 | `arm-linux-gnueabihf-gcc` |
| 第三方库 | tslib（触摸屏）、FreeType（字体渲染） |

## 目录结构

| 目录 | 内容 | 关键技术 | 详细说明 |
|------|------|----------|----------|
| [`test_LED`](test_LED/README.md) | LED 亮灭控制 | sysfs `/sys/class/leds/`、trigger 机制 | [README](test_LED/README.md) |
| [`test_beep`](test_beep/README.md) | 蜂鸣器控制 | sysfs `/sys/class/leds/beep/`、trigger 机制 | [README](test_beep/README.md) |
| [`test_lcd`](test_lcd/README.md) | LCD 显示 | framebuffer `/dev/fb0`、`mmap`、RGB565 | [README](test_lcd/README.md) ・ [笔记](test_lcd/fbr笔记.md) |
| [`test_input`](test_input/README.md) | 按键输入 | input 子系统 `/dev/input/event*` | [README](test_input/README.md) ・ [笔记](test_input/input_event_notes.md) |
| [`test_ts`](test_ts/README.md) | 触摸屏 | tslib、单点 / 多点触摸 | [README](test_ts/README.md) ・ [笔记](test_ts/编译指令.md) |
| [`test_freetype`](test_freetype/README.md) | 矢量字体渲染 | FreeType、中文显示 | [README](test_freetype/README.md) ・ [笔记](test_freetype/freetype笔记.md) |
| [`V4L2`](V4L2/README.md) | 摄像头采集 | V4L2、`ioctl`、`mmap` 零拷贝 | [README](V4L2/README.md) ・ [笔记](V4L2/V4L2笔记.md) |

## 快速开始

每个模块目录下都有 `README.md`，写明该模块的编译命令与运行方式。
通用流程如下：

```bash
# 1. 进入模块目录，交叉编译
cd test_LED
arm-linux-gnueabihf-gcc led.c -o led

# 2. 拷贝到开发板
scp led root@<板子IP>:/root/

# 3. 在开发板上运行（部分模块需 root 权限）
./led
```

> 依赖第三方库的模块（`test_ts`、`test_freetype`）需额外指定头文件与库路径，
> 具体命令见对应目录的 `README.md`。

## 模块总览

1. **LED / 蜂鸣器** —— 通过读写 sysfs 中的 `brightness` 与 `trigger` 文件控制外设，Linux 字符设备编程的最简入门。
2. **LCD** —— 打开 `/dev/fb0`，用 `ioctl` 获取屏幕参数，`mmap` 映射显存后直接写像素，实现清屏、画点画线、显示 BMP 图片。
3. **输入** —— 读取 `/dev/input/event*`，解析 `struct input_event`，识别按键按下 / 松开，并演示按键控制 LED。
4. **触摸屏** —— 借助 tslib 屏蔽底层细节，读取单点（`ts_read`）与多点（`ts_read_mt`）触摸坐标及压力。
5. **FreeType** —— 初始化字体引擎、加载字体文件、设置旋转矩阵与字号，将字符渲染为灰度位图后写入 framebuffer，支持中文。
6. **V4L2** —— 通过 `/dev/videoX` 与 `ioctl` 完成格式协商、缓冲区申请与 `mmap` 映射，采集摄像头数据并显示到 LCD。

## 参考资料

- [V4L2 官方文档](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2.html)
- [FreeType 官方文档](https://freetype.org/freetype2/docs/documentation.html)
- [Linux Framebuffer 文档](https://www.kernel.org/doc/html/latest/fb/index.html)

---

本项目仅用于个人学习，示例代码可自由使用。
