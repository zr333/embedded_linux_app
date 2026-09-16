# Linux Framebuffer（帧缓冲）核心知识笔记


---

# 1. 什么是 Framebuffer

## 1.1 基本概念

Framebuffer（帧缓冲）可以理解为：

> “帧缓冲”（framebuffer）就是一块内存，里面存放着屏幕上一帧图像的数据。屏幕上的每个像素对应内存中的若干位，比如 RGB565、ARGB8888 等。显示控制器会不断扫描这块内存，并把它输出到屏幕。Linux 把这块显存抽象成 framebuffer 设备。应用程序不需要了解具体显卡寄存器怎么操作，只要打开 /dev/fb0，通过 ioctl 获取屏幕参数，再用 mmap 把显存映射到用户空间，就可以直接写像素。屏幕上的每一个像素，都对应 Framebuffer 中的一段数据。





## 1.2 Framebuffer 的工作关系

```text
应用程序
    │
    │ 写入像素数据
    ▼
Framebuffer（帧缓冲/显存）
    │
    │ 显示控制器读取
    ▼
LCD / 显示器
```

因此，从应用程序角度看：

> **绘制图形的本质，就是向 Framebuffer 的正确位置写入正确的像素数据。**

---

# 2. Linux 中的 Framebuffer 设备

Linux 中，Framebuffer 通常表现为字符设备。

常见设备：

```text
/dev/fb0
```


应用程序通常先打开：

```c
int fd = open("/dev/fb0", O_RDWR);
```

得到文件描述符：

```text
fd
```
然后可以通过 `ioctl()` 查询设备信息，并通过 `mmap()` 映射显存。

---

# 3. Framebuffer 中的两个重要信息结构体

Linux Framebuffer 编程中经常使用：

```c
struct fb_var_screeninfo;
struct fb_fix_screeninfo;
```

这两个结构体都定义在 Linux 内核头文件 `<linux/fb.h>` 中，是 Framebuffer 编程最核心的两个数据结构：

- `struct fb_var_screeninfo` —— **可变屏幕信息**（variable），描述“可以修改”的显示参数
- `struct fb_fix_screeninfo` —— **固定屏幕信息**（fixed），描述“由硬件决定、无法修改”的显示参数

它们都通过 `ioctl()` 获取：

```c
#include <linux/fb.h>

struct fb_var_screeninfo var;
struct fb_fix_screeninfo fix;

// 获取可变信息（可读可写）
ioctl(fd, FBIOGET_VSCREENINFO, &var);   // 读
ioctl(fd, FBIOPUT_VSCREENINFO, &var);   // 写（可修改分辨率等）

// 获取固定信息（只读）
ioctl(fd, FBIOGET_FSCREENINFO, &fix);   // 只能读
```

## 3.1 struct fb_var_screeninfo（可变屏幕信息）

“var” 即 variable，描述可以动态修改的显示参数。完整定义：

```c
struct fb_var_screeninfo {
    __u32 xres;            // 可见区域宽度（实际显示分辨率，像素）
    __u32 yres;            // 可见区域高度（像素）
    __u32 xres_virtual;    // 虚拟分辨率宽度（显存实际缓冲大小）
    __u32 yres_virtual;    // 虚拟分辨率高度
    __u32 xoffset;         // 从虚拟缓冲到可见区域的水平偏移
    __u32 yoffset;         // 从虚拟缓冲到可见区域的垂直偏移

    __u32 bits_per_pixel;  // 每像素位数（bpp），如 16 / 24 / 32
    __u32 grayscale;       // 0=彩色, 1=灰度, >1=FOURCC 格式

    struct fb_bitfield red;      // 红色通道的位定义
    struct fb_bitfield green;    // 绿色通道的位定义
    struct fb_bitfield blue;     // 蓝色通道的位定义
    struct fb_bitfield transp;   // 透明通道的位定义

    __u32 nonstd;          // 非标准像素格式标志
    __u32 activate;        // 激活方式，见 FB_ACTIVATE_*

    __u32 height;          // 屏幕物理高度（mm）
    __u32 width;           // 屏幕物理宽度（mm）

    __u32 pixclock;        // 像素时钟周期（皮秒 ps）
    __u32 left_margin;     // 行同步到有效数据的时间
    __u32 right_margin;    // 有效数据到行同步的时间
    __u32 upper_margin;    // 帧同步到有效数据的时间
    __u32 lower_margin;    // 有效数据到帧同步的时间
    __u32 hsync_len;       // 水平同步脉冲长度
    __u32 vsync_len;       // 垂直同步脉冲长度

    __u32 sync;            // 同步方式，见 FB_SYNC_*
    __u32 vmode;           // 视频模式，见 FB_VMODE_*
    __u32 rotate;          // 逆时针旋转角度
    __u32 colorspace;      // FOURCC 模式的色彩空间
    __u32 reserved[4];     // 保留字段
};
```

