#ifndef _OptimalCeilingTier_h_
#define _OptimalCeilingTier_h_
/* OptimalCeilingTier.h
 *
 * Copyright (C) 2015 David Weenink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "IntensityTier.h"
#include "TableOfReal.h"
#include "Sound.h"

/********** class OptimalCeilingTier **********/

Thing_define (OptimalCeilingTier, RealTier) {
	int v_domainQuantity ()
		override { return MelderQuantity_TIME_SECONDS; }
};

OptimalCeilingTier OptimalCeilingTier_create (double tmin, double tmax);

void OptimalCeilingTier_draw (OptimalCeilingTier me, Graphics g, double tmin, double tmax,
	double ymin, double ymax, const char32 *method, int garnish);

TableOfReal OptimalCeilingTier_downto_TableOfReal (OptimalCeilingTier me);

/* End of file OptimalCeilingTier.h */
#endif
