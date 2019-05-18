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
