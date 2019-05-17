
#ifndef TYPES_H_
#define TYPES_H_

typedef struct
{
	uint8_t green;
	uint8_t red;
	uint8_t blue;
} pixel;

typedef struct
{
	float hue;
	float saturation;
	float value;
} HSVpixel;

typedef struct
{
	size_t len;
	uint8_t *data;
} AudioPacket;

typedef struct
{
	float *l;
	float *r;
} stereoBuf;

/* event for handler "bt_av_hdl_stack_up */
enum {
    BT_APP_EVT_STACK_UP = 0,
};

#endif /* TYPES_H_ */
