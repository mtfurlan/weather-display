#ifndef INAUDIBLE_LED_H_
#define INAUDIBLE_LED_H_

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

extern void led_init(rmt_channel_t channel, gpio_num_t pinNum);

esp_err_t led_write(rmt_channel_t channel, const pixel_t *src, size_t num_pixels);

#endif /* INAUDIBLE_LED_H_ */

