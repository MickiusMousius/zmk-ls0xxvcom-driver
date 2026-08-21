/*
 * Copyright (c) 2025 eden ariel
 * Copyright (c) 2020 Rohit Gujarathi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sharp_ls0xx_vcom

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ls0xx, CONFIG_DISPLAY_LOG_LEVEL);

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <display/ls0xx_vcom.h>

#if DT_INST_PROP(0, serial_vcom_inversion)
#define USE_VCOM_THREAD true
#elif DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
#define USE_VCOM_THREAD true
#endif // DT_INST_PROP(0, serial_vcom_inversion)

#define LS0XX_BUS_RETURN_DELAY_TICKS 4

struct ls0xx_config {
	struct spi_dt_spec bus;
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	struct gpio_dt_spec disp_en_gpio;
#endif
#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	struct gpio_dt_spec extcomin_gpio;
#endif
	int serial_vcom_int;
};

/* This semaphore is added to prevent display refreshes from being interrupted
 by commands mid-refresh
 */
K_SEM_DEFINE(ls0xx_bus_sem, 0, 1);

static int ls0xx_blanking_off(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	const struct ls0xx_config *config = dev->config;

	int ret = gpio_pin_set_dt(&config->disp_en_gpio, 1);
#else
	LOG_WRN("Unsupported");
	int ret = -ENOTSUP;
#endif
#ifdef USE_VCOM_THREAD
	struct ls0xx_data *data = dev->data;
	const struct ls0xx_config *config_vcom = dev->config;
	data->current_vcom_interval = config_vcom->serial_vcom_int;
	k_work_reschedule(&data->vcom_toggle_work, K_MSEC(data->current_vcom_interval));
#endif
	return ret;
}

static int ls0xx_blanking_on(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	const struct ls0xx_config *config = dev->config;

	int ret = gpio_pin_set_dt(&config->disp_en_gpio, 0);
#else
	LOG_WRN("Unsupported");
	int ret = -ENOTSUP;
#endif
#ifdef USE_VCOM_THREAD
	struct ls0xx_data *data = dev->data;
	data->current_vcom_interval = 1000;
	k_work_reschedule(&data->vcom_toggle_work, K_MSEC(data->current_vcom_interval));
#endif
	return ret;
}

static int ls0xx_cmd(const struct device *dev, uint8_t *buf, uint8_t len)
{
	int ret;
	ret = 0;
	const struct ls0xx_config *config = dev->config;
	struct spi_buf cmd_buf = {.buf = buf, .len = len};
	struct spi_buf_set buf_set = {.buffers = &cmd_buf, .count = 1};
#if DT_INST_PROP(0, serial_vcom_inversion)
	struct ls0xx_data *data = dev->data;
	buf[0] |= data->vcom_state ? LS0XX_BIT_VCOM : 0;
	data->vcom_state = !data->vcom_state;
#endif // DT_INST_PROP(0, serial_vcom_inversion)
	ret = spi_write_dt(&config->bus, &buf_set);
	return ret;
}

#ifdef USE_VCOM_THREAD
/* Driver will handle VCOM toggling */
static void ls0xx_vcom_toggle_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ls0xx_data *data = CONTAINER_OF(dwork, struct ls0xx_data, vcom_toggle_work);
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct ls0xx_config *config = dev->config;

#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	gpio_pin_toggle_dt(&config->extcomin_gpio);
	k_usleep(3);
	gpio_pin_toggle_dt(&config->extcomin_gpio);
#elif DT_INST_PROP(0, serial_vcom_inversion)
	if (k_sem_take(&ls0xx_bus_sem, K_MSEC(240)) == 0) {
		uint8_t empty_cmd[2] = {0, 0};
		/* Send empty command to toggle VCOM */
		ls0xx_cmd(dev, empty_cmd, sizeof(empty_cmd));
		/* Sleep before giving semaphore based on errors in testing */
		k_sleep(K_TICKS(LS0XX_BUS_RETURN_DELAY_TICKS));
		spi_release_dt(&config->bus);
		k_sem_give(&ls0xx_bus_sem);
	} else {
		LOG_ERR("memory display semaphore not available - cmd");
	}
#endif // DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	k_work_reschedule(&data->vcom_toggle_work, K_MSEC(data->current_vcom_interval));
}
#endif // USE_VCOM_THREAD

