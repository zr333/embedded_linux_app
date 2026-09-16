#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
/*
    
 * 该程序用于在 Linux 下的 framebuffer 设备上显示 BMP 图片。
 * 主要功能：
 *   1. 打开 framebuffer 设备 /dev/fb0
 *   2. 获取 LCD 的分辨率、像素格式等信息
 *   3. 将 BMP 文件的像素数据读取并写入显存，从而在 LCD 上显示图片
 *
 * 注意事项：
 *   - 本程序假定 BMP 图片为 RGB565 格式，且与 LCD 的像素格式一致。
 *   - BMP 文件的高度可以为正或负，分别表示倒向位图和正向位图。
 *   - 使用 mmap 映射显存后，直接写入映射地址即可显示内容。
*/


/********************************************************************
 * BMP 文件头结构体
 * 作用：描述 BMP 文件本身的基本信息，主要用于定位图像像素数据
 * 说明：使用 __attribute__((packed)) 是为了保证结构体字段按原始
 *       BMP 文件中的字节布局存储，不插入额外的填充字节。
 ********************************************************************/
typedef struct
{
    unsigned char type[2];    /* "BM"，标识这是 BMP 文件 */
    unsigned int size;        /* BMP 文件总大小，单位：字节 */
    unsigned short reserved1; /* 保留字段 1，通常为 0 */
    unsigned short reserved2; /* 保留字段 2，通常为 0 */
    unsigned int offset;      /* 从文件头开始，到真正像素数据位置的偏移量 */
} __attribute__((packed)) bmp_file_header;
/*
__attribute__((packed)) 是 GCC/Clang 的扩展，作用是：告诉编译器取消结构体成员之间的自动填充字节，让结构体按最小字节紧凑排列。在解析 BMP、网络协议包等二进制格式时非常常用
*/

/********************************************************************
 * BMP 信息头结构体
 * 作用：描述图像的尺寸、颜色深度、压缩方式等参数
 * 说明：这里按 Windows BMP 的 BITMAPINFOHEADER 结构体布局来解析。
 ********************************************************************/
typedef struct
{
    unsigned int size;        /* 该信息头结构体的大小，通常为 40 */
    int width;                /* 图像宽度，单位：像素 */
    int height;               /* 图像高度，单位：像素，正数/负数表示上下方向 */
    unsigned short planes;    /* 目标设备的位平面数，通常为 1 */
    unsigned short bpp;       /* 每个像素占用的位数，如 24、16、32 */
    unsigned int compression; /* 图像压缩方式，0 表示不压缩 */
    unsigned int image_size;  /* 图像数据区大小，压缩时可能不为 0 */
    int x_pels_per_meter;     /* 水平分辨率，像素/米 */
    int y_pels_per_meter;     /* 垂直分辨率，像素/米 */
    unsigned int clr_used;    /* 使用的颜色索引数，0 表示默认 */
    unsigned int clr_omportant; /* 重要颜色索引数，通常可忽略 */
} __attribute__((packed)) bmp_info_header;

/********************************************************************
 * 全局变量说明
 * width / height：当前 LCD 屏幕分辨率
 * screen_base：mmap 映射后的显存起始地址，通过它可直接写像素
 * line_length：LCD 一行的字节长度，和 fb_fix_screeninfo.line_length 一致
 ********************************************************************/
static int width;                          /* LCD X 分辨率 */
static int height;                         /* LCD Y 分辨率 */
static unsigned short *screen_base = NULL; /* mmap 映射后的显存基地址 */
static unsigned long line_length;          /* LCD 一行的长度（字节） */


/********************************************************************
 * 函数名称： show_bmp_image
 * 功能描述： 读取 BMP 文件并将其像素数据直接写入 LCD 显存中，即在LCD上显示指定的BMP图片
 * 输入参数： path - BMP 文件路径
 * 返回值：  成功返回 0，失败返回 -1
 * 说明：
 *   1. 这里假定 BMP 图像采用 RGB565 格式，且 LCD 也使用同样的像素格式。
 *   2. 对 BMP 图片的上下方向做了兼容处理：
 *      - height > 0：倒向位图（从底部开始显示）
 *      - height < 0：正向位图（从顶部开始显示）
 *   3. 使用 mmap 后的显存指针直接写入，效率高，适合小型 Linux 嵌入式显示应用。
 ********************************************************************/
