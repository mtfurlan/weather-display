
#ifndef TYPES_H_
#define TYPES_H_

typedef struct
{
	uint8_t green;
	uint8_t red;
	uint8_t blue;
} pixel_t;

typedef struct
{
	float hue;
	float saturation;
	float value;
} HSVpixel_t;

#endif /* TYPES_H_ */
