#include "msdfgen.h"

#include "msdfgen-ext.h"

#include <xhl/debug.h>
#include <xhl/files.h>
#include <xhl/time.h>

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

using namespace msdfgen;

int main()
{
    xtime_init();

    FreetypeHandle* ft   = NULL;
    FontHandle*     font = NULL;
    Shape           shape;
    //          output width, height
    Bitmap<float, 3>        msdf(32, 32);
    const SDFTransformation t(Projection(32.0, Vector2(0.125, 0.125)), Range(0.125));

    bool ok = false;

    ft = initializeFreetype();
    xassert(ft);

#if defined(_WIN32)
    static const char* fontpath = "C:\\Windows\\Fonts\\arialbd.ttf";
#elif defined(__APPLE__)
    static const char* fontpath = "/Library/Fonts/Arial Unicode.ttf";
#endif

    font = loadFont(ft, fontpath);
    xassert(font);

    static const char* latin =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~`!@#$%^&*()-_=+,./<>?[]{}\\|;:'\"";
    const size_t latin_len = strlen(latin);

    uint64_t time_start = xtime_now_ns();
    for (int i = 0; i < latin_len; i++)
    {
        ok = loadGlyph(shape, font, latin[i], FONT_SCALING_EM_NORMALIZED);
        xassert(ok);

        shape.normalize();
        //                      max. angle
        edgeColoringSimple(shape, 3.0);
        //                            scale, translation (in em's)
        generateMSDF(msdf, shape, t);
    }
    uint64_t time_end = xtime_now_ns();
    double   time_ms  = xtime_convert_ns_to_ms(time_end - time_start);
    printf("Build %zu glyphs in %.2lfms", latin_len, time_ms);

    char   path[1024];
    size_t path_len = 0;
    memset(path, 0, sizeof(path));
    xfiles_get_user_directory(path, sizeof(path), XFILES_USER_DIRECTORY_DESKTOP);
    path_len = strlen(path);
    xassert(path_len < sizeof(path));
    snprintf(path + path_len, (sizeof(path) - path_len), "%s", XFILES_DIR_STR "output.bmp");

    saveBmp(msdf, path);

    destroyFont(font);
    deinitializeFreetype(ft);
    return 0;
}