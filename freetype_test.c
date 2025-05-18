#define XHL_ALLOC_IMPL
#define XHL_TIME_IMPL
#define XHL_FILES_IMPL

#include <ft2build.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <xhl/alloc.h>
#include <xhl/debug.h>
#include <xhl/files.h>
#include <xhl/time.h>

#include "svpng.h"

#include FT_FREETYPE_H

#include <hb.h>

#include <hb-ft.h>

#define println_impl(fmt, ...) fprintf(stderr, fmt "%s", __VA_ARGS__)
#define println(...)           println_impl(__VA_ARGS__, "\n")

int main()
{
    xtime_init();
    xalloc_init();
    println("Hello world");

    FT_Library library     = 0;
    FT_Face    face        = 0;
    int        error       = 0;
    int        glyph_index = 0;

    error = FT_Init_FreeType(&library);
    xassert(!error);

    error = FT_New_Face(library, "C:\\Windows\\Fonts\\arialbd.ttf", 0, &face);
    xassert(!error);

    println("Found %ld glyphs", face->num_glyphs);

#define FONT_SIZE 16
    FT_Set_Char_Size(face, FONT_SIZE * 64, FONT_SIZE * 64, 0, 0);

    glyph_index = FT_Get_Char_Index(face, 'A');
    xassert(glyph_index != 0); // 0 if glyph not found

    hb_buffer_t* buf = hb_buffer_create();

    // hb_language_t default_language = hb_language_get_default();
    // xassert(default_language != NULL);
    // hb_buffer_set_language(buf, default_language);

    const char* my_text     = "ABC";
    size_t      my_text_len = strlen(my_text);
    xassert(my_text_len == 3);
    hb_buffer_add_utf8(buf, my_text, my_text_len, 0, my_text_len);
    hb_buffer_guess_segment_properties(buf);
    // hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    // hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    // hb_buffer_set_language(buf, hb_language_from_string("en", -1));

    hb_font_t* font = hb_ft_font_create(face, NULL);
    hb_shape(font, buf, NULL, 0);

    unsigned int         glyph_count;
    hb_glyph_info_t*     glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* glyph_pos  = hb_buffer_get_glyph_positions(buf, &glyph_count);
    xassert(glyph_count == 3);

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;
    for (unsigned int i = 0; i < glyph_count; i++)
    {
        const hb_glyph_info_t*     info = glyph_info + i;
        const hb_glyph_position_t* pos  = glyph_pos + i;

        hb_codepoint_t glyphid   = info->codepoint;
        hb_position_t  x_offset  = pos->x_offset;
        hb_position_t  y_offset  = pos->y_offset;
        hb_position_t  x_advance = pos->x_advance;
        hb_position_t  y_advance = pos->y_advance;

        float pos_x = (float)(cursor_x + pos->x_offset) / (float)64;
        float pos_y = (float)(cursor_y + pos->y_offset) / (float)64;

        println("Render glyph %d at %.2fx%.2f", glyphid, pos_x, pos_y);

        // Note: In Harfbuzz, after text has been 'shaped', info->codepoint will be set to the FreeType glyph index AKA
        // char index.
        // eg: FT_UInt charindex = FT_Get_Char_Index(face, my_text[i]); xassert(charindex == info->codepoint);
        int err = FT_Load_Glyph(face, glyphid, FT_LOAD_DEFAULT);
        xassert(err == 0);
        FT_Render_Mode render_mode = FT_RENDER_MODE_LCD; // subpixel antialiasing, horizontal screen
        FT_Render_Glyph(face->glyph, render_mode);

        const FT_Bitmap* bmp = &face->glyph->bitmap;
        xassert(bmp->pixel_mode == FT_PIXEL_MODE_LCD);
        xassert((bmp->width % 3) == 0);

        unsigned char* img_data = xcalloc(1, bmp->width * bmp->rows);
        for (int i = 0; i < bmp->rows; i++)
        {
            unsigned char* src = bmp->buffer + i * bmp->pitch;
            unsigned char* dst = img_data + i * bmp->width;
            memcpy(dst, src, bmp->width);
        }

        char   path[1024];
        size_t path_len = 0;
        memset(path, 0, sizeof(path));
        xfiles_get_user_directory(path, sizeof(path), XFILES_USER_DIRECTORY_DESKTOP);
        path_len = strlen(path);
        xassert(path_len < sizeof(path));
        snprintf(
            path + path_len,
            (sizeof(path) - path_len),
            XFILES_DIR_STR "ft_glyph" XFILES_DIR_STR "glyph_%u_%dx%d.png",
            glyphid,
            bmp->width,
            bmp->rows);

        FILE* fp = fopen(path, "wb");
        svpng(fp, bmp->width / 3, bmp->rows, img_data, 0);
        fclose(fp);

        xfree(img_data);

        cursor_x += x_advance;
        cursor_y += y_advance;
    }

    hb_buffer_destroy(buf);
    hb_font_destroy(font);

    error = FT_Done_Face(face);
    xassert(!error);
    error = FT_Done_FreeType(library);
    xassert(!error);

    xalloc_shutdown();
    return 0;
}