# test_lcd — LCD 显示（Framebuffer）

通过 Linux framebuffer 设备 `/dev/fb0` 直接操作显存，实现清屏、画点画线、显示 BMP 图片。

## 文件说明

| 文件 | 功能 |
|------|------|
| `fb_info.c` | 获取并打印 LCD 分辨率、像素深度、行字节数、RGB 位定义等信息 |
| `lcd_test.c` | 画点、画水平/垂直线、画矩形，演示 ARGB8888 → RGB565 转换 |
| `bmp_show.c` | 解析 BMP 文件并在 LCD 上显示图片（支持正/倒向位图） |
| `fbr笔记.md` | Framebuffer 编程核心知识笔记 |
| `five_star.bmp` | 测试用 BMP 图片 |

## 编译

```bash
arm-linux-gnueabihf-gcc fb_info.c -o fb_info
arm-linux-gnueabihf-gcc lcd_test.c -o lcd_test
arm-linux-gnueabihf-gcc bmp_show.c -o bmp_show
```

## 运行

```bash
./fb_info                 # 查看屏幕参数
./lcd_test                # 画点画线测试
./bmp_show five_star.bmp  # 显示 BMP 图片
```

## 原理要点

1. `open("/dev/fb0", O_RDWR)` 打开设备。
2. `ioctl(FBIOGET_VSCREENINFO / FBIOGET_FSCREENINFO)` 获取分辨率、`bits_per_pixel`、`line_length` 等参数。
3. `mmap()` 把显存映射到用户空间，直接写像素。
4. 写入格式必须与设备一致（本平台为 RGB565）。

详见同目录下的 `fbr笔记.md`。
