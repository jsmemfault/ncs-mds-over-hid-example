/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MDS_HID_H_
#define MDS_HID_H_

#include <zephyr/device.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the MDS HID device
 *
 * @param hid_dev HID device instance
 * @return 0 on success, negative errno on failure
 */
int mds_hid_init(const struct device *hid_dev);

/**
 * @brief Check if the HID interface is ready
 *
 * @return true if ready, false otherwise
 */
bool mds_hid_is_ready(void);

/**
 * @brief Check if streaming is enabled
 *
 * @return true if streaming enabled, false otherwise
 */
bool mds_hid_is_streaming(void);

/**
 * @brief Send a Memfault diagnostic data chunk
 *
 * @param hid_dev HID device instance
 * @return Positive number of bytes sent, 0 if no data available, negative errno on error
 */
int mds_hid_send_chunk(const struct device *hid_dev);

/**
 * @brief Get the HID report descriptor
 *
 * @param size Output parameter for descriptor size
 * @return Pointer to the report descriptor
 */
const uint8_t *mds_hid_get_report_desc(size_t *size);

/**
 * @brief Get the HID device operations structure
 *
 * @return Pointer to the hid_device_ops structure
 */
struct hid_device_ops *mds_hid_get_ops(void);

#endif /* MDS_HID_H_ */
