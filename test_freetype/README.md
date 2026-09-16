# test_freetype — FreeType 字体渲染

使用 FreeType 库把字符（含中文）渲染成灰度位图，再写入 framebuffer 显示，支持旋转、字号设置。

## 文件说明

| 文件 | 功能 |
|------|------|
| `test_freetype.c` | 完整示例：初始化 LCD + FreeType，在屏幕上显示多行中文诗句 |
| `freetype笔记.md` | FreeType 核心知识笔记（定义、使用方法、关键对象） |
| `iMX6ULL_Lesson20 - 副本.pptx` | 课程讲义（Lesson20） |

## 编译

需要先交叉编译安装 freetype（依赖 zlib、libpng），详见 `freetype笔记.md` 第四部分。以本机路径为例：

```bash
arm-linux-gnueabihf-gcc \
    -I /home/user/tools/freetype/include/freetype2 \
    -L /home/user/tools/freetype/lib -lfreetype \
    -L /home/user/tools/zlib/lib -lz \
    -L /home/user/tools/png/lib -lpng \
    -lm -o test_freetype test_freetype.c
```

## 运行

```bash
# 参数1：字体文件路径（需支持中文），参数2：旋转角度
./test_freetype /usr/lib/fonts/simsun.ttc 0
```

## 原理要点

1. `FT_Init_FreeType()` 初始化引擎 → `FT_New_Face()` 加载字体。
2. `FT_Set_Transform()` 设置旋转矩阵，`FT_Set_Pixel_Sizes()` 设置字号。
3. `FT_Load_Char()` 渲染单个字符，得到灰度位图 `slot->bitmap`。
4. 将位图中灰度 > 0 的像素写入 framebuffer 对应位置（注意 Y 轴翻转与边界裁剪）。

详见同目录下的 `freetype笔记.md`。
