// Single translation unit that compiles the miniaudio implementation.
// Built only when EMBER_ENABLE_AUDIO is set; warnings are disabled for it in CMake.
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
