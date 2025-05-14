#include "raqm.h"
#include <xhl/debug.h>

#define println(fmt, ...) fprintf(stderr, fmt "\n", __VA_ARGS__)

// int main(int argc, char* argv[])
int main()
{
    const char* fontfile;
    const char* text;
    const char* direction;
    const char* language;
    int         ret = 1;

    FT_Library library = NULL;
    FT_Face    face    = NULL;

    // if (argc < 5)
    // {
    //     printf("Usage: %s FONT_FILE TEXT DIRECTION LANG\n", argv[0]);
    //     return 1;
    // }

    // fontfile = argv[1];
    // text     = argv[2];
    // direction = argv[3];
    // language  = argv[4];

    // https://utf8everywhere.org/
    // UTF8    = 1 byte per character
    // Приве́т  = 2 bytes
    // नमस्ते     = 3 bytes
    // שלום = 3 בייטים
    // 🐨       = 4 bytes
    fontfile  = "C:\\Windows\\Fonts\\arialbd.ttf";
    text      = "UTF8 Приве́т नमस्ते שָׁלוֹם 🐨";
    direction = "r";
    language  = "en";

    if (FT_Init_FreeType(&library) == 0)
    {
        if (FT_New_Face(library, fontfile, 0, &face) == 0)
        {
            if (FT_Set_Char_Size(face, 16 * 64, 0, 72, 72) == 0)
            {
                raqm_t* rq = raqm_create();
                if (rq != NULL)
                {
                    raqm_direction_t dir = RAQM_DIRECTION_DEFAULT;

                    if (strcmp(direction, "r") == 0)
                        dir = RAQM_DIRECTION_RTL;
                    else if (strcmp(direction, "l") == 0)
                        dir = RAQM_DIRECTION_LTR;

                    if (raqm_set_text_utf8(rq, text, strlen(text)) && raqm_set_freetype_face(rq, face) &&
                        raqm_set_par_direction(rq, dir) && raqm_set_language(rq, language, 0, strlen(text)) &&
                        raqm_layout(rq))
                    {
                        size_t        count, i;
                        raqm_glyph_t* glyphs = raqm_get_glyphs(rq, &count);

                        float cursor_x = 0;
                        float cursor_y = 0;

                        ret = !(glyphs != NULL || count == 0);

                        println("glyph count: %zu", count);

                        for (i = 0; i < count; i++)
                        {
                            raqm_glyph_t* g         = glyphs + i;
                            float         x_offset  = (float)glyphs[i].x_offset / 64.0f;
                            float         y_offset  = (float)glyphs[i].y_offset / 64.0f;
                            float         x_advance = (float)glyphs[i].x_advance / 64.0f;
                            float         y_advance = (float)glyphs[i].y_advance / 64.0f;
                            println(
                                "gid #%d off: (%.2f, %.2f) adv: (%.2f, %.2f) idx: %d cursor: (%.2f, %.2f)",
                                glyphs[i].index,
                                x_offset,
                                y_offset,
                                x_advance,
                                y_advance,
                                glyphs[i].cluster,
                                cursor_x,
                                cursor_y);

                            // size_t index = glyphs[i].cluster;
                            // int    x = 0, y = 0;
                            // bool   ok = raqm_index_to_position(rq, &index, &x, &y);
                            // xassert(ok);
                            // float x_f = (float)x / 64.0f;
                            // float y_f = (float)y / 64.0f;
                            // println("index: %zu, %.2fx%.2f", index, x_f, y_f);

                            cursor_x += x_advance;
                            cursor_y += y_advance;
                        }
                    }

                    raqm_destroy(rq);
                }
            }

            FT_Done_Face(face);
        }

        FT_Done_FreeType(library);
    }

    return ret;
}