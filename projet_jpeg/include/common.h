
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <complex.h>
#include <inttypes.h>

#define USAGE "Usage : %s <jpeg_file> [options]\n"\
              "\n"\
              "Options list :\n"\
              "    -o <output_file> : Output TIFF path\n"\
              "    -h               : Display this help\n"

// defines
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

#define BLOCK_DIM 8
#define BLOCK_SIZE 64


// Log
#define LOG_LEVEL 3
#define TRACE(__level, ...)	if ( __level >= LOG_LEVEL ) { printf(__VA_ARGS__); }
#define INFO(__message) printf("[%s: %s, l.%d] %s.\n", __FILE__, __func__, __LINE__, __message);

// Other
#define UNUSED(arg) ((void)(arg))
#define SAFE_FREE(p) do { if (p != NULL) { free(p), p = NULL; } } while (0)

#endif