static int show_bmp_image(const char *path)
{
    bmp_file_header file_h;              /* BMP 文件头 */
    bmp_info_header info_h;              /* BMP 信息头 */
    unsigned short *line_buf = NULL;     /* 一行像素的缓冲区，用于暂存从 BMP 文件中读取的一行 */
    unsigned long line_bytes;            /* BMP 一行数据总字节数 */
    unsigned int min_h, min_bytes;       /* 实际要显示的高度和最小拷贝字节数 */
    int fd = -1;                         /* BMP 文件描述符 */
    int j;                               /* 循环变量 */

    /* 1. 打开 BMP 文件，只读方式打开 */
    if (0 > (fd = open(path, O_RDONLY)))
    {
        perror("open error");
        return -1;
    }

    /* 2. 读取 BMP 文件头，验证文件类型是否为 "BM" */
    if (sizeof(bmp_file_header) !=
        read(fd, &file_h, sizeof(bmp_file_header)))
    {
        perror("read error");
        close(fd);
        return -1;
    }

    if (0 != memcmp(file_h.type, "BM", 2))
    {
        fprintf(stderr, "it's not a BMP file\n");
        close(fd);
        return -1;
    }

    /* 3. 读取 BMP 信息头，获取宽高、bpp 等属性 */
    if (sizeof(bmp_info_header) !=
        read(fd, &info_h, sizeof(bmp_info_header)))
    {
        perror("read error");
        close(fd);
        return -1;
    }

    /* 4. 打印 BMP 基本信息，便于调试 */
    printf("文件大小: %d\n"
           "位图数据的偏移量: %d\n"
           "位图信息头大小: %d\n"
           "图像分辨率: %d*%d\n"
           "像素深度: %d\n",
           file_h.size, file_h.offset,
           info_h.size, info_h.width, info_h.height,
           info_h.bpp);

    /* 5. 将文件偏移移动到像素数据区起始位置，这里是移动文件指针到像素数据区 */
    if (-1 == lseek(fd, file_h.offset, SEEK_SET))
    {
        perror("lseek error");
        close(fd);
        return -1;
    }

    /* 6. 计算一行数据占用的字节数：宽度 × 每像素字节数 */
    line_bytes = info_h.width * info_h.bpp / 8;
    line_buf = malloc(line_bytes);
    if (NULL == line_buf)
    {
        fprintf(stderr, "malloc error\n");
        close(fd);
        return -1;
    }

    /* 7. 计算 LCD 实际可显示的最小宽度字节数 */
    if (line_length > line_bytes)
        min_bytes = line_bytes;
    else
        min_bytes = line_length;

    /******************************************************************
     * 8. 将图像数据按屏幕方向显示到 LCD
     *    说明：
     *      - BMP 的高度大于 0：倒向位图，像素数据从底部开始存储
     *      - BMP 的高度小于 0：正向位图，像素数据从顶部开始存储
     *      - 这里默认图片为 RGB565，屏幕格式与之匹配
     *      - 因为使用了直接写显存，所以不需要额外进行图像转换
     ******************************************************************/
    if (0 < info_h.height)
    { /* 倒向位图：图像数据从底部开始，显示时要从屏幕底部向上刷 */
        if (info_h.height > height)
        {
            min_h = height;
            /* 如果图像比屏幕高，跳过超出屏幕部分的上方像素
            如果图片比屏幕高，先跳过图片顶部多出来的那些行，
            然后从屏幕底部开始写入，只显示最下面的 height 行
             */
            lseek(fd, (info_h.height - height) * line_bytes, SEEK_CUR);
            /* 从屏幕左下角开始写入，屏幕坐标系中 y 增大向下，因而从底部开始 */
            screen_base += width * (height - 1);
        }
        else
        {
            min_h = info_h.height;
            /* 图像高度不超过屏幕高度时，从屏幕底部开始排列 */
            screen_base += width * (info_h.height - 1);
        }

        /* 从最后一行开始向上写入，每次写一行 */
        for (j = min_h; j > 0; screen_base -= width, j--)
        {
            read(fd, line_buf, line_bytes);           /* 读取一行 BMP 像素数据 */
            memcpy(screen_base, line_buf, min_bytes); /* 复制到 LCD 显存对应位置 */
        }
    }
    else
    { /* 正向位图：图像数据从顶部开始，显示时从屏幕顶部向下刷 */
        int temp = 0 - info_h.height; /* 负值转正，表示图像真实高度 */
        if (temp > height)
            min_h = height;
        else
            min_h = temp;

        /* 从屏幕顶部开始一个像素行接一个像素行写入 */
        for (j = 0; j < min_h; j++, screen_base += width)
        {
            read(fd, line_buf, line_bytes);
            memcpy(screen_base, line_buf, min_bytes);
        }
    }

    /* 9. 释放资源并返回 */
    close(fd);
    free(line_buf);
    return 0;
}

