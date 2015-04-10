/*
 * Copyright (c) 2011 Bosch Sensortec GmbH
 * Copyright (c) 2011 Unixphere
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef LINUX_BMA150_MODULE_H
#define LINUX_BMA150_MODULE_H

/**
 * struct bma150_platform_data - data to set up bma150 driver
 *
 * @setup: optional callback to activate the driver.
 * @teardown: optional callback to invalidate the driver.
 *
**/

struct bma150_platform_data {
	int (*setup)(struct device *);
	void (*teardown)(struct device *);
	int (*power_on)(void);
	void (*power_off)(void);
};

#endif /* LINUX_BMA150_MODULE_H */
