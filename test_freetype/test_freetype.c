#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <math.h> //数学库函数头文件
#include <wchar.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define FB_DEV "/dev/fb0" // LCD 设备节点

#define argb8888_to_rgb565(color) ({ \
    unsigned int temp = (color);     \
    ((temp & 0xF80000UL) >> 8) |     \
        ((temp & 0xFC00UL) >> 5) |   \
        ((temp & 0xF8UL) >> 3);      \
})

static unsigned int width;                 // LCD 宽度
static unsigned int height;                // LCD 高度
static unsigned short *screen_base = NULL; // LCD 显存基地址 RGB565
static unsigned long screen_size;          // 显存总大小，单位：字节
static int lcd_fd = -1;                    // LCD 设备文件描述符
static FT_Library library;                 // FreeType 库对象
static FT_Face face;                       // FreeType 字体对象

/*
    * @brief: framebuffer 设备初始化
    * @return: 0 成功，-1 失败
    * @note: 1. 打开 framebuffer 设备 /dev/fb0
           2. 获取 LCD 的分辨率、像素格式等信息
           3. 将显存映射到用户空间
           4. 清屏
*/
static int fb_dev_init(void)
{
    struct fb_var_screeninfo fb_var = {0};
    struct fb_fix_screeninfo fb_fix = {0};
    /* 打开 framebuffer 设备 */
    lcd_fd = open(FB_DEV, O_RDWR);
    if (0 > lcd_fd)
    {
        fprintf(stderr, "open error: %s: %s\n", FB_DEV, strerror(errno));
        return -1;
    }
    /* 获取 framebuffer 设备信息 */
    ioctl(lcd_fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(lcd_fd, FBIOGET_FSCREENINFO, &fb_fix);

    // 显存缓冲区的大小，后续用于 mmap() 的映射长度参数和 memset() 清屏的范围
    screen_size = fb_fix.line_length * fb_var.yres;
    width = fb_var.xres;  // LCD 宽度
    height = fb_var.yres; // LCD 高度
    /* 内存映射 */
    screen_base = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, lcd_fd, 0);
    if (MAP_FAILED == (void *)screen_base)
    {
        perror("mmap error");
        close(lcd_fd);
        return -1;
    }
    /* LCD 背景刷成黑色 */
    memset(screen_base, 0xFF, screen_size);
    return 0;
}
/*
    * @brief: freetype 初始化
    * @param: font 字体文件路径
    * @param: angle 旋转角度
    * @return: 0 成功，-1 失败
    * @note: 1. 初始化 FreeType 库
           2. 加载字体文件，创建 face 对象
           3. 设置字体大小和旋转矩阵
*/
static int freetype_init(const char *font, int angle)
{

    FT_Error error;   // FreeType 错误码
    FT_Vector pen;    // 字形绘制的起始原点（平移量）
    FT_Matrix matrix; // 2x2 变换矩阵（旋转/斜体/缩放）
    float rad;        // 旋转角度,角度转弧度后的值

    /* FreeType 初始化 */
    FT_Init_FreeType(&library);
    /* 加载 face 对象 */
    error = FT_New_Face(library, font, 0, &face);
    if (error)
    {
        fprintf(stderr, "FT_New_Face error: %d\n", error);
        exit(EXIT_FAILURE);
    }
    /* 原点坐标 */
    pen.x = 0 * 64;
    pen.y = 0 * 64; // 原点设置为(0, 0)
    /* 2x2 矩阵初始化 */
    rad = (1.0 * angle / 180) * M_PI; // 角度转换为弧度
#if 1
    matrix.xx = (FT_Fixed)(cos(rad) * 0x10000L);
    matrix.xy = (FT_Fixed)(-sin(rad) * 0x10000L);
    matrix.yx = (FT_Fixed)(sin(rad) * 0x10000L);
    matrix.yy = (FT_Fixed)(cos(rad) * 0x10000L);
#endif

#if 0 // 斜体 水平方向 and slimer
        matrix.xx = (FT_Fixed)( cos(rad) * 0x10000L);
        matrix.xy = (FT_Fixed)( sin(rad) * 0x10000L);
        matrix.yx = (FT_Fixed)( 0 * 0x10000L);
        matrix.yy = (FT_Fixed)( 1 * 0x10000L);

#endif

#if 0 // 斜体 水平方向显示的
        matrix.xx = (FT_Fixed)( 1 * 0x10000L);
        matrix.xy = (FT_Fixed)( tan(rad) * 0x10000L);
        matrix.yx = (FT_Fixed)( 0 * 0x10000L);
        matrix.yy = (FT_Fixed)( 1 * 0x10000L);
#endif

#if 0 // 斜体 up and down
        matrix.xx = (FT_Fixed)( 1 * 0x10000L);
        matrix.xy = (FT_Fixed)(  0* 0x10000L);
        matrix.yx = (FT_Fixed)( tan(rad) * 0x10000L);
        matrix.yy = (FT_Fixed)( 1 * 0x10000L);
#endif

    // 设置字体变换矩阵和原点坐标
    FT_Set_Transform(face, &matrix, &pen);

    // 设置字体大小.字号
    FT_Set_Pixel_Sizes(face, 60, 0); // 设置字体大小,50 pixel
    return 0;
}

