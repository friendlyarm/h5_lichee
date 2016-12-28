/*
 * Copyright (C) 2013 Allwinnertech
 * Heming <lvheming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/sys_config.h>


struct gpio_led gpio_leds[] = {
{
	.name					= "green_led",
	.default_trigger		= "default-on",
	.gpio					= (11*32+10),		// PL10
	.retain_state_suspended = 1,
	}, {
	.name					= "blue_led",
	.default_trigger		= "heartbeat",
	.gpio					= (10),				// PA10
	.retain_state_suspended = 1,
	},
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device sunxi_leds = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_led_info,
	}
};

static int __init sunxi_leds_init(void)
{
	int ret = 0;

	ret = platform_device_register(&sunxi_leds);
	return ret;
}

static void __exit sunxi_leds_exit(void)
{
    platform_device_unregister(&sunxi_leds);
}

module_init(sunxi_leds_init);
module_exit(sunxi_leds_exit);

MODULE_DESCRIPTION("sunxi leds driver");
MODULE_AUTHOR("Heming");
MODULE_LICENSE("GPL");



