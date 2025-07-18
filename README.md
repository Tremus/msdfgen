Nothing to see here. Just me hacking away at MSDF.

Initial thoughts at the time of writing:

-   Build system is too complicated for my own needs. Worth rewriting
-   Freetype is even more complicated. It may be possible to replace this dependacy with `stb_truetype.h`. All Freetype is used for is extracting glyphs from fonts. STB should be up to this task (.tt only).
-   When building in release, it takes 110-130ms to rasterize ~94 glyphs (standard latin characters and symbols found on a US keybaord) on a single thread on my Intel 185H CPU with Clang 18 & MSVC 19, and ~90ms on my MacBook Air M1. This task is slow, so here are a few ideas to speed things up:
    -   The task is trivially parallelisable, and good work for a threadpool. Asumming the threadpool uses 8 threads (more is likely on modern machines), we can calculate 110ms / 8 = ~13ms. These times are much more acceptable.
    -   A quick trip to Visual Studios sampling profiler shows MultiDistanceSelector::addEdge() glowing red hot. The functions consuming the most time are all vritual functions on classes contained in `edge-segments.h`. The use of virtual functions and polymorphism could be replaced by enum types and functions could be dispatched via switch cases. The edge segments container class `EdgeHolder.h` holds an array of pointers to these edge segment nodes, which could be replaced with a flat array of objects. In general there are likely several opportunities for using arena allocators.
-   A C interface for a few functions would be nice
-   A font texture atlas
-   Text layout. The layout used by `fontstash.h` or `nanovg.h` could probably be rewritten to use whatever text atlas I build.

Some of these assumptions may prove to be incorrect as I continue to learn more.

(Update)
_The next day..._

It turns out some clever people fixed the speed/build/interface problems years ago. Here are two good finds:

-   C version [msdf-c](https://github.com/Tremus/msdf_c)

    I've included my own fork here because it contains the benchmark I used, + fixes by [@garettbass](https://github.com/garettbass).

    This is a neat C99 single header library, ~1.4k locs. The Freetype dependency was replaced by STB TrueType. It supports custom allocators.
    Rendering the same glyphs with the same height takes only ~45ms on a single thread. Using a custom arena allocator only yielded a 1ms speedup. Glyphs are slightly smaller with this library, you request a height and it determines enough width to fit the glyph, preventing a lot of overdraw. For a height of 32px in the Arial Bold font, the largest allocation made by my arena was ~27kb for the character '@'.

-   OpenGL version [msdfgl](https://github.com/nyyManni/msdfgl). Parallel parts of the algorithm run on geometry shaders. This is blazing fast, but OpenGL only.

    Unfortunately I don't use OpenGL. A clever person may be able to convert these shaders to compute shaders. I am not that clever.

(Update)
_The next day..._

Upon further investigation, both the C and OpenGL version look like they use earlier verson of algorithm. The C version was first uploaded to GitHub around 2018, and the OpenGL version in 2019. The algorithm started to recieve some pretty good updates since 2019, which helped with rendering sharper text using smaller MSDF textures. To render a simple 'A' glyph with decent quality in the C version, you need to render a fairly big glyph, around 64x64 (padding will be removed), while Chlumsky's new version generally needs only 32x32. This is the difference between approx. 4kb & 16kb per glyph.

The general rule of thumb with rendering quality in the latest algo is the more detailed & more points the source glyph has, the more artifacts the MSDF will contain at lower resolutions. 32x32 is acceptable for a simple font like Arial Bold. Complex characters like 'g' and '@' in Arial Bold are more likely to have noticible artifacts when the character is scaled very large, however if it's rendered at an ordinary reading size eg. 12-16px then these artifacts are likely unnoticeable. It may be possible to estiamte an ideal MSDF size by doing a simple count of the number of points in a glyph.

By my estimation, Chlumsky's latest updates are too good to pass up, meaning the original C++ library will need some hacking. The old C version can serve as a good reference for final API design, as well integration with STBTT.

(Update)
_The next month..._

After learning more about Freetype and going down the HarfBuzz rabbit hole, I have become convinced these two libraries are a better solution for sharp small 2D multilingual text on Windows. Nothing else compares, which is a shame, because I hate both libraries. I will just have to suck it up. If you want text that looks as good as the text in Google Chrome, you either have to use the same technology that they do (freetype + harfbuzz) or spend years of research writing your own raster algorithms and learning the quirks of all the languages on earth. This is hard, and also why there isn't a great deal of competition (most alternatives only handle Latin, Cyrillic, or similar scripts. This may be fine for some projects).

On the off chance someone finds this repo and is going down similar rabbit holes, consider all of these articles mandatory reading:

-   Intro to why text is so hard: https://faultlore.com/blah/text-hates-you/
-   Great tut on how fonts work: https://freetype.org/freetype2/docs/tutorial/index.html (Read the whole tutorial)
-   What is HarfBuzz?: https://harfbuzz.github.io/what-is-harfbuzz.html (Read all the what/why sub articles)
-   State of Text Rendering 2024: https://behdad.org/text2024/

# Multi-channel signed distance field generator

This is a utility for generating signed distance fields from vector shapes and font glyphs,
which serve as a texture representation that can be used in real-time graphics to efficiently reproduce said shapes.
Although it can also be used to generate conventional signed distance fields best known from
[this Valve paper](https://steamcdn-a.akamaihd.net/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf)
and perpendicular distance fields, its primary purpose is to generate multi-channel distance fields,
using a method I have developed. Unlike monochrome distance fields, they have the ability
to reproduce sharp corners almost perfectly by utilizing all three color channels.

The following comparison demonstrates the improvement in image quality.

![demo-msdf16](https://user-images.githubusercontent.com/18639794/106391899-e37ebe80-63ef-11eb-988b-4764004bb196.png)
![demo-sdf16](https://user-images.githubusercontent.com/18639794/106391905-e679af00-63ef-11eb-96c3-993176330911.png)
![demo-sdf32](https://user-images.githubusercontent.com/18639794/106391906-e7aadc00-63ef-11eb-8f84-d402d0dd9174.png)

-   To learn more about this method, you can read my [Master's thesis](https://github.com/Chlumsky/msdfgen/files/3050967/thesis.pdf).
-   Check out my [MSDF-Atlas-Gen](https://github.com/Chlumsky/msdf-atlas-gen) if you want to generate entire font atlases for text rendering.
-   See what's new in the [changelog](CHANGELOG.md).

## Getting started

The project can be used either as a library or as a console program. It is divided into two parts, **[core](core)**
and **[extensions](ext)**. The core module has no dependencies and only uses bare C++. It contains all
key data structures and algorithms, which can be accessed through the [msdfgen.h](msdfgen.h) header.
Extensions contain utilities for loading fonts and SVG files, as well as saving PNG images.
Those are exposed by the [msdfgen-ext.h](msdfgen-ext.h) header. This module uses
[FreeType](https://freetype.org/),
[TinyXML2](https://www.grinninglizard.com/tinyxml2/),
[libpng](http://www.libpng.org/pub/png/libpng.html),
and (optionally) [Skia](https://skia.org/).

Additionally, there is the [main.cpp](main.cpp), which wraps the functionality into
a comprehensive standalone console program. To start using the program immediately,
there is a Windows binary available for download in the ["Releases" section](https://github.com/Chlumsky/msdfgen/releases).
To use the project as a library, you may install it via the [vcpkg](https://vcpkg.io) package manager as

```
vcpkg install msdfgen
```

Or, to build the project from source, you may use the included [CMake script](CMakeLists.txt).
In its default configuration, it requires [vcpkg](https://vcpkg.io) as the provider for third-party library dependencies.
If you set the environment variable `VCPKG_ROOT` to the vcpkg directory,
the CMake configuration will take care of fetching all required packages from vcpkg.

## Console commands

The standalone program is executed as

```
msdfgen.exe <mode> <input> <options>
```

where only the input specification is required.

Mode can be one of:

-   **sdf** &ndash; generates a conventional monochrome (true) signed distance field.
-   **psdf** &ndash; generates a monochrome signed perpendicular distance field.
-   **msdf** (default) &ndash; generates a multi-channel signed distance field using my new method.
-   **mtsdf** &ndash; generates a combined multi-channel and true signed distance field in the alpha channel.

The input can be specified as one of:

-   **-font \<filename.ttf\> \<character code\>** &ndash; to load a glyph from a font file.
    Character code can be expressed as either a decimal (63) or hexadecimal (0x3F) Unicode value, or an ASCII character
    in single quotes ('?').
-   **-svg \<filename.svg\>** &ndash; to load an SVG file. Note that only the last vector path in the file will be used.
-   **-shapedesc \<filename.txt\>**, -defineshape \<definition\>, -stdin &ndash; to load a text description of the shape
    from either a file, the next argument, or the standard input, respectively. Its syntax is documented further down.

The complete list of available options can be printed with **-help**.
Some of the important ones are:

-   **-o \<filename\>** &ndash; specifies the output file name. The desired format will be deduced from the extension
    (png, bmp, tiff, rgba, fl32, txt, bin). Otherwise, use -format.
-   **-dimensions \<width\> \<height\>** &ndash; specifies the dimensions of the output distance field (in pixels).
-   **-range \<range\>**, **-pxrange \<range\>** &ndash; specifies the width of the range around the shape
    between the minimum and maximum representable signed distance in shape units or distance field pixels, respectivelly.
-   **-scale \<scale\>** &ndash; sets the scale used to convert shape units to distance field pixels.
-   **-translate \<x\> \<y\>** &ndash; sets the translation of the shape in shape units. Otherwise the origin (0, 0)
    lies in the bottom left corner.
-   **-autoframe** &ndash; automatically frames the shape to fit the distance field. If the output must be precisely aligned,
    you should manually position it using -translate and -scale instead.
-   **-angle \<angle\>** &ndash; specifies the maximum angle to be considered a corner.
    Can be expressed in radians (3.0) or degrees with D at the end (171.9D).
-   **-testrender \<filename.png\> \<width\> \<height\>** - tests the generated distance field by using it to render an image
    of the original shape into a PNG file with the specified dimensions. Alternatively, -testrendermulti renders
    an image without combining the color channels, and may give you an insight in how the multi-channel distance field works.
-   **-exportshape \<filename.txt\>** - saves the text description of the shape with edge coloring to the specified file.
    This can be later edited and used as input through -shapedesc.
-   **-printmetrics** &ndash; prints some useful information about the shape's layout.

For example,

```
msdfgen.exe msdf -font C:\Windows\Fonts\arialbd.ttf 'M' -o msdf.png -dimensions 32 32 -pxrange 4 -autoframe -testrender render.png 1024 1024
```

will take the glyph capital M from the Arial Bold typeface, generate a 32&times;32 multi-channel distance field
with a 4 pixels wide distance range, store it into msdf.png, and create a test render of the glyph as render.png.

**Note:** Do not use `-autoframe` to generate character maps! It is intended as a quick preview only.

## Library API

If you choose to use this utility inside your own program, there are a few simple steps you need to perform
in order to generate a distance field. Please note that all classes and functions are in the `msdfgen` namespace.

-   Acquire a `Shape` object. You can either load it via `loadGlyph` or `loadSvgShape`, or construct it manually.
    It consists of closed contours, which in turn consist of edges. An edge is represented by a `LinearEdge`, `QuadraticEdge`,
    or `CubicEdge`. You can construct them from two endpoints and 0 to 2 Bézier control points.
-   Normalize the shape using its `normalize` method and assign colors to edges if you need a multi-channel SDF.
    This can be performed automatically using the `edgeColoringSimple` (or other) heuristic, or manually by setting each edge's
    `color` member. Keep in mind that at least two color channels must be turned on in each edge.
-   Call `generateSDF`, `generatePSDF`, `generateMSDF`, or `generateMTSDF` to generate a distance field into a floating point
    `Bitmap` object. This can then be worked with further or saved to a file using `saveBmp`, `savePng`, `saveTiff`, etc.
-   You may also render an image from the distance field using `renderSDF`. Consider calling `simulate8bit`
    on the distance field beforehand to simulate the standard 8 bits/channel image format.

Example:

```c++
#include <msdfgen.h>
#include <msdfgen-ext.h>

using namespace msdfgen;

int main() {
    if (FreetypeHandle *ft = initializeFreetype()) {
        if (FontHandle *font = loadFont(ft, "C:\\Windows\\Fonts\\arialbd.ttf")) {
            Shape shape;
            if (loadGlyph(shape, font, 'A', FONT_SCALING_EM_NORMALIZED)) {
                shape.normalize();
                //                      max. angle
                edgeColoringSimple(shape, 3.0);
                //          output width, height
                Bitmap<float, 3> msdf(32, 32);
                //                            scale, translation (in em's)
                SDFTransformation t(Projection(32.0, Vector2(0.125, 0.125)), Range(0.125));
                generateMSDF(msdf, shape, t);
                savePng(msdf, "output.png");
            }
            destroyFont(font);
        }
        deinitializeFreetype(ft);
    }
    return 0;
}
```

## Using a multi-channel distance field

Using a multi-channel distance field generated by this program is similarly simple to how a monochrome distance field is used.
The only additional operation is computing the **median** of the three channels inside the fragment shader,
right after sampling the distance field. This signed distance value can then be used the same way as usual.

The following is an example GLSL fragment shader with anti-aliasing:

```glsl
in vec2 texCoord;
out vec4 color;
uniform sampler2D msdf;
uniform vec4 bgColor;
uniform vec4 fgColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(msdf, texCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    color = mix(bgColor, fgColor, opacity);
}
```

Here, `screenPxRange()` represents the distance field range in output screen pixels. For example, if the pixel range was set to 2
when generating a 32x32 distance field, and it is used to draw a quad that is 72x72 pixels on the screen,
it should return 4.5 (because 72/32 \* 2 = 4.5).
**For 2D rendering, this can generally be replaced by a precomputed uniform value.**

For rendering in a **3D perspective only**, where the texture scale varies across the screen,
you may want to implement this function with fragment derivatives in the following way.
I would suggest precomputing `unitRange` as a uniform variable instead of `pxRange` for better performance.

```glsl
uniform float pxRange; // set to distance field's pixel range

float screenPxRange() {
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(msdf, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(texCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}
```

`screenPxRange()` must never be lower than 1. If it is lower than 2, there is a high probability that the anti-aliasing will fail
and you may want to re-generate your distance field with a wider range.

## Shape description syntax

The text shape description has the following syntax.

-   Each closed contour is enclosed by braces: `{ <contour 1> } { <contour 2> }`
-   Each point (and control point) is written as two real numbers separated by a comma.
-   Points in a contour are separated with semicolons.
-   The last point of each contour must be equal to the first, or the symbol `#` can be used, which represents the first point.
-   There can be an edge segment specification between any two points, also separated by semicolons.
    This can include the edge's color (`c`, `m`, `y` or `w`) and/or one or two Bézier curve control points inside parentheses.

For example,

```
{ -1, -1; m; -1, +1; y; +1, +1; m; +1, -1; y; # }
```

would represent a square with magenta and yellow edges,

```
{ 0, 1; (+1.6, -0.8; -1.6, -0.8); # }
```

is a teardrop shape formed by a single cubic Bézier curve.