**最常用的字段：**

- `xres` / `yres`：**实际显示**的分辨率（像素），程序按它来绘图。
- `bits_per_pixel`：每像素占多少位（bpp），决定一个像素在内存里占多大。
- `red` / `green` / `blue`：`fb_bitfield` 类型，精确定义 RGB 三通道在像素中的**位置和位数**，是“正确写颜色”的关键。

> 其余字段（`xres_virtual`、时序参数 `pixclock` 等）一般在驱动/配置阶段才用到，应用层绘图通常只关心上面三个。

## 3.2 struct fb_fix_screeninfo（固定屏幕信息）

“fix” 即 fixed，描述由硬件/驱动决定、**只读不可修改**的参数。完整定义：

```c
struct fb_fix_screeninfo {
    char id[16];               // 设备标识字符串，如 "CLCD FB"
    unsigned long smem_start;  // 显存起始物理地址
    __u32 smem_len;            // 显存总大小（字节）

    __u32 type;                // 帧缓冲类型，见 FB_TYPE_*
    __u32 type_aux;            // 平面交织方式
    __u32 visual;              // 颜色模式，见 FB_VISUAL_*

    __u16 xpanstep;            // 水平平移步长（0 = 不支持硬件平移）
    __u16 ypanstep;            // 垂直平移步长（0 = 不支持）
    __u16 ywrapstep;           // 垂直回绕步长

    __u32 line_length;         // 每一行像素占用的字节数（重要！）

    unsigned long mmio_start;  // MMIO 起始物理地址
    __u32 mmio_len;            // MMIO 长度

    __u32 accel;               // 加速芯片信息
    __u16 capabilities;        // 设备能力，见 FB_CAP_*
    __u16 reserved[2];         // 保留字段
};
```

**最常用的字段：**

- `smem_start` / `smem_len`：显存的**物理地址和大小**，`mmap()` 映射时参考。
- `line_length`：**一行像素占用的字节数**。注意它可能大于 `xres * (bpp/8)`（因为有对齐填充），所以逐像素写显存时**必须用 `line_length` 作为行跨度**。
- `visual`：颜色模式，常见取值有 `FB_VISUAL_TRUECOLOR`（真彩色，最常用）、`FB_VISUAL_PSEUDOCOLOR`（伪彩色/调色板）等。

## 3.3 struct fb_bitfield（颜色通道位定义）

`fb_var_screeninfo` 中的 `red/green/blue/transp` 都是这个类型，用来精确定义某个颜色通道在像素中的位置：

```c
struct fb_bitfield {
    __u32 offset;     // 该通道的起始位（从最低位 bit0 开始数）
    __u32 length;     // 该通道占用的位数
    __u32 msb_right;  // 最高位是否在右边（!=0 表示是）
};
```

例如 RGB565（R:G:B = 5:6:5）常见定义：

```text
blue.offset  = 0,  blue.length  = 5   // B 占 bit0~bit4
green.offset = 5,  green.length = 6   // G 占 bit5~bit10
red.offset   = 11, red.length   = 5   // R 占 bit11~bit15
```


**编程时的配合：**

- 用 `var` 得到 `bits_per_pixel` 和 RGB 位字段 → 决定怎么组装一个像素的颜色值；
- 用 `fix` 得到 `line_length` → 决定每行像素在显存里的偏移；
- 二者结合，才能把像素写到正确的位置。



## 4 RGB 系列的对比总结

```text
格式      每像素位数     颜色通道            说明
---------------------------------------------------
RGB565       16            R:G:B = 5:6:5      常见于嵌入式 LCD
RGB888       24            R:G:B = 8:8:8      颜色丰富，常见于高质量显示
RGB8888      32            R:G:B = 8:8:8      32 位真彩，不带 Alpha
ARGB8888     32            A:R:G:B = 8:8:8:8  支持透明度
```

在 Linux Framebuffer 中，真正影响显示效果的，不是“看起来像某种颜色格式”，而是：

```text
bits_per_pixel + fb_bitfield 中各通道的位定义
```

也就是说，同样是 RGB565，实际也可能因为位位置不同，而表现成不同的内存布局。程序在写显存时，必须与设备真实支持的像素格式一致，否则颜色会异常、图像会偏色或出现错乱。

因此，Framebuffer 编程里最常见的核心思路就是：

```text
1. 读取 fb_var_screeninfo
2. 判断 bits_per_pixel 和 RGB bitfield
3. 按格式正确写入像素数据
4. 显示到屏幕上
```


