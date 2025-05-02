cmake_minimum_required(VERSION 3.15)
project(msdfgen)
set(CMAKE_C_STANDARD 99)

if (APPLE)
    enable_language(OBJC)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(
        -Wno-deprecated-declarations
        -Wno-unused-function
        )
endif()

set(FREETYPE_SRC
    # modules/freetype/src/bzip2/ftbzip2.c
    modules/freetype/src/base/ftbdf.c
    modules/freetype/src/base/ftbbox.c
    modules/freetype/src/base/ftpatent.c
    modules/freetype/src/base/ftcid.c
    modules/freetype/src/base/ftwinfnt.c
    modules/freetype/src/base/ftsynth.c
    modules/freetype/src/base/ftmm.c
    modules/freetype/src/base/ftstroke.c
    modules/freetype/src/base/ftpfr.c
    modules/freetype/src/base/ftfstype.c
    modules/freetype/src/base/ftotval.c
    modules/freetype/src/base/fttype1.c
    modules/freetype/src/base/ftinit.c
    modules/freetype/src/base/ftgxval.c
    modules/freetype/src/base/ftgasp.c
    modules/freetype/src/base/ftglyph.c
    modules/freetype/src/base/ftbitmap.c
    modules/freetype/src/bdf/bdf.c
    modules/freetype/src/cache/ftcache.c
    modules/freetype/src/cid/type1cid.c
    modules/freetype/src/lzw/ftlzw.c
    modules/freetype/src/svg/svg.c
    modules/freetype/src/raster/raster.c
    modules/freetype/src/winfonts/winfnt.c
    modules/freetype/src/smooth/smooth.c
    modules/freetype/src/pcf/pcf.c
    modules/freetype/src/gzip/ftgzip.c
    modules/freetype/src/pfr/pfr.c
    # modules/freetype/src/base/ftver.rc.res
    modules/freetype/src/pshinter/pshinter.c
    modules/freetype/src/type42/type42.c
    modules/freetype/src/sdf/sdf.c
    modules/freetype/src/cff/cff.c
    modules/freetype/src/psnames/psnames.c
    modules/freetype/src/autofit/autofit.c
    modules/freetype/src/type1/type1.c
    modules/freetype/src/base/ftbase.c
    modules/freetype/src/psaux/psaux.c
    modules/freetype/src/truetype/truetype.c
    modules/freetype/src/sfnt/sfnt.c
    )

set(FREETYPE_DEFINES
    FT_CONFIG_CONFIG_H=<freetype/config/ftconfig.h>
    FT_CONFIG_OPTIONS_H=<freetype/config/ftoption.h>
    FT_CONFIG_STANDARD_LIBRARY_H=<freetype/config/ftstdlib.h>
    FT_CONFIG_MODULES_H=<freetype/config/ftmodule.h>
    FT_BEGIN_HEADER=;
    FT_END_HEADER=;
    FT2_BUILD_LIBRARY
)
if (WIN32)
    list(APPEND FREETYPE_SRC
        modules/freetype/builds/windows/ftdebug.c
        modules/freetype/builds/windows/ftsystem.c
        )
    list(APPEND FREETYPE_DEFINES
        _CRT_SECURE_NO_WARNINGS
        )
elseif(APPLE)
    list(APPEND FREETYPE_SRC
        modules/freetype/builds/mac/ftmac.c
        modules/freetype/src/base/ftdebug.c
        modules/freetype/builds/unix/ftsystem.c
        )
    list(APPEND FREETYPE_DEFINES
        HAVE_FCNTL_H
        HAVE_UNISTD_H
        )
endif()

add_library(freetype2 STATIC ${FREETYPE_SRC})

target_include_directories(freetype2 PRIVATE
    modules/freetype/include
    )
target_compile_definitions(freetype2 PRIVATE ${FREETYPE_DEFINES})

set(MSDFGEN_SRC
    core/contour-combiners.cpp
    core/Contour.cpp
    core/DistanceMapping.cpp
    core/edge-coloring.cpp
    core/edge-segments.cpp
    core/edge-selectors.cpp
    core/EdgeHolder.cpp
    core/equation-solver.cpp
    core/export-svg.cpp
    core/msdf-error-correction.cpp
    core/MSDFErrorCorrection.cpp
    core/msdfgen.cpp
    core/Projection.cpp
    core/rasterization.cpp
    core/render-sdf.cpp
    core/save-bmp.cpp
    core/save-fl32.cpp
    core/save-rgba.cpp
    core/save-tiff.cpp
    core/Scanline.cpp
    core/sdf-error-estimation.cpp
    core/shape-description.cpp
    core/Shape.cpp

    ext/import-font.cpp
)
set(MSDFGEN_LIBS "freetype2")
if (WIN32)
    list(APPEND MSDFGEN_SRC xhl_win.c)
elseif(APPLE)
    list(APPEND MSDFGEN_SRC xhl_mac.m)
    list(APPEND MSDFGEN_LIBS "-framework AppKit")
endif()

add_executable(msdfgen_cli main.cpp ${MSDFGEN_SRC})
target_compile_definitions(msdfgen_cli PRIVATE
    MSDFGEN_PUBLIC=;
    MSDFGEN_STANDALONE

    MSDFGEN_EXTENSIONS
    MSDFGEN_DISABLE_SVG
    MSDFGEN_DISABLE_PNG
    MSDFGEN_DISABLE_VARIABLE_FONTS
    )
target_include_directories(msdfgen_cli PRIVATE
    modules/freetype/include
    modules/xhl/include
    )
target_link_libraries(msdfgen_cli PRIVATE ${MSDFGEN_LIBS})

# benchmark
add_executable(msdfgen_benchmark example.cpp ${MSDFGEN_SRC})
target_compile_definitions(msdfgen_benchmark PRIVATE
    MSDFGEN_PUBLIC=;
    MSDFGEN_STANDALONE

    MSDFGEN_EXTENSIONS
    MSDFGEN_DISABLE_SVG
    MSDFGEN_DISABLE_PNG
    MSDFGEN_DISABLE_VARIABLE_FONTS
    )
target_include_directories(msdfgen_benchmark PRIVATE
    modules/freetype/include
    modules/xhl/include
    )
target_link_libraries(msdfgen_benchmark PRIVATE ${MSDFGEN_LIBS})
