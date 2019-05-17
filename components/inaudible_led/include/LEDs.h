#ifndef LEDS_H
#define LEDS_H

extern void rmt_init(rmt_channel_t channel, gpio_num_t pinNum);
extern void LEDTask(void *ignored);

#endif
