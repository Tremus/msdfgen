#pragma once
#define RAQM_VERSION_MAJOR  0
#define RAQM_VERSION_MINOR  10
#define RAQM_VERSION_MICRO  2
#define RAQM_VERSION_STRING "0.10.2"

#define RAQM_VERSION_ATLEAST(major, minor, micro)                                                                      \
    ((major)*10000 + (minor)*100 + (micro) <=                                                                          \
     RAQM_VERSION_MAJOR * 10000 + RAQM_VERSION_MINOR * 100 + RAQM_VERSION_MICRO)