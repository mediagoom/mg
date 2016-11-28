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
#pragma once

#define ASSERT_AREEQUAL(EXP, ACT, COMM) if(EXP != ACT) {std::wcout << L"ASSERT FAILED: " << COMM << std::endl;return 8;}

class CAutoTest
{
protected:

};

class MP4AutoTest: private CAutoTest
{

public:
	
	int TestRangeMap()
	{
		CRangeMap<int, int> r;

		r.add(12, 12); //1-12 12
		r.add(161480, 11); //13-161480 11
		r.add(161492, 3); //161480-161492 3

		int t = -1;

			ASSERT_AREEQUAL(-1, r.get(-1, t), "-1");
			ASSERT_AREEQUAL(12, t, "12");

			ASSERT_AREEQUAL(-1, r.get(11, t), "-1b");
			ASSERT_AREEQUAL(12, t, "12b");
			ASSERT_AREEQUAL(12, r.get_upper_limit(), "upper 12");

			ASSERT_AREEQUAL(0, r.get(12, t), "0");
			ASSERT_AREEQUAL(12, t, "12c");

			ASSERT_AREEQUAL(0, r.get(13, t), "0b");
			ASSERT_AREEQUAL(11, t, "11");

			ASSERT_AREEQUAL(0, r.get(161479, t), "0c");
			ASSERT_AREEQUAL(11, t, "11b");
			ASSERT_AREEQUAL(161480, r.get_upper_limit(), "upper 161480");

			ASSERT_AREEQUAL(0, r.get(161480, t), "0d");
			ASSERT_AREEQUAL(11, t, "11c");

			ASSERT_AREEQUAL(0, r.get(161490, t), "0c");
			ASSERT_AREEQUAL(3, t, "3a");
			ASSERT_AREEQUAL(161492, r.get_upper_limit(), "upper 161492");


			ASSERT_AREEQUAL(0, r.get(161492, t), "0e");
			ASSERT_AREEQUAL(3, t, "3");

			ASSERT_AREEQUAL(1, r.get(161493, t), "+1");
			ASSERT_AREEQUAL(3, t, "3");

			return 0;
			
		
	}

	int TestRangeMap2()
	{

		CRangeMap<int, int> r;
		int t = -1;

		for(int i = 0; i < 1000; i += 10)
		{
			r.add(i, i);
		}

		for(int i = 0; i < 1000; i += 10)
		{
			Cstring is;
			is += i;
			ASSERT_AREEQUAL(0, r.get(i, t), static_cast<const TCHAR*>(is));
			ASSERT_AREEQUAL(i, t,   static_cast<const TCHAR*>(is));
			if(i < 990)
			{
				ASSERT_AREEQUAL(0, r.get(i + 5, t),   static_cast<const TCHAR*>(is));
			    ASSERT_AREEQUAL(i + 10, t,   static_cast<const TCHAR*>(is));
			}
			
		}

		ASSERT_AREEQUAL(1, r.get(1000, t), L"1000");
		ASSERT_AREEQUAL(990, t, L"1000");

		return 0;

	}



};

