/* lutil_meter.h - progress meters */
/* $ReOpenLDAP$ */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright (c) 2009 by Emily Backes, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Emily Backes for inclusion
 * in OpenLDAP software.
 */

#ifndef _LUTIL_METER_H
#define _LUTIL_METER_H

#include "portable.h"

#include <limits.h>
#include <stdio.h>
#include <sys/types.h>

#include <ac/stdlib.h>
#include <ac/time.h>

typedef struct {
	int (*display_open) (void **datap);
	int (*display_update) (void **datap, double frac, time_t remaining_time, time_t elapsed, double byte_rate);
	int (*display_close) (void **datap);
} lutil_meter_display_t;

typedef struct {
	int (*estimator_open) (void **datap);
	int (*estimator_update) (void **datap, double start, double frac, time_t *remaining_time);
	int (*estimator_close) (void **datap);
} lutil_meter_estimator_t;

typedef struct {
	const lutil_meter_display_t *display;
	void * display_data;
	const lutil_meter_estimator_t *estimator;
	void * estimator_data;
	double start_time;
	double last_update;
	size_t goal_value;
	size_t last_position;
} lutil_meter_t;

extern const lutil_meter_display_t lutil_meter_text_display;
extern const lutil_meter_estimator_t lutil_meter_linear_estimator;

extern int lutil_meter_open (
	lutil_meter_t *lutil_meter,
	const lutil_meter_display_t *display,
	const lutil_meter_estimator_t *estimator,
	size_t goal_value);
extern int lutil_meter_update (
	lutil_meter_t *lutil_meter,
	size_t position,
	int force);
extern int lutil_meter_close (lutil_meter_t *lutil_meter);

#endif /* _LUTIL_METER_H */
