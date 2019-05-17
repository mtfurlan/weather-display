#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "include/defines.h"

static const uint8_t ledgamma[];
static void IRAM_ATTR byte2Item32s
	(
		uint8_t *src,
		rmt_item32_t *dest,
		size_t srcBytes,
		size_t maxItems,
		size_t *bytesUsed,
		size_t *itemsCopied
	)
{
	if (!(src && dest && bytesUsed && itemsCopied))
	{
		if (bytesUsed) *bytesUsed = 0;
		if (itemsCopied) *itemsCopied = 0;
		return;
	}

	static const rmt_item32_t zero = {{{7,1,18,0}}};
	static const rmt_item32_t one = {{{18,1,7,0}}};

	size_t bytes = 0;
	size_t items = 0;
	while (bytes < srcBytes && (items + 8) <= maxItems)
	{
		int byte = *src++;
//		byte = ledgamma[byte];
		for (int i = 0 ; i < 8 ; i++)
		{
			if ( byte & (1<< (7-i) ) ) *dest++ = one;
			else			 *dest++ = zero;

			items++;
		}
		bytes++;
	}

	*bytesUsed = bytes;
	*itemsCopied = items;
	return;
}

void rmt_init(rmt_channel_t chan, gpio_num_t pinNum)
{
static const char *TAG = "rmt_init";
ESP_LOGV(TAG,"rmt_init(%d) called", pinNum);
	rmt_config_t conf;
	conf.rmt_mode = RMT_MODE_TX;
	conf.channel = chan;
	conf.clk_div = 4;
	conf.gpio_num = pinNum;
	conf.mem_block_num = 1;
	conf.tx_config.loop_en = false;
	conf.tx_config.carrier_freq_hz = 0;
	conf.tx_config.carrier_duty_percent = 0;
	conf.tx_config.carrier_level = 0;
	conf.tx_config.carrier_en = false;
	conf.tx_config.idle_level = 0;
	conf.tx_config.idle_output_en = false;

	ESP_ERROR_CHECK(rmt_config(&conf));
	ESP_ERROR_CHECK(rmt_driver_install(conf.channel, 0, 0));
	ESP_ERROR_CHECK(rmt_translator_init(conf.channel, (sample_to_rmt_t)byte2Item32s));
	return;
}

//void LED_write_sample(int channel, uint8_t *buf, size_t len)
//{
//	if (NEOPIXEL) rmt_write_sample(channel, buf, len, false);
//	else if (DOTSTAR) dotstar_write_pixels(channel, buf, len);
//}
//
//void LEDTask(void *ignored)
//{
//	extern xQueueHandle LEDDataBufferQueue;
//	extern xQueueHandle LEDEmptyBufferQueue;
//	void *buf;
//
//	for (;;)
//	{
//		if( xQueueReceive(rainbowDataBufferQueue, &buf, portMAX_DELAY) )
//		{
//			LED_write_sample(CHAN1, buf, BUFSIZE);
//			xQueueSendToBack(LEDEmptyBufferQueue, &buf, 0);
//		}
//		if( xQueueReceive(LEDDataBufferQueue, &buf, portMAX_DELAY) )
//		{
//			LED_write_sample(CHAN2, buf, BUFSIZE);
//			xQueueSendToBack(LEDEmptyBufferQueue, &buf, 0);
//		}
//		if( xQueueReceive(LEDDataBufferQueue, &buf, portMAX_DELAY) )
//		{
//			LED_write_sample(CHAN3, buf, BUFSIZE);
//			xQueueSendToBack(LEDEmptyBufferQueue, &buf, 0);
//		}
//		if( xQueueReceive(LEDDataBufferQueue, &buf, portMAX_DELAY) )
//		{
//			LED_write_sample(CHAN4, buf, BUFSIZE);
//			xQueueSendToBack(LEDEmptyBufferQueue, &buf, 0);
//		}
//	}
//	vTaskDelete(NULL);
//	return;
//}
//
//static const uint8_t ledgamma[] = {
//    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
//    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
//    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
//    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
//    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
//   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
//   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
//   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
//   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
//   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
//   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
//   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
//  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
//  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
//  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
//  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
