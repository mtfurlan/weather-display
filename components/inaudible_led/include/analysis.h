#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "types.h"

extern void AnalysisTask(void *ignored);

extern void HSVtoRGB(const HSVpixel *, pixel *);
extern void RGBtoHSV(const pixel *, HSVpixel *);

extern bool polar(float *, float *, size_t);

#endif