static int ls0xx_clear(const struct device *dev)
{
	const struct ls0xx_config *config = dev->config;
	uint8_t clear_cmd[2] = {LS0XX_BIT_CLEAR, 0};
	int err;
	if (k_sem_take(&ls0xx_bus_sem, K_MSEC(240)) == 0) {
		err = ls0xx_cmd(dev, clear_cmd, sizeof(clear_cmd));
		k_sleep(K_TICKS(LS0XX_BUS_RETURN_DELAY_TICKS));
		spi_release_dt(&config->bus);
		k_sem_give(&ls0xx_bus_sem);
	} else {
		LOG_ERR("memory display semaphore not available - data");
		err = -EBUSY;
	}
	return err;
}

static inline uint8_t reverse_byte(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

static inline uint32_t ls0xx_bitreverse32(uint32_t x) {
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
	return ((x >> 16) | (x << 16));
}

static int ls0xx_update_display(const struct device *dev, uint16_t start_line, uint16_t num_lines,
				const uint8_t *data)
{
	const struct ls0xx_config *config = dev->config;
	uint8_t write_cmd[1] = {LS0XX_BIT_WRITECMD};
	uint8_t ln = start_line;
	uint8_t dummy = 27;
	struct spi_buf line_buf[3] = {
		{
			.len = sizeof(ln),
			.buf = &ln,
		},
		{
			.len = LS0XX_PANEL_WIDTH / LS0XX_PIXELS_PER_BYTE,
		},
		{
			.len = sizeof(dummy),
			.buf = &dummy,
		},
	};
	struct spi_buf_set line_set = {
		.buffers = line_buf,
		.count = ARRAY_SIZE(line_buf),
	};
	int err;

#if DT_INST_PROP(0, rotate_180)
	uint8_t row_buf[LS0XX_PANEL_WIDTH / LS0XX_PIXELS_PER_BYTE];
#endif
	int bytes_per_line = LS0XX_PANEL_WIDTH / LS0XX_PIXELS_PER_BYTE;

	LOG_DBG("Lines %d to %d", start_line, start_line + num_lines - 1);
	if (k_sem_take(&ls0xx_bus_sem, K_MSEC(240)) == 0) {
		err = ls0xx_cmd(dev, write_cmd, sizeof(write_cmd));

		for (int i = 0; i < num_lines; i++) {
			uint16_t current_logical_line = start_line + i;

#if DT_INST_PROP(0, rotate_180)
			ln = LS0XX_PANEL_HEIGHT - current_logical_line + 1;
			/* Hardware accelerated bit/byte reversal using RBIT */
			int words = bytes_per_line / 4;
			for (int j = 0; j < words; j++) {
				uint32_t val;
				memcpy(&val, data + bytes_per_line - 4 - (j * 4), 4);
				val = ls0xx_bitreverse32(val);
				memcpy(row_buf + (j * 4), &val, 4);
			}
			/* Handle remaining bytes if width is not a multiple of 32 bits */
			for (int j = words * 4; j < bytes_per_line; j++) {
				row_buf[j] = reverse_byte(data[bytes_per_line - 1 - j]);
			}
			line_buf[1].buf = row_buf;
#else
			ln = current_logical_line;
			line_buf[1].buf = (uint8_t *)data;
#endif

			err |= spi_write_dt(&config->bus, &line_set);
			data += bytes_per_line;
		}

		/* Send another trailing 8 bits for the last line
		 * These can be any bits, it does not matter
		 * just reusing the write_cmd buffer
		 */
		err |= ls0xx_cmd(dev, write_cmd, sizeof(write_cmd));
		k_sleep(K_TICKS(LS0XX_BUS_RETURN_DELAY_TICKS));
		spi_release_dt(&config->bus);
		k_sem_give(&ls0xx_bus_sem);
	} else {
		LOG_ERR("memory display semaphore not available - refresh data");
		err = -EBUSY;
	}
#ifdef USE_VCOM_THREAD
	struct ls0xx_data *driver_data = dev->data;
	k_work_reschedule(&driver_data->vcom_toggle_work, K_MSEC(driver_data->current_vcom_interval));
#endif
	return err;
}

/* Buffer width should be equal to display width */
static int ls0xx_write(const struct device *dev, const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc, const void *buf)
{
	LOG_DBG("X: %d, Y: %d, W: %d, H: %d", x, y, desc->width, desc->height);
	if (buf == NULL) {
		LOG_WRN("Display buffer is not available");
		return -EINVAL;
	}

	if (desc->width != LS0XX_PANEL_WIDTH) {
		LOG_ERR("Width not a multiple of %d", LS0XX_PANEL_WIDTH);
		return -EINVAL;
	}

	if (desc->pitch != desc->width) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if ((y + desc->height) > LS0XX_PANEL_HEIGHT) {
		LOG_ERR("Buffer out of bounds (height)");
		return -EINVAL;
	}

	if (x != 0) {
		LOG_ERR("X-coordinate has to be 0");
		return -EINVAL;
	}

	/* Adding 1 since line numbering on the display starts with 1 */
	return ls0xx_update_display(dev, y + 1, desc->height, buf);
}

static int ls0xx_read(const struct device *dev, const uint16_t x, const uint16_t y,
		      const struct display_buffer_descriptor *desc, void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *ls0xx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int ls0xx_set_brightness(const struct device *dev, const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int ls0xx_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void ls0xx_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = LS0XX_PANEL_WIDTH;
	caps->y_resolution = LS0XX_PANEL_HEIGHT;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->screen_info = SCREEN_INFO_X_ALIGNMENT_WIDTH;
}

static int ls0xx_set_orientation(const struct device *dev,
				 const enum display_orientation orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int ls0xx_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO01) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int ls0xx_init(const struct device *dev)
{
	const struct ls0xx_config *config = dev->config;
	struct ls0xx_data *data = dev->data;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	if (!gpio_is_ready_dt(&config->disp_en_gpio)) {
		LOG_ERR("DISP port device not ready");
		return -ENODEV;
	}
	LOG_INF("Configuring DISP pin to OUTPUT_HIGH");
	gpio_pin_configure_dt(&config->disp_en_gpio, GPIO_OUTPUT_HIGH);
#endif

#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	if (!gpio_is_ready_dt(&config->extcomin_gpio)) {
		LOG_ERR("EXTCOMIN port device not ready");
		return -ENODEV;
	}
	LOG_INF("Configuring EXTCOMIN pin");
	gpio_pin_configure_dt(&config->extcomin_gpio, GPIO_OUTPUT_LOW);
#endif /* DT_INST_NODE_HAS_PROP(0, extcomin_gpios) */
	data->vcom_state = false;
	/* Give the semaphore to allow bus access */
	k_sem_give(&ls0xx_bus_sem);
#ifdef USE_VCOM_THREAD
	/* Initialize dynamic VCOM toggling */
	k_work_init_delayable(&data->vcom_toggle_work, ls0xx_vcom_toggle_handler);
	data->current_vcom_interval = config->serial_vcom_int;
	k_work_reschedule(&data->vcom_toggle_work, K_MSEC(data->current_vcom_interval));
#endif // USE_VCOM_THREAD

	/* Clear display else it shows random data */
	return ls0xx_clear(dev);
}

static struct ls0xx_data ls0xx_data;

static const struct ls0xx_config ls0xx_config = {
	.bus = SPI_DT_SPEC_INST_GET(0,
				    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_LSB |
					    SPI_CS_ACTIVE_HIGH | SPI_HOLD_ON_CS | SPI_LOCK_ON,
				    0),
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	.disp_en_gpio = GPIO_DT_SPEC_INST_GET(0, disp_en_gpios),
#endif
#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	.extcomin_gpio = GPIO_DT_SPEC_INST_GET(0, extcomin_gpios),
#endif
#if DT_INST_PROP(0, serial_vcom_inversion)
	.serial_vcom_int = DT_INST_PROP_OR(0, serial_vcom_interval, LS0XX_MAX_VCOM_MSEC)
#elif DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	.serial_vcom_int = 1000 / DT_INST_PROP(0, extcomin_frequency)
#endif
};

static struct display_driver_api ls0xx_driver_api = {
	.blanking_on = ls0xx_blanking_on,
	.blanking_off = ls0xx_blanking_off,
	.write = ls0xx_write,
	.read = ls0xx_read,
	.get_framebuffer = ls0xx_get_framebuffer,
	.set_brightness = ls0xx_set_brightness,
	.set_contrast = ls0xx_set_contrast,
	.get_capabilities = ls0xx_get_capabilities,
	.set_pixel_format = ls0xx_set_pixel_format,
	.set_orientation = ls0xx_set_orientation,
};

DEVICE_DT_INST_DEFINE(0, ls0xx_init, NULL, &ls0xx_data, &ls0xx_config, POST_KERNEL,
		      CONFIG_DISPLAY_INIT_PRIORITY, &ls0xx_driver_api);
