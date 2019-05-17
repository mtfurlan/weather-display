#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "include/defines.h"
#include "include/types.h"
#include "include/analysis.h"


static const char *TAG = "patterngen.c";
size_t bins40[] ={ 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 16, 19, 23, 27, 32, 38, 45, 52, 60, 69, 79, 90, 
	102, 115, 129, 144, 161, 179, 198, 218, 240, 263, 288, 314, 342, 371, 402, 435, 470, 512};
/*size_t bins60[] = { 1, 2, 4, 6, 8, 11, 14, 17, 20, 24, 28, 32, 36, 41, 46, 51, 56, 61, 67, 73, 79, 85,
					91, 98, 105, 112, 119, 126, 133, 141, 149, 157, 165, 173, 181, 189, 198, 207, 216,
					225, 234, 243, 252, 262, 272, 282, 292, 302, 312, 322, 332, 343, 354, 365, 376, 387,
					398, 409, 420, 432, 444 };
*/
/*
size_t  bins60[] = { 1, 2, 4, 7, 11, 16, 22, 29, 37, 46, 56, 67, 79, 92, 106, 121, 137, 154,
					  172, 191, 211, 232, 254, 277, 301, 326, 352, 379, 407, 436, 466, 497, 529,
					  562, 596, 631, 667, 704, 742, 781, 821, 862, 904, 947, 991, 1036, 1082, 1129,
					  1177, 1226, 1276, 1327, 1379, 1432, 1486, 1541, 1597, 1654, 1712, 1771, 1831 };
*/
size_t bins60[] = { 1, 2, 4, 6, 9, 13, 17, 22, 28, 34, 41, 48, 56, 64, 73, 82, 92, 102, 113, 124, 136,
148, 161, 174, 188, 202, 217, 232, 247, 263, 279, 296, 313, 331, 349, 368, 387, 406, 426, 446, 512, 533,
555, 577, 599, 622, 645, 669, 693, 718, 743, 768, 794, 820, 847, 874, 901, 929, 957, 986, 1015}; 

//size_t bins16[] = {1, 2, 3, 6, 11, 19, 31, 47, 68, 95, 128, 168, 216, 272, 337, 412, 512};
size_t bins16[] = {1, 2, 6, 16, 35, 66, 111, 174, 258, 366, 501, 666, 865, 1101, 1377, 1697, 2048};
pixel *rainbow(float *);


float valueize(float raw, float min, float max)
{

	float dx = max - min;
	const float dy = 1.0f-0.0125f;
	float m;
	float y;
	if (dx < 0.00001f) return min > 0.4 ? 0.4 : min;
	else m = dy/dx, y = 1.0f;
	float b = y - m*max;

	return m * raw + b;
}


bool polar0(float *buf, size_t len, bool mags, bool phs, float **magnitudes, float **phases)
{
	static float mg[FFTLEN/2];
	static float ph[FFTLEN/2];

	if (len > FFTLEN)
	{
		*magnitudes = *phases = NULL;
		return false;
	}
	mg[0] = buf[0];
	for (int i = 1; i < len/2; i++)
	{
		if (mags) mg[i] = hypotf(buf[i], buf[len-i]);
		if (phases) ph[i] = atan2f(buf[len-i], buf[i]);
	}
	*magnitudes = mg;
	*phases = ph;
	return true;
}


bool bin(float *buf, size_t len, bool reverse)
{
	if (!buf) return false;
	if (len <= 0) return false;

	int max = (len < 41) ? len : 41;

	for (int i = 0; i < max-1 ; i++)
	{
		float tot = 0.0f;
		int count = 0;

		for (int j = bins40[i] ; j < max && j < bins40[i+1] ; j++)
		{
			count++;
			if (reverse) tot += buf[len-j];
			else tot += buf[j];
		}
		buf[i] = tot/count;
	}
	return true;
}


bool average(float *buf, size_t len)
{
	float retval = 0.0f;

	for (int i = 0; i < len; i++)
	{
		retval += buf[i];
	}
	return retval/len;
}


bool bin60(float *inbuf, float *outbuf)
{
	for (int i = 0; i < 60; i++)
	{
		float tot = 0.0f;
		int count = 0;
		for (int j = bins60[i]; j < bins60[i+1]; j++)
		{
			count++;
			tot += inbuf[j];
		}
		outbuf[i] = tot/count;
ESP_LOGE("bin60()", "output bin %d is %f", i, outbuf[i]);
	}
	return true;
}


