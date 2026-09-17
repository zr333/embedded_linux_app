# FreeType 笔记

> 面向 i.MX6ULL 嵌入式 Linux，framebuffer 用 RGB565（见 `test_lcd/bmp_show.c`）。
> 本篇按 **定义 → 基本使用方法 → 关键对象** 的结构整理。

---

## 一、定义

### 1.1 FreeType 是什么

FreeType 是一个开源的**字体渲染引擎库**：

- **输入**：字体文件（`.ttf` / `.otf` / `.ttc`）+ 字符编码（如 `'A'`、`L'汉'`）+ 字号
- **输出**：该字符的**灰度位图**（bitmap），本质就是一小块灰度像素数据
- **不管**：像素怎么显示到屏幕上 —— 写 Framebuffer 由我们自己完成

整个工作过程可以理解为：

```
字体文件 + 字符编码 + 字号  ──FreeType──▶  灰度位图  ──我们自己──▶  写显存 / LCD
```



## 二、基本使用方法

### 2.1 使用流程

```
FT_Init_FreeType()  →  FT_New_Face()  →  FT_Set_Pixel_Sizes()
      →  FT_Load_Char()  →  读 slot->bitmap 写显存
      →  FT_Done_Face() / FT_Done_FreeType()
```

可归纳为四步：**初始化库 → 加载字体 → 设置字号 → 加载字符并读位图**，用完释放。

### 2.2 关键 API

```c
FT_Init_FreeType(&library);                        // 初始化，返回 0 成功

FT_New_Face(library, "/path/font.ttc", 0, &face);  // 加载字体，参数3=face_index

FT_Set_Pixel_Sizes(face, 0, 32);                   // 像素大小（宽 0 = 按高度自动）
FT_Set_Char_Size(face, 16*64, 0, 96, 0);           // 点大小（16pt=16*64）

FT_Load_Char(face, 'A', FT_LOAD_RENDER);           // 加载+渲染字符
FT_Load_Char(face, L'汉', FT_LOAD_RENDER);         // Unicode 码点（中文）

FT_Done_Face(face); FT_Done_FreeType(library);     // 释放
```

### 2.3 渲染模式

| 模式 | 说明 |
|------|------|
| `FT_RENDER_MODE_NORMAL` | 8bit 灰度，抗锯齿（默认） |
| `FT_RENDER_MODE_MONO` | 1bit 黑白，无抗锯齿 |
| `FT_RENDER_MODE_LCD` | RGB 次像素，真彩 LCD 更清晰 |


---

## 三、关键对象

### 3.1 对象总览

| 对象 | 类型 | 作用 |
|------|------|------|
| `library` | `FT_Library` | 库句柄（进程初始化一次） |
| `face` | `FT_Face` | 字体对象，代表一个字体文件 |
| `slot` | `FT_GlyphSlot` | 字形槽，通过 `face->glyph` 访问 |
| `bitmap` | `FT_Bitmap` | 字形位图（灰度像素） |
| `pen` | `FT_Vector` | 平移量（26.6 定点） |
| `matrix` | `FT_Matrix` | 2×2 变换矩阵（16.16 定点） |
| `error` | `FT_Error` | 错误码，0 表示成功 |

关系链：`library → face → face->glyph → glyph->bitmap`

### 3.2 FT_Library

整个 FreeType 库的实例，相当于"上下文/句柄"：

```c
FT_Library library;
FT_Init_FreeType(&library);    // 创建（成功返回 0）
FT_Done_FreeType(library);     // 释放
```

- 一个进程通常**只初始化一次**，是所有其他对象的根。
- `FT_New_Face()` 等接口都需要它作为第一个参数。

### 3.3 FT_Face

一个字体文件**加载后的实例**（字体脸）：

```c
FT_Face face;
FT_New_Face(library, "/path/font.ttc", 0, &face);  // 第三个参数 = face_index
```

- 一个 `.ttc` 集合文件可以包含多个 face，用 `face_index` 选择第几个。
- 常用成员：
  - `face->glyph` → `FT_GlyphSlot`（当前字形）
  - `face->size->metrics.height` → 行高（26.6）
  - `face->charmap` → 字符编码映射表
- 字号通过 `FT_Set_Pixel_Sizes()` / `FT_Set_Char_Size()` 设置在该对象上。

### 3.4 FT_GlyphSlot

**字形槽**，存放"当前正在处理的字形"的渲染结果：

- 通过 `face->glyph` 获取，加载字符后立刻读取其中的内容。
- 常用成员：

| 成员 | 含义 | 单位 |
|------|------|------|
| `slot->bitmap` | 字形位图 | — |
| `slot->bitmap_left` | 字形左边缘相对 pen 的 X 偏移 | 像素 |
| `slot->bitmap_top` | 字形顶部相对基线的距离 | 像素 |
| `slot->advance.x/y` | 到下一个字符的步进 | 26.6 |