/*
    * @brief: 把一串宽字符（中文）逐个渲染成字形位图，再
                写入 framebuffer 显示
    * @param: x 字符绘制的起始 x 坐标
    * @param: y 字符绘制的起始 y 坐标
    * @param: str 待绘制的字符串,宽字符串（L"..."），每个字符是 wchar_t，支持中文
    * @param: color 字符颜色，ARGB8888 格式
    * @return: void
    * @note: 1. 循环加载各个字符，获取字形位图数据
           2. 将字形位图数据绘制到显存中
*/
static void lcd_draw_character(int x, int y,
                               const wchar_t *str, unsigned int color)
{
    unsigned short rgb565_color = argb8888_to_rgb565(color); // 得到 RGB565 颜色值
    /*
    取字形槽（后续每次都覆盖）。
    slot = face->glyph 只是一个指针，指向 FreeType 内部维护的当前字形槽。每次 FT_Load_Char() 后它指向的内容会更新，所以拿到后要立刻使用
    */
    FT_GlyphSlot slot = face->glyph;

    size_t len = wcslen(str); // 计算字符的个数
    long int temp;
    int n;
    int i, j, p, q;
    int max_x, max_y, start_y, start_x;

    // 循环加载各个字符
    for (n = 0; n < len; n++)
    {
        /*
        加载字形、转换得到位图数据
        FT_Load_Char 做了三件事：编码 → 字形索引 → 渲染成位图，渲染结果在 slot->bitmap。
        */
        if (FT_Load_Char(face, str[n], FT_LOAD_RENDER))
            continue;

        /*
        难点：坐标计算。先不管，后面再说
        */
        start_y = y - slot->bitmap_top; // 计算字形轮廓上边 y 坐标起点位置 注意是减去 bitmap_top
        if (0 > start_y)
        { // 如果为负数 如何处理？？
            q = -start_y;
            temp = 0;
            j = 0;
        }
        else
        { // 正数又该如何处理??
            q = 0;
            temp = width * start_y;
            j = start_y;
        }
        max_y = start_y + slot->bitmap.rows; // 计算字形轮廓下边 y 坐标结束位置
        if (max_y > (int)height)
            max_y = height;
        for (; j < max_y; j++, q++, temp += width)
        {
            start_x = x + slot->bitmap_left; // 起点位置要加上左边空余部分长度
            if (0 > start_x)
            {
                p = -start_x;
                i = 0;
            }
            else
            {
                p = 0;
                i = start_x;
            }
            max_x = start_x + slot->bitmap.width;
            if (max_x > (int)width)
                max_x = width;
            for (; i < max_x; i++, p++)
            {
                // 如果数据不为 0，则表示需要填充颜色
                if (slot->bitmap.buffer[q * slot->bitmap.width + p])
                    screen_base[temp + i] = rgb565_color;
            }
        }
        // 调整到下一个字形的原点
        x += slot->advance.x / 64; // 26.6 固定浮点格式
        y -= slot->advance.y / 64;
    }
}

/*
    * @brief: 主函数
    * @param: 第一个参数：字体文件路径
            第二个参数：旋转角度
    * @return: 0 成功，-1 失败
    * @note: 1. 初始化 framebuffer 设备
           2. 初始化 freetype 库
           3. 在 LCD 上显示中文
           4. 退出程序，释放资源
*/
int main(int argc, char *argv[])
{
    /* LCD 初始化 */
    if (fb_dev_init())
        exit(EXIT_FAILURE);
    /* freetype 初始化 */
    if (freetype_init(argv[1], atoi(argv[2])))
        exit(EXIT_FAILURE);

    /* 在 LCD 上显示中文 */
    int y = height * 0.25;
    lcd_draw_character(50, 100, L"远涉重洋学成归，一灯一课细心裁。", 0x000000);
    lcd_draw_character(50, y + 100, L"机器学习三十讲，深度新篇四十开。", 0x9900FF);
    lcd_draw_character(50, 2 * y + 100, L"嵌入乾坤从浅入，智能万象次第来。", 0xFF0099);
    lcd_draw_character(50, 3 * y + 100, L"镜前数载勤开讲，愿使人人学得来。", 0x9932CC);
    /* 退出程序 */
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    munmap(screen_base, screen_size);
    close(lcd_fd);
    exit(EXIT_SUCCESS);
}
