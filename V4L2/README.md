# test V4L2 — 摄像头采集显示

通过 V4L2 接口采集 USB / MIPI 摄像头数据，并以 RGB565 格式实时显示到 LCD 上。

## 文件说明

| 文件 | 说明 |
|------|------|
| `V4L2.c` | 主程序：初始化 LCD 与摄像头、枚举格式、采集并显示 |
| `V4L2笔记.md` | V4L2 学习笔记：核心概念、使用流程、关键结构体 |

## 编译

```bash
arm-linux-gnueabihf-gcc V4L2.c -o V4L2
```

仅依赖标准头文件与内核头文件（`linux/videodev2.h`、`linux/fb.h`），无需第三方库。

## 运行

需要传入摄像头设备节点作为参数：

```bash
scp V4L2 root@<板子IP>:/root/
./V4L2 /dev/video1
```

> 先执行 `ls /dev/video*` 确认摄像头节点。
> `/dev/video0` 通常是板载的其它设备，USB 摄像头一般为 `video1`。

## 原理要点

**采集流程（`main` 中的顺序）**

```
fb_dev_init       初始化 LCD：open /dev/fb0 → ioctl 取参数 → mmap 映射显存
v4l2_dev_init     初始化摄像头：open /dev/videoX → VIDIOC_QUERYCAP 查询能力
v4l2_enum_formats 枚举支持的像素格式           (VIDIOC_ENUM_FMT)
v4l2_print_formats 打印各格式的分辨率与帧率     (VIDIOC_ENUM_FRAMESIZES / FRAMEINTERVALS)
v4l2_set_format   设置采集格式为 RGB565         (VIDIOC_S_FMT / G_PARM / S_PARM)
v4l2_init_buffer  申请 3 个缓冲 → mmap 映射 → 全部入队 (REQBUFS / QUERYBUF / QBUF)
v4l2_stream_on    开启采集                      (VIDIOC_STREAMON)
v4l2_read_data    循环「出队 → 拷贝到 LCD → 重新入队」 (DQBUF / QBUF)
```

**缓冲区的循环使用**

```
QBUF 入队 ──▶ 驱动填数据 ──▶ DQBUF 出队 ──▶ 处理 ──▶ QBUF 重新入队 ──▶ ...
```

缓冲区数量为 `FRAMEBUFFER_COUNT = 3`，只有重新入队才能被驱动再次使用，否则很快就会"无缓冲可用"。

**零拷贝**

驱动采集的数据位于内核空间，用户态通过 `mmap` 直接映射访问，**不需要 read() 拷贝**。
`v4l2_read_data` 中只需把每行数据 `memcpy` 到 LCD 显存，且行步长按 `frm_width`（视频帧宽）与 `width`（LCD 宽）分别推进。

**关键点**

- 摄像头与 LCD 分辨率可能不同，代码取两者较小值 `min_w` / `min_h` 作为拷贝范围，避免越界。
- 像素格式必须是 **RGB565**（2 字节/像素），才能直接拷进 LCD 显存；其它格式需先转换。
