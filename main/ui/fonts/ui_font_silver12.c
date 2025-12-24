/*******************************************************************************
 * Size: 12 px
 * Bpp: 4
 * Opts: --bpp 4 --size 12 --font C:/Users/Mizore/Desktop/新建文件夹 (2)/assets/Silver.ttf -o C:/Users/Mizore/Desktop/新建文件夹 (2)/assets\ui_font_silver12.c --format lvgl -r 0x20 -r 0x23 -r 0x25 -r 0x2B -r 0x2D -r 0x2E -r 0x30-0x39 -r 0x41-0x5A -r 0x61-0x7A -r 0xB0 -r 0x2103 --no-compress --no-prefilter --force-fast-kern-format
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_SILVER12
#define UI_FONT_SILVER12 1
#endif

#if UI_FONT_SILVER12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0023 "#" */
    0x66, 0x80, 0xcc, 0xd1, 0x66, 0x80, 0x66, 0x80,
    0xcc, 0xd1, 0x66, 0x80,

    /* U+0025 "%" */
    0x32, 0x0, 0xa9, 0x41, 0x1, 0x50, 0x39, 0x0,
    0x67, 0xa1, 0x1, 0x50,

    /* U+002B "+" */
    0x3, 0x0, 0xa, 0x0, 0xad, 0xa1, 0xa, 0x0,

    /* U+002D "-" */
    0xaa, 0xa1,

    /* U+002E "." */
    0x3d, 0x50, 0xa0,

    /* U+0030 "0" */
    0x3a, 0x50, 0xa0, 0x72, 0xa1, 0xd2, 0xd9, 0x72,
    0xa0, 0x72, 0x7a, 0x80,

    /* U+0031 "1" */
    0xa, 0xac, 0xa, 0xa, 0xa, 0xa,

    /* U+0032 "2" */
    0x3a, 0x50, 0xa0, 0x72, 0x30, 0x72, 0x7, 0x50,
    0x62, 0x0, 0xea, 0xa1,

    /* U+0033 "3" */
    0xaa, 0xd2, 0x1, 0x80, 0x7, 0x0, 0x11, 0x62,
    0x60, 0x62, 0x4a, 0x50,

    /* U+0034 "4" */
    0xc, 0x80, 0x69, 0x80, 0x92, 0x80, 0xa1, 0x80,
    0xcb, 0xd1, 0x1, 0x80,

    /* U+0035 "5" */
    0xda, 0xa1, 0xa0, 0x0, 0xaa, 0x81, 0x0, 0x72,
    0xa0, 0x72, 0x6a, 0x70,

    /* U+0036 "6" */
    0x7, 0x50, 0xa2, 0x0, 0xda, 0x50, 0xa0, 0x72,
    0xa0, 0x72, 0x7a, 0x80,

    /* U+0037 "7" */
    0xaa, 0xd2, 0x1, 0xa0, 0x1, 0x20, 0xa, 0x0,
    0x64, 0x0, 0x64, 0x0,

    /* U+0038 "8" */
    0x4a, 0x60, 0xa0, 0x72, 0x33, 0x50, 0x4a, 0x60,
    0xa0, 0x72, 0x6a, 0x70,

    /* U+0039 "9" */
    0x3a, 0x50, 0xa0, 0x72, 0xa0, 0x72, 0x5a, 0xd2,
    0x1, 0x60, 0x49, 0x0,

    /* U+0041 "A" */
    0xa, 0x0, 0x47, 0x50, 0x66, 0x80, 0xcc, 0xd1,
    0xa0, 0x72, 0xa0, 0x72,

    /* U+0042 "B" */
    0xda, 0x50, 0xa0, 0x72, 0xa0, 0x72, 0xda, 0xc2,
    0xa0, 0x72, 0xea, 0x50,

    /* U+0043 "C" */
    0x3a, 0x50, 0x80, 0x41, 0xa0, 0x0, 0xa0, 0x0,
    0x80, 0x41, 0x4a, 0x50,

    /* U+0044 "D" */
    0xd9, 0x0, 0xa0, 0x60, 0xa0, 0x72, 0xa0, 0x72,
    0xa1, 0x60, 0xe9, 0x0,

    /* U+0045 "E" */
    0xda, 0xa1, 0xa0, 0x0, 0xa0, 0x0, 0xda, 0x50,
    0xa0, 0x0, 0xea, 0xa1,

    /* U+0046 "F" */
    0xda, 0xa1, 0xa0, 0x0, 0xa0, 0x0, 0xda, 0x50,
    0xa0, 0x0, 0xa0, 0x0,

    /* U+0047 "G" */
    0x3a, 0x50, 0xa0, 0x51, 0xa0, 0x0, 0xa7, 0xb2,
    0xa0, 0x72, 0x7a, 0xd2,

    /* U+0048 "H" */
    0xa0, 0x72, 0xa0, 0x72, 0xa0, 0x72, 0xda, 0xd2,
    0xa0, 0x72, 0xa0, 0x72,

    /* U+0049 "I" */
    0x3d, 0x50, 0xa0, 0xa, 0x0, 0xa0, 0xa, 0x4,
    0xe5,

    /* U+004A "J" */
    0x0, 0x72, 0x0, 0x72, 0x0, 0x72, 0x60, 0x72,
    0xa0, 0x72, 0x7a, 0x80,

    /* U+004B "K" */
    0xa0, 0x41, 0xa7, 0x50, 0xf4, 0x0, 0xb7, 0x0,
    0xa0, 0x60, 0xa0, 0x72,

    /* U+004C "L" */
    0xa0, 0x0, 0xa0, 0x0, 0xa0, 0x0, 0xa0, 0x0,
    0xa0, 0x0, 0xea, 0xa1,

    /* U+004D "M" */
    0xa0, 0x72, 0xda, 0xd2, 0xa0, 0x72, 0xa0, 0x72,
    0xa0, 0x72, 0xa0, 0x72,

    /* U+004E "N" */
    0xa0, 0x72, 0xd9, 0x72, 0xa1, 0xd2, 0xa0, 0x72,
    0xa0, 0x72, 0xa0, 0x72,

    /* U+004F "O" */
    0x3a, 0x50, 0xa0, 0x72, 0xa0, 0x72, 0xa0, 0x72,
    0xa0, 0x72, 0x7a, 0x80,

    /* U+0050 "P" */
    0xda, 0x50, 0xa0, 0x72, 0xa0, 0x72, 0xda, 0x50,
    0xa0, 0x0, 0xa0, 0x0,

    /* U+0051 "Q" */
    0xaa, 0xa1, 0xa0, 0x72, 0xa0, 0x72, 0xa0, 0x72,
    0xa0, 0x72, 0x4a, 0x50, 0x1, 0xa8,

    /* U+0052 "R" */
    0xda, 0x50, 0xa0, 0x72, 0xa0, 0x72, 0xdc, 0x50,
    0xa5, 0x60, 0xa0, 0x72,

    /* U+0053 "S" */
    0x3a, 0x50, 0xa0, 0x51, 0x92, 0x0, 0x12, 0x62,
    0x60, 0x62, 0x4a, 0x50,

    /* U+0054 "T" */
    0xad, 0xa1, 0xa, 0x0, 0xa, 0x0, 0xa, 0x0,
    0xa, 0x0, 0xa, 0x0,

    /* U+0055 "U" */
    0xa0, 0x72, 0xa0, 0x72, 0xa0, 0x72, 0xa0, 0x72,
    0xa0, 0x72, 0x7a, 0x80,

    /* U+0056 "V" */
    0xa0, 0x72, 0xa0, 0x72, 0x60, 0x41, 0x66, 0x80,
    0x47, 0x50, 0xa, 0x0,

    /* U+0057 "W" */
    0xaa, 0x72, 0xaa, 0x72, 0xaa, 0x72, 0x99, 0x72,
    0x66, 0x80, 0x66, 0x80,

    /* U+0058 "X" */
    0xa0, 0x82, 0x96, 0xa0, 0x11, 0x20, 0x6c, 0x80,
    0x41, 0x40, 0xa0, 0x72,

    /* U+0059 "Y" */
    0xa0, 0x82, 0x96, 0xa0, 0x14, 0x20, 0xa, 0x0,
    0xa, 0x0, 0xa, 0x0,

    /* U+005A "Z" */
    0xaa, 0xd2, 0x1, 0xa0, 0x0, 0x20, 0x6a, 0x0,
    0x41, 0x0, 0xea, 0xa1,

    /* U+0061 "a" */
    0x3a, 0x80, 0x4a, 0xd2, 0xa0, 0x72, 0x6a, 0xd2,

    /* U+0062 "b" */
    0xa0, 0x0, 0xa0, 0x0, 0xda, 0x50, 0xa0, 0x72,
    0xa0, 0x72, 0xea, 0x80,

    /* U+0063 "c" */
    0x3a, 0x50, 0x80, 0x41, 0x80, 0x41, 0x4a, 0x50,

    /* U+0064 "d" */
    0x0, 0x72, 0x0, 0x72, 0x3a, 0xd2, 0xa0, 0x72,
    0xa0, 0x72, 0x7a, 0xd2,

    /* U+0065 "e" */
    0xca, 0xb2, 0xda, 0xb2, 0x80, 0x41, 0x4a, 0x50,

    /* U+0066 "f" */
    0x4, 0xa1, 0xa, 0x0, 0xa, 0x0, 0xad, 0xa1,
    0xa, 0x0, 0xa, 0x0,

    /* U+0067 "g" */
    0xda, 0xd2, 0xa0, 0x72, 0x3a, 0xd2, 0x0, 0x72,
    0xaa, 0xb2,

    /* U+0068 "h" */
    0xa0, 0x0, 0xa0, 0x0, 0xb9, 0x80, 0xc1, 0x72,
    0xa0, 0x72, 0xa0, 0x72,

    /* U+0069 "i" */
    0x32, 0x0, 0xc4, 0x64, 0x64, 0xca,

    /* U+006A "j" */
    0x1, 0x50, 0x0, 0x8, 0x80, 0x18, 0x1, 0x80,
    0x18, 0xaa, 0x70,

    /* U+006B "k" */
    0xa0, 0x0, 0xa0, 0x0, 0xa1, 0x50, 0xd9, 0x0,
    0xa7, 0x50, 0xa0, 0x51,

    /* U+006C "l" */
    0xc4, 0x64, 0x64, 0x64, 0x64, 0x6a,

    /* U+006D "m" */
    0xdd, 0x80, 0xaa, 0x72, 0xaa, 0x72, 0xaa, 0x72,

    /* U+006E "n" */
    0xb9, 0x80, 0xc1, 0x72, 0xa0, 0x72, 0xa0, 0x72,

    /* U+006F "o" */
    0x3a, 0x50, 0xa0, 0x72, 0xa0, 0x72, 0x7a, 0x80,

    /* U+0070 "p" */
    0xda, 0xd2, 0xa0, 0x72, 0xda, 0x50, 0xa0, 0x0,
    0xa0, 0x0,

    /* U+0071 "q" */
    0xda, 0xd2, 0xa0, 0x72, 0x3a, 0xd2, 0x0, 0x72,
    0x0, 0x72,

    /* U+0072 "r" */
    0xa7, 0x50, 0xd2, 0x41, 0xa0, 0x0, 0xa0, 0x0,

    /* U+0073 "s" */
    0x3a, 0x50, 0x85, 0xb2, 0x60, 0x62, 0x4a, 0x50,

    /* U+0074 "t" */
    0x64, 0x0, 0x64, 0x0, 0xcb, 0x50, 0x64, 0x0,
    0x64, 0x20, 0x19, 0x80,

    /* U+0075 "u" */
    0xa0, 0x72, 0xa0, 0x72, 0xa0, 0xa2, 0x7a, 0xa2,

    /* U+0076 "v" */
    0xa0, 0x82, 0x96, 0xa0, 0x14, 0x20, 0xa, 0x0,

    /* U+0077 "w" */
    0xaa, 0x72, 0xaa, 0x72, 0x88, 0x71, 0x66, 0x80,

    /* U+0078 "x" */
    0x60, 0x41, 0x3d, 0x50, 0x35, 0x50, 0x60, 0x51,

    /* U+0079 "y" */
    0xa0, 0x72, 0xa0, 0x72, 0xaa, 0xd2, 0x0, 0x72,
    0xaa, 0xb2,

    /* U+007A "z" */
    0xab, 0xd1, 0x7, 0x20, 0x62, 0x0, 0xea, 0xa1,

    /* U+00B0 "°" */
    0x49, 0xa, 0x18, 0x59, 0x10,

    /* U+2103 "℃" */
    0x3a, 0x60, 0x0, 0x73, 0x81, 0x31, 0x5, 0x28,
    0x68, 0x0, 0xa0, 0x0, 0x0, 0xa0, 0x0, 0x0,
    0x90, 0x0, 0x0, 0x9, 0xa9
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 51, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 12, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 32, .adv_w = 61, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 34, .adv_w = 51, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 37, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 40, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 235, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 51, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 268, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 316, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 71, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 354, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 378, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 438, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 450, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 462, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 482, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 510, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 61, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 532, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 544, .adv_w = 40, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 550, .adv_w = 51, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 561, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 573, .adv_w = 40, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 579, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 587, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 603, .adv_w = 61, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 613, .adv_w = 61, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 623, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 631, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 651, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 659, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 667, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 675, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 683, .adv_w = 61, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 693, .adv_w = 61, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 701, .adv_w = 51, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 706, .adv_w = 111, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint8_t glyph_id_ofs_list_0[] = {
    0, 0, 0, 1, 0, 2, 0, 0,
    0, 0, 0, 3, 0, 4, 5, 0,
    6, 7, 8, 9, 10, 11, 12, 13,
    14, 15
};

static const uint16_t unicode_list_3[] = {
    0x0, 0x2053
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 26, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = glyph_id_ofs_list_0, .list_length = 26, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL
    },
    {
        .range_start = 65, .range_length = 26, .glyph_id_start = 17,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 97, .range_length = 26, .glyph_id_start = 43,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 176, .range_length = 8276, .glyph_id_start = 69,
        .unicode_list = unicode_list_3, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 4,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_silver12 = {
#else
lv_font_t ui_font_silver12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 8,          /*The maximum line height required by the font*/
    .base_line = 1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_SILVER12*/