/********************************************************************
 * 主函数：初始化 LCD framebuffer，并显示 BMP 图像
 * 逻辑：
 *   1. 校验程序参数是否正确
 *   2. 打开 /dev/fb0 设备
 *   3. 读取 framebuffer 的固定/可变信息
 *   4. 使用 mmap 将显存映射到用户空间
 *   5. 清屏后调用 show_bmp_image() 显示图片
 *   6. 退出前释放映射和关闭设备
 ********************************************************************/
int main(int argc, char *argv[])
{
    struct fb_fix_screeninfo fb_fix; /* LCD 固定参数：行长度、显存起始地址等 */
    struct fb_var_screeninfo fb_var; /* LCD 可变参数：分辨率、bpp、颜色格式等 */
    unsigned int screen_size;        /* 显存总大小，单位：字节 */
    int fd;                         /* framebuffer 设备文件描述符 */

    /* 1. 程序调用方式检查：必须带一个 BMP 文件路径参数 */
    if (2 != argc)
    {
        fprintf(stderr, "usage: %s <bmp_file>\n", argv[0]);
        exit(-1);
    }

    /* 2. 打开 framebuffer 设备 /dev/fb0 */
    if (0 > (fd = open("/dev/fb0", O_RDWR)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    /* 3. 获取 framebuffer 的参数信息 */
    /*
     * fb_var 包含：分辨率 xres / yres、bits_per_pixel、颜色 bitfield 等
     * fb_fix 包含：line_length、smem_len、类型、设备名等固定属性
     */
    ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);

    /* 4. 保存 LCD 基本参数，后续显示 BMP 时会用到 */
    screen_size = fb_fix.line_length * fb_var.yres; // 这个值之后用于 mmap() 的映射长度参数和 memset() 清屏的范围
    line_length = fb_fix.line_length;
    width = fb_var.xres;
    height = fb_var.yres;

    /* 5. 将 LCD 显存映射到当前进程地址空间 */
    /*
     * mmap() 后，screen_base 指向显存首地址。
     * 之后直接向该地址写入像素数据，就相当于在 LCD 上显示内容。
     */
    screen_base = mmap(NULL, screen_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == (void *)screen_base)
    {
        perror("mmap error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* 6. 先清屏为白色（0xFF 为 16 位 RGB565 下的白色） */
    memset(screen_base, 0xFF, screen_size);

    /* 7. 显示指定 BMP 图片 */
    show_bmp_image(argv[1]);

    /* 8. 退出前解除映射并关闭设备 */
    munmap(screen_base, screen_size); /* 取消显存映射 */
    close(fd);                        /* 关闭 framebuffer 设备 */
    exit(EXIT_SUCCESS);               /* 正常退出 */
}
