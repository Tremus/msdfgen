#include "msdfgen.h"

#include "msdfgen-ext.h"

#include <stdio.h>
#include <xhl/debug.h>
#include <xhl/files.h>
#include <xhl/time.h>

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

using namespace msdfgen;

SDFTransformation autoframe_shape(Shape* shape, int width, int height)
{
    Shape::Bounds bounds = shape->getBounds();

    const bool scaleSpecified = false;

    Vector2 translate;
    Vector2 scale    = 1;
    double  avgScale = .5 * (scale.x + scale.y);

    enum
    {
        RANGE_UNIT,
        RANGE_PX
    } rangeMode = RANGE_PX;
    Range range(1);
    const Range pxRange(2);

    // Auto-frame
    double  l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
    Vector2 frame(width, height);
    if (!scaleSpecified)
    {
        if (rangeMode == RANGE_UNIT)
            l += range.lower, b += range.lower, r -= range.lower, t -= range.lower;
        else
            frame += 2 * pxRange.lower;
    }
    if (l >= r || b >= t)
        l = 0, b = 0, r = 1, t = 1;
    if (frame.x <= 0 || frame.y <= 0)
    {
        // Cannot fit the specified pixel range!
        xassert(false);
    }
    Vector2 dims(r - l, t - b);
    if (scaleSpecified)
        translate = .5 * (frame / scale - dims) - Vector2(l, b);
    else
    {
        if (dims.x * frame.y < dims.y * frame.x)
        {
            translate.set(.5 * (frame.x / frame.y * dims.y - dims.x) - l, -b);
            scale = avgScale = frame.y / dims.y;
        }
        else
        {
            translate.set(-l, .5 * (frame.y / frame.x * dims.x - dims.y) - b);
            scale = avgScale = frame.x / dims.x;
        }
    }
    if (rangeMode == RANGE_PX && !scaleSpecified)
        translate -= pxRange.lower / scale;

    if (rangeMode == RANGE_PX)
        range = pxRange / min(scale.x, scale.y);


    return SDFTransformation(Projection(scale, translate), range);
}

int main()
{
    xtime_init();

    FreetypeHandle* ft   = NULL;
    FontHandle*     font = NULL;
    Shape           shape;
    //          output width, height
    Bitmap<float, 3> msdf(32, 32);

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
        int codepoint = latin[i];
        // int codepoint = '@';
        ok = loadGlyph(shape, font, codepoint, FONT_SCALING_NONE);
        xassert(ok);

        shape.normalize();
        //                      max. angle
        edgeColoringSimple(shape, 3.0);
        //                            scale, translation (in em's)
        const SDFTransformation transformation = autoframe_shape(&shape, msdf.width(), msdf.width());
        generateMSDF(msdf, shape, transformation);

        // char   path[1024];
        // size_t path_len = 0;
        // memset(path, 0, sizeof(path));
        // xfiles_get_user_directory(path, sizeof(path), XFILES_USER_DIRECTORY_DESKTOP);
        // path_len = strlen(path);
        // xassert(path_len < sizeof(path));
        // snprintf(
        //     path + path_len,
        //     (sizeof(path) - path_len),
        //     "%s/output/glyph_%d_%dx%d.bmp",
        //     XFILES_DIR_STR,
        //     codepoint,
        //     msdf.width(),
        //     msdf.height());

        // saveBmp(msdf, path);
    }
    uint64_t time_end = xtime_now_ns();
    double   time_ms  = xtime_convert_ns_to_ms(time_end - time_start);
    fprintf(stderr, "Build %zu glyphs in %.2lfms", latin_len, time_ms);

    destroyFont(font);
    deinitializeFreetype(ft);
    return 0;
}