// https://github.com/jwillia3/wse2/blob/2c95fc58fd5fc63ef24429af7acee8720abb1d03/colour.h
#define _USE_MATH_DEFINES
#include <math.h>
#include "inaudible_led.h"

typedef union {
    struct { double x,y,z; };
    struct { double l;
                union {
                    struct { double a,b; };
                    struct { double u,v; };
                    struct { double c,h; };
                };
            };
} colour_t;

typedef struct {
    double l;
    double c;
    double h;
} lch_t;

// COLOUR SPACE
// CIELCH -> L* = luminance: [0, 100]; C = chroma: [0, 100], h = hue [0, 360)
// CIELUV -> L* = luminance: [0, 100]; u*, v*: [?, ?]
// CIE XYZ -> tristimulus values
// sRGB -> gamma corrected sRGB [0, 1]

static colour_t D65 = {{95.047, 100.0, 108.883}};
static double kappa = 903.3;
static double epsilon = 0.008856;


pixel_t lch2rgb(colour_t lch);