void square(const float *in, float *out, size_t len)
{
	if (!(in&&out)) return;
	for (int i = 0; i < len; i++)
	{
		out[i] = powf(in[i], 2.0f);
ESP_LOGE("square()", "out[%d] is %f", i, out[i]);
	}
	return;
}


void rtOfSquare(const float *in, float *out, size_t len)
{
	if (!(in && out)) return;
	for (int i = 0; i < len; i++)
	{
		out[i] = powf(in[i] * in[i],0.5);
	}
	return;
}


void minmax(const float *buf, size_t len, float *min, float *max)
{
	if (!buf) return;
	if (!(min || max)) return; //must have at least one or the other, or both
	if (len <= 0) return; //must have at least one value

	float lmax;
	float lmin;

	lmax = lmin = buf[0];

	for (int i = 1; i < len; i++)
	{
		if (buf[i] < lmin) lmin = buf[i];
		if (buf[i] > lmax) lmax = buf[i];
	}
	if (min) *min = lmin;
	if (max) *max = lmax;
	return;
}


bool subtract(float *a, float *b, float *dest, size_t len)
{
	if (!(a && b && dest && len > 0)) return false;
	for (int i = 0; i < len; i++)
	{
		dest[i] = a[i]-b[i];
	}
	return true;
}


pixel *diff(float *left, float *right)
{

	subtract(left, right, left, NUMLEDS);
	return rainbow(left);
}

bool meanAndStdDev(float *buf, size_t len, float *mean, float *stddev)
{
	static const char *TAG = "meanAndStdDev()";
	if (!buf) return false;

	float A = 0.0f;
	float Q = 0.0f;

	for (int k = 1; k <= len; k++)  //note, k = 1,2,3,...,N
	{
		float temp = buf[k-1] - A;
		A = A + (temp / k);
		Q = Q + ((((float)k-1) / (float)k) * temp * temp);
ESP_LOGE(TAG, "k = %d of %d, buf[k]: %f, temp: %f, A: %f, Q: %f", k, len, buf[k], temp, A, Q);
	}

	if (mean) *mean = A;
	if (stddev) *stddev = sqrtf(Q/(float)len);

	return true;
}

float standardize(float x, float mean, float stddev)
{
	if (stddev != 0) return ((x - mean) / stddev);
	return 0.5f;
}

pixel *rainbow(float *buf)
{
static const char *TAG = "rainbow()";
TickType_t start, end;
	static HSVpixel rainbow16[] = {{0.0f,1.0f,0.0f},{15.0f,1.0f,0.0f},{30.0f,1.0f,0.0f},{37.5f,1.0f,0.0f},{45.0f,1.0f,0.0f},
								   {60.0f,1.0f,0.0f},{75.0f,1.0f,0.0f}, {90.0f,1.0f,0.0f},{120.0f,1.0f,0.0f},
								   {150.0f,1.0f,0.0f},{180.0f,1.0f,0.0f},{210.0f,1.0f,0.0f},{240.0f,1.0f,0.0f},
								   {270.0f,1.0f,0.0f},{300.0f,1.0f,0.0f},{330.0f,1.0f,0.0f},};
	static float eq[] = {3, 1, 1, 1, 1, 1, 1.1, 1.1, 1.2, 1.3, 1.4, 1.6, 1.8, 2.2, 2.6, 3.0};
	pixel *LEDs = malloc(NUMLEDS*sizeof(pixel));
	bin60(buf, buf);
	
	float mean;
	float stddev;
	start = xTaskGetTickCount();
	meanAndStdDev(buf, NUMLEDS, &mean, &stddev);
	end = xTaskGetTickCount();
ESP_LOGE(TAG, "meanAndStdDev took %d ticks", end - start);
ESP_LOGE(TAG, "mean is %f, stddev is %f", mean, stddev);
	for (int i = 0; i < NUMLEDS; i++)
	{
		float temp  = standardize(buf[i], mean, stddev);
ESP_LOGE(TAG, "standardize %f returned %f", buf[i], temp);
		buf[i] = temp;
	}

	float max;
	float min;
	minmax(buf, NUMLEDS, &min, &max);
ESP_LOGE(TAG, "min is %f, max is %f", min, max);

	for (int i = 0; i < NUMLEDS; i++)
	{
		HSVpixel temp = {0.0f,0.0f,0.0f};
		temp.hue = 360.0f / (float) NUMLEDS * (float) i;
		temp.saturation  = 1.0;
		float level = buf[i];//valueize(buf[i], min, max);
		if (level < 0.01f) level = 0.01f;
ESP_LOGE(TAG, "level is %f", level);
		
		temp.value = level;
	start = xTaskGetTickCount();
		HSVtoRGB(&temp, &LEDs[i]);
	end = xTaskGetTickCount();
ESP_LOGE(TAG, "HSVtoRGB took %d ticks", end - start);
	}
	return LEDs;
}


pixel *VUMeter(float *buf)
{
static const char *TAG = "VUMeter()";
//	static pixel VUColors[NUMLEDS] = {{32,0,0},{48,0,0},{64,0,0},{72,0,0},{72,0,0},{72,0,0},{72,0,0},{72,0,0},{72,16,0},
//										{72,24,0},{72,32,0},{32,32,0},{32,72,0},{16,72,0},{0,72,0},{0,72,0}}; // VU Meter
	static HSVpixel VUColors[NUMLEDS] = {{ 120.0f, 1.0f, 0.1f}, {120.0f, 1.0f, 0.125f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.375f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {125.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}, {120.0f, 1.0f, 0.25f}}; 
	static int max = 0;
	static pixel black = {0,0,0};
	int power = 0;
	pixel *vu = NULL;
	
	if (!buf) return NULL;
	vu = malloc(NUMLEDS*sizeof(pixel));

	if (!vu) return NULL;

ESP_LOGE(TAG, "buf[0] is %f", buf[0]);
	//power = (int)(log10f(buf[0]+1)*(float)NUMLEDS);
	float m,b,dx,dy;
	dx = 0.54f - 0.46f;
	dy = 1;
	m = dy/dx;
	b = 0.54-m;
	power = m*buf[0]+b;
ESP_LOGE(TAG, "power is %d", power);
	if (power > max) max = power;
	else max -= 1;
	if (max > NUMLEDS - 1) max = NUMLEDS-1;
	if (max < 0) max = 0;

	for (int i = 0; i < NUMLEDS; i++)
	{
		if (i < power ) HSVtoRGB(&VUColors[i], &vu[i]);
		else vu[i] = black;
	}
ESP_LOGW(TAG, "\tsetting max (%d)", max);
	HSVtoRGB( &VUColors[max], &vu[max]);
ESP_LOGW(TAG, "\treturning");
	return vu;
}


/*pixel *HSV(float *buf)
{
	static HSVpixel[NUMLEDS] = {};

}*/

bool reverselast(float *buf, size_t len)
{
	if (!buf) return false;
	if (len%2 != 0) return false;

	int half = len/2;
	float *p = buf + half;

	for (int i = 1; i < half; i++)
	{
		float temp = p[i];
		p[i] = p[half-i];
		p[half-i] = temp;
	}
	return true;
}


pixel *phaseshift(float *buf)
{
	static const char *TAG = "phaseshift()";

	HSVpixel hsv = {0.0f,0.0f,0.0f};
	pixel *pixels = NULL;

	if (!(pixels = malloc(NUMLEDS*sizeof(pixel))))  return NULL;

	polar(buf, buf, FFTLEN);
	reverselast(buf, FFTLEN);
	bin60(buf, buf);
	bin60(buf+(FFTLEN/2), buf+(FFTLEN/2));
	square(buf, buf, NUMLEDS);
	float min,max;
	minmax(buf, NUMLEDS, &min, &max);

	for (int i = 0; i < NUMLEDS; i++)
	{
		hsv.hue = 360.0f * buf[FFTLEN/2+i];
ESP_LOGE(TAG, "phase is %f, hue is %f", buf[FFTLEN/2+i], hsv.hue);
		hsv.saturation = 1.0f;
ESP_LOGE(TAG, "magnitue is %f", buf[i]);
		hsv.value = valueize(buf[i], min, max);
ESP_LOGE(TAG, "value is %f", hsv.value);

		HSVtoRGB(&hsv, &pixels[i]);
	}
	return pixels;
}