- ⚠️ 每次 `FT_Load_Char()` 都会**覆盖**它的内容，必须立即使用，不能缓存指针。

### 3.5 FT_Bitmap

字形位图，是渲染的最终产物，也是我们真正要读的像素数据：

```c
unsigned int  rows;      // 位图高（像素）
unsigned int  width;     // 位图宽（像素）
int           pitch;     // 每行字节数（可能 > width，行对齐）
unsigned char *buffer;   // 像素数据首地址
unsigned char pixel_mode;// FT_PIXEL_MODE_MONO / GRAY / LCD
```

**读像素**（用 `pitch` 定位行，不要用 `width`，原因同 framebuffer 的 `line_length`）：

```c
// GRAY 模式（8bit/像素）：
unsigned char gray = bitmap->buffer[row * bitmap->pitch + col];

// MONO 模式（1bit/像素）：
unsigned char byte = bitmap->buffer[row * bitmap->pitch + col/8];
int bit = (byte >> (7 - col%8)) & 1;
```

写屏时判断 `gray > 0`（或 `bit == 1`）即表示该点属于字形，填上颜色即可。

### 3.6 FT_Vector / FT_Matrix（字形变换）

两者配合 `FT_Set_Transform()` 实现**旋转、斜体、平移**：

```c
FT_Vector pen;      // 平移量，26.6 定点（× 64）
FT_Matrix matrix;   // 2×2 变换矩阵，16.16 定点（× 65536）

FT_Set_Transform(face, &matrix, &pen);
```

- `FT_Vector`：`pen.x = 10 * 64;` 表示水平平移 10 像素。
- `FT_Matrix`：四个元素 `xx / xy / yx / yy`，旋转矩阵为

```text
[ xx  xy ]   =   [ cosθ  -sinθ ]
[ yx  yy ]       [ sinθ   cosθ ]
```

```c
float rad = angle * M_PI / 180;              // 角度转弧度
matrix.xx = (FT_Fixed)( cos(rad) * 0x10000L);
matrix.xy = (FT_Fixed)(-sin(rad) * 0x10000L);
matrix.yx = (FT_Fixed)( sin(rad) * 0x10000L);
matrix.yy = (FT_Fixed)( cos(rad) * 0x10000L);
```

> 用整数定点数（× 64 / × 65536）替代浮点，是为了在没有 FPU 的嵌入式平台上也能高效运算。

---

## 四、交叉编译移植（Lesson20）

依赖顺序：**zlib → libpng → freetype**

### 4.1 环境准备

```bash
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi
```

### 4.2 移植步骤

```bash
# zlib
./configure --prefix=/home/zr-arm/tools/zlib && make && make install

# libpng（依赖 zlib，先 export 路径）
export LDFLAGS="$LDFLAGS -L/home/zr-arm/tools/zlib/lib"
export CFLAGS="$CFLAGS -I/home/zr-arm/tools/zlib/include"
export CPPFLAGS="$CPPFLAGS -I/home/zr-arm/tools/zlib/include"
./configure --prefix=/home/zr-arm/tools/png --host=arm-poky-linux-gnueabi && make && make install

# freetype（依赖 zlib + libpng）
./configure --prefix=/home/zr-arm/tools/freetype --host=arm-poky-linux-gnueabi \
    --with-zlib=yes --with-bzip2=no --with-png=yes --with-harfbuzz=no \
    ZLIB_CFLAGS="-I/home/zr-arm/tools/zlib/include -L/home/zr-arm/tools/zlib/lib" ZLIB_LIBS=-lz \
    LIBPNG_CFLAGS="-I/home/zr-arm/tools/png/include -L/home/zr-arm/tools/png/lib" LIBPNG_LIBS=-lpng \
    && make && make install
```

之后将安装在pc机的相关库的lib打包发送给开发板，然后在开发板上解压缩到相应目录



### 4.4 编译测试程序

```bash
arm-linux-gnueabihf-gcc \
    -I/home/zr-arm/tools/freetype/include/freetype2 \
    -L/home/zr-arm/tools/freetype/lib -lfreetype \
    -L/home/zr-arm/tools/zlib/lib -lz \
    -L/home/zr-arm/tools/png/lib -lpng \
    -lm -o test_freetype test_freetype.c
```

### 4.4 注意事项

1. 头文件路径是 `freetype/include/freetype2`（不是 `freetype/include`）
2. 链接顺序：`-lfreetype -lz -lpng`，被依赖库放后面
3. 移植 configure 用 `arm-poky-linux-gnueabi`（Yocto SDK），编译用 `arm-linux-gnueabihf-gcc`，两者不同
4. 中文渲染需字体文件支持中文 + 用 `L'汉'` 宽字符
