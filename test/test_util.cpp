/*****************************************************************************
 * Copyright (C) 2016 mg project
 *
 * Authors: MediaGoom <admin@mediagoom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE or NONINFRINGEMENT. 
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * For more information, contact us at info@mediagoom.com.
 *****************************************************************************/
#include <mgcore.h>

int64_t compare_file(const TCHAR* f1, const TCHAR* f2, int64_t size)
{
	FILE * fh1, * fh2;

	fh1 = ::fopen(f1, "rb");
	fh2 = ::fopen(f2, "rb");

	int v1, v2;

	if (NULL == fh1)
	{
		MGCHECK(-1);
	}

	if (NULL == fh2)
	{
		MGCHECK(-1);
	}

	for (int64_t i = 0; i < size; i++)
	{
		v1 = fgetc(fh1);
		v2 = fgetc(fh2);

		if ( (v1 != v2) || v1 == EOF)
		{
			::fclose(fh1);
			::fclose(fh2);

			return i;
		}
	}

	::fclose(fh1);
	::fclose(fh2);

	return 0;
}

