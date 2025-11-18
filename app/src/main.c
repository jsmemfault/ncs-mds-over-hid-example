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

#include "mds_hid.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

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
			/* Small delay between chunks */
			k_sleep(K_MSEC(10));
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
