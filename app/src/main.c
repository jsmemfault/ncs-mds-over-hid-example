/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include <memfault/components.h>

#include "mds_hid.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button_cb_data;

static void trigger_random_fault(void)
{
	uint32_t fault_type = sys_rand32_get() % 5;

	LOG_INF("Triggering fault type %d", fault_type);

	switch (fault_type) {
	case 0:
		LOG_INF("Triggering assert...");
		MEMFAULT_ASSERT(0);
		break;
	case 1:
		LOG_INF("Triggering NULL pointer dereference...");
		{
			volatile uint32_t *p = NULL;
			*p = 0xDEADBEEF;
		}
		break;
	case 2:
		LOG_INF("Triggering divide by zero...");
		{
			volatile int a = 1;
			volatile int b = 0;
			volatile int c = a / b;
			(void)c;
		}
		break;
	case 3:
		LOG_INF("Triggering unaligned access...");
		{
			volatile uint8_t buf[8] = {0};
			volatile uint32_t *p = (volatile uint32_t *)&buf[1];
			*p = 0xDEADBEEF;
		}
		break;
	case 4:
		LOG_INF("Triggering software watchdog...");
		memfault_software_watchdog_enable();
		while (1) {
			/* Spin forever without feeding watchdog */
		}
		break;
	}
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_WRN("Button pressed - triggering random fault!");
	trigger_random_fault();
}

int main(void)
{
	struct usbd_context *sample_usbd;
	const struct device *hid_dev;
	int ret;

	/* Initialize LED */
	if (!gpio_is_ready_dt(&led0)) {
		LOG_ERR("LED device %s is not ready", led0.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return 0;
	}

	/* Initialize button */
	if (!gpio_is_ready_dt(&button0)) {
		LOG_ERR("Button device %s is not ready", button0.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button0, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure button pin, error: %d", ret);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure button interrupt, error: %d", ret);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button0.pin));
	gpio_add_callback(button0.port, &button_cb_data);
	LOG_INF("Button configured - press to trigger random fault");

	/* Get HID device */
	hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
	if (!device_is_ready(hid_dev)) {
		LOG_ERR("HID Device is not ready");
		return -EIO;
	}

	/* Initialize MDS HID interface */
	ret = mds_hid_init(hid_dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize MDS HID, %d", ret);
		return ret;
	}

	/* Initialize and enable USB device */
	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to enable device support");
		return ret;
	}

	LOG_INF("MDS over HID device enabled");

	/* Main loop: Send diagnostic chunks when streaming is enabled */
	while (true) {
		if (!mds_hid_is_ready()) {
			LOG_DBG("USB HID device is not ready");
			k_sleep(K_MSEC(1000));
			continue;
		}

		if (!mds_hid_is_streaming()) {
			/* Streaming disabled, just wait */
			k_sleep(K_MSEC(100));
			continue;
		}

		/* Try to send a chunk */
		ret = mds_hid_send_chunk(hid_dev);
		if (ret > 0) {
			/* Chunk sent successfully, toggle LED */
			(void)gpio_pin_toggle(led0.port, led0.pin);
			/* Delay between chunks - gateway needs time for HTTP upload */
			k_sleep(K_MSEC(100));
		} else if (ret == 0) {
			/* No data available, check again later */
			k_sleep(K_MSEC(100));
		} else {
			/* Error sending chunk */
			LOG_ERR("Error sending chunk: %d", ret);
			k_sleep(K_MSEC(100));
		}
	}

	return 0;
}
