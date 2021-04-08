#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "esp_log.h"
#include "mh-z19.h"

static const char *TAG = "mhz19";

esp_err_t mhz19_init_uart()
{
	uart_config_t uart_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
		.source_clk = UART_SCLK_REF_TICK
	};

	esp_err_t ret;
	if ((ret = uart_param_config(CONFIG_MHZ19_UART_NUM, &uart_config)) != ESP_OK)
	{
		return ret;
	}

	if ((ret = uart_set_pin(CONFIG_MHZ19_UART_NUM, CONFIG_MHZ19_UART_TXD_PIN, CONFIG_MHZ19_UART_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) != ESP_OK)
	{
		return ret;
	}
	//Install UART driver( We don't need an event queue here)

	ret = uart_driver_install(CONFIG_MHZ19_UART_NUM, 128 * 2, 0, 0, NULL, 0);
	return ret;
}

static int mhz19_check(uint8_t *packet)
{
	unsigned char checksum = 0;
	for (int i = 1; i < 8; i++)
	{
		checksum += packet[i];
	}
	checksum = 0xff - checksum;
	checksum += 1;
	if (packet[8] != checksum)
	{
		packet[8] = checksum;
		return 0;
	}
	return 1;
}

typedef enum
{
	MH_Z19_READ_GAS_CONCENTRATION = 0x86,
	MH_Z19_CALIBRATE_ZERO_POINT = 0x87,
	MH_Z19_CALIBRATE_SPAN_POINT = 0x88,
	MH_Z19_ABC_LOGIC = 0x79,
	MH_Z19_SENSOR_DETECTION_RANGE = 0x99
} mhz19_cmd_t;

static int mhz19_cmd(mhz19_cmd_t cmd, uint32_t val)
{
	uint8_t packet[9] = {0xff, 0x01, cmd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	switch (cmd)
	{
	case MH_Z19_SENSOR_DETECTION_RANGE:
	case MH_Z19_CALIBRATE_SPAN_POINT:
		packet[3] = val >> 8;
		packet[4] = val & 255;
		break;
	case MH_Z19_ABC_LOGIC:
		packet[3] = val ? 0xa0 : 0x00;
		break;
	case MH_Z19_CALIBRATE_ZERO_POINT:
	case MH_Z19_READ_GAS_CONCENTRATION:
		break;
	}
	mhz19_check(packet);

	return uart_write_bytes(CONFIG_MHZ19_UART_NUM, (const char *)packet, sizeof(packet));
}

int mhz19_get_co2()
{
	int len = mhz19_cmd(MH_Z19_READ_GAS_CONCENTRATION, 0);

	if (len == 9)
	{
		uint8_t data[32];
		len = uart_read_bytes(CONFIG_MHZ19_UART_NUM, data, sizeof(data), 100 / portTICK_RATE_MS);
		if (len == 9 && mhz19_check(data))
		{
			if (data[0] == 0xff && data[1] == 0x86)
			{
				int co2val = (data[2] << 8) | data[3];
				int t = data[4] - 40;
				int s = data[5];
				int u = (data[6] << 8) | data[7];
				ESP_LOGD(TAG, "CO2: %d T:%d S:%d U:%d", co2val, t, s, u);

				return co2val;
			}
		}
	}

	return 0;
}

esp_err_t mhz19_calibrate()
{
	mhz19_cmd(MH_Z19_ABC_LOGIC, 0x0);
	return mhz19_cmd(MH_Z19_CALIBRATE_ZERO_POINT, 0) == 9 ? ESP_OK : ESP_FAIL;
}

void mhz19_init()
{
	mhz19_init_uart();

	// set ABC logic on (0xa0) / off (0x00)
	mhz19_cmd(MH_Z19_ABC_LOGIC, 0xa0);
}
