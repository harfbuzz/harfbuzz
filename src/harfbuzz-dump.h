/* harfbuzz-dump.h: Dump OpenType layout tables
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef HARFBUZZ_DUMP_H
#define HARFBUZZ_DUMP_H

#include <stdio.h>
#include "harfbuzz-gsub.h"
#include "harfbuzz-gpos.h"

HB_BEGIN_HEADER

void HB_Dump_GSUB_Table (HB_GSUB gsub, FILE *stream);
void HB_Dump_GPOS_Table (HB_GPOS gpos, FILE *stream);

HB_END_HEADER

#endif /* HARFBUZZ_DUMP_H */
