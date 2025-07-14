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

#include <SheenBidi.h>

#define SB_CONFIG_UNITY
#include <SheenBidi.c>

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

    // const char* font_path = "C:\\Windows\\Fonts\\arialbd.ttf";
    // const char* font_path = "C:\\Windows\\Fonts\\segoeui.ttf";
    // const char* font_path = "C:\\Windows\\Fonts\\segoeuib.ttf"; // bold
    const char* font_path = "C:\\Windows\\Fonts\\seguisb.ttf"; // semibold
    error                 = FT_New_Face(library, font_path, 0, &face);
    xassert(!error);

    println("Found %ld glyphs", face->num_glyphs);

    // Note: a width of 0 will automatically set the width of the glyph based on height
    const float FONT_SIZE = 10;
    const float DPI       = 96;
    FT_Set_Char_Size(face, 0, FONT_SIZE * 64, DPI, DPI);

    glyph_index = FT_Get_Char_Index(face, 'A');
    xassert(glyph_index != 0); // 0 if glyph not found

    hb_buffer_t* buf  = hb_buffer_create();
    hb_font_t*   font = hb_ft_font_create(face, NULL);

    // hb_language_t default_language = hb_language_get_default();
    // xassert(default_language != NULL);
    // hb_buffer_set_language(buf, default_language);
    // hb_buffer_set_language(buf, hb_language_from_string("en", -1));
    // hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);

    // const char* my_text     = "Sphinx of black quartz, judge my vow";
    // const char* my_text     = "AV. .W.V.";
    // const char* my_text     = "ft_glyph";
    // https://utf8everywhere.org/
    const char* my_text = "UTF8 Приве́т नमस्ते שָׁלוֹם 🐨";
    // const char*  my_text     = "یہ ایک )car( ہے۔";
    const size_t my_text_len = strlen(my_text);

    SBCodepointSequence codepointSequence = {SBStringEncodingUTF8, (void*)my_text, my_text_len};
    /* Extract the first bidirectional paragraph. */
    SBAlgorithmRef bidiAlgorithm   = SBAlgorithmCreate(&codepointSequence);
    SBParagraphRef firstParagraph  = SBAlgorithmCreateParagraph(bidiAlgorithm, 0, INT32_MAX, SBLevelDefaultLTR);
    SBUInteger     paragraphLength = SBParagraphGetLength(firstParagraph);

    /* Create a line consisting of the whole paragraph and get its runs. */
    SBLineRef    paragraphLine = SBParagraphCreateLine(firstParagraph, 0, paragraphLength);
    SBUInteger   runCount      = SBLineGetRunCount(paragraphLine);
    const SBRun* runArray      = SBLineGetRunsPtr(paragraphLine);

    println("my_text len: %llu", my_text_len);
    /* Log the details of each run in the line. */
    for (SBUInteger run_idx = 0; run_idx < runCount; run_idx++)
    {
        const SBRun* run = runArray + run_idx;
        println("Run Offset: %ld", (long)run->offset);
        println("Run Length: %ld", (long)run->length);
        println("Run Level: %ld\n", (long)run->level);

        hb_buffer_clear_contents(buf);
        hb_buffer_add_utf8(buf, my_text, my_text_len, run->offset, run->length);

        // hb_buffer_set_script(run->buffer, run->script);
        // xassert(my_text_len == 3);
        // hb_buffer_add_utf8(buf, my_text, my_text_len, 0, my_text_len);
        // hb_buffer_guess_segment_properties(buf);
        // hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
        // hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
        // hb_buffer_set_language(buf, hb_language_from_string("en", -1));

        hb_shape(font, buf, NULL, 0);

        unsigned int         glyph_count;
        hb_glyph_info_t*     glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
        hb_glyph_position_t* glyph_pos  = hb_buffer_get_glyph_positions(buf, &glyph_count);
        // xassert(glyph_count == 3);

        hb_position_t pen_x = 0;
        hb_position_t pen_y = 0;

        int max_font_height_pixels = (face->size->metrics.ascender - face->size->metrics.descender) >> 6;
        for (unsigned int i = 0; i < glyph_count; i++)
        {
            pen_x += glyph_pos[i].x_advance;
            pen_y += glyph_pos[i].y_advance;
        }

        // const int      ATLAS_WIDTH            = 512;
        // const int      ATLAS_HEIGHT           = 512;
        // const int      ATLAS_ROW_STRIDE       = ATLAS_WIDTH * 3;
        // unsigned char* atlas                  = calloc(1, ATLAS_HEIGHT * ATLAS_ROW_STRIDE);
        const int      LABEL_WIDTH      = pen_x >> 6;
        const int      LABEL_HEIGHT     = max_font_height_pixels;
        const int      LABEL_ROW_STRIDE = LABEL_WIDTH * 3;
        unsigned char* label            = calloc(1, LABEL_HEIGHT * LABEL_ROW_STRIDE);

        // Set pen position
        // https://freetype.org/freetype2/docs/tutorial/step2.html#section-1
        pen_x = 0;
        pen_y = max_font_height_pixels + (face->size->metrics.descender >> 6);

        for (unsigned int i = 0; i < glyph_count; i++)
        {
            const hb_glyph_info_t*     info = glyph_info + i;
            const hb_glyph_position_t* pos  = glyph_pos + i;

            hb_codepoint_t glyphid   = info->codepoint;
            hb_position_t  x_offset  = pos->x_offset;
            hb_position_t  y_offset  = pos->y_offset;
            hb_position_t  x_advance = pos->x_advance;
            hb_position_t  y_advance = pos->y_advance;

            int pos_x = pen_x + (pos->x_offset >> 6);
            int pos_y = pen_y + (pos->y_offset >> 6);

            println("Render glyph %d at %dx%d", glyphid, pos_x, pos_y);

            // Note: In Harfbuzz, after text has been 'shaped', info->codepoint will be set to the FreeType glyph index
            // AKA char index. eg: FT_UInt charindex = FT_Get_Char_Index(face, my_text[i]); xassert(charindex ==
            // info->codepoint);
            int err = FT_Load_Glyph(face, glyphid, FT_LOAD_DEFAULT);
            xassert(err == 0);
            FT_Render_Mode render_mode = FT_RENDER_MODE_LCD; // subpixel antialiasing, horizontal screen
            FT_Render_Glyph(face->glyph, render_mode);

            const FT_Bitmap* bmp = &face->glyph->bitmap;
            xassert(bmp->pixel_mode == FT_PIXEL_MODE_LCD);
            xassert((bmp->width % 3) == 0); // note: FT width is measured in bytes (subpixels)

            // Note all glyphs have height/rows... (spaces?)
            if (bmp->width && bmp->rows)
            {
                const int BMP_PEN_OFFSET_X = face->glyph->bitmap_left;
                const int BMP_PEN_OFFSET_Y = face->glyph->bitmap_top;

                int label_offset_x = pen_x + BMP_PEN_OFFSET_X;

                xassert(label_offset_x >= 0 && label_offset_x < LABEL_WIDTH);
                if (label_offset_x < 0)
                {
                    pen_x          += abs(label_offset_x);
                    label_offset_x  = 0;
                }
                label_offset_x *= 3;
                xassert(label_offset_x + bmp->width <= LABEL_ROW_STRIDE);

                for (int y = 0; y < bmp->rows; y++)
                {
                    int label_offset_y = y + pen_y - BMP_PEN_OFFSET_Y;
                    xassert(label_offset_y >= 0 && label_offset_y < LABEL_HEIGHT);

                    unsigned char* dst = label + label_offset_y * LABEL_ROW_STRIDE + label_offset_x;
                    unsigned char* src = bmp->buffer + y * bmp->pitch;

                    // memcpy(dst, src, bmp->width); // BAD!
                    for (int j = 0; j < bmp->width; j++)
                    {
                        // Bitwise OR is important. Some glyphs overlap eg "W.V"
                        dst[j] |= src[j];
                    }
                }

                // Save bitmap
                /*
                {
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
                    int width_pixels = bmp->width / 3;
                    snprintf(
                        path + path_len,
                        (sizeof(path) - path_len),
                        XFILES_DIR_STR "ft_glyph" XFILES_DIR_STR "glyph_%u_%dx%d.png",
                        glyphid,
                        width_pixels,
                        bmp->rows);

                    FILE* fp = fopen(path, "wb");
                    xassert(fp);
                    svpng(fp, width_pixels, bmp->rows, img_data, 0);
                    fclose(fp);

                    xfree(img_data);
                }
                */
            }

            pen_x += x_advance >> 6;
            pen_y += y_advance >> 6;
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
            XFILES_DIR_STR "ft_glyph" XFILES_DIR_STR "label_%dx%d.png",
            LABEL_WIDTH,
            LABEL_HEIGHT);

        FILE* fp = fopen(path, "wb");
        xassert(fp);
        svpng(fp, LABEL_WIDTH, LABEL_HEIGHT, label, 0);
        fclose(fp);

        free(label);
    }

    hb_buffer_destroy(buf);
    hb_font_destroy(font);

    /* Release all objects. */
    SBLineRelease(paragraphLine);
    SBParagraphRelease(firstParagraph);
    SBAlgorithmRelease(bidiAlgorithm);

    error = FT_Done_Face(face);
    xassert(!error);
    error = FT_Done_FreeType(library);
    xassert(!error);

    xalloc_shutdown();
    return 0;
}