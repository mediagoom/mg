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

#include <iostream>
#include <mgmedia.h>

using namespace MGCORE;

#include "test_thread.h"
#include "test_bitstream.h"


#include "test_resource.h"
#include "test_uv.h"

#include "test_mp4_parse.h"

#include "test_base.h"


template class ::mg::core::CstringT<char>;

uint64_t get_repeat()
{
	Cstring t = get_env_variable(_T("MGTEST_REPEAT_TIME"));

	if (0 == t.size())
		return 1;

	else
		return t;
}






int main(int argc, char *argv[])
{

	MGCOLOR orig = get_current_color();

	std::cout << GREEN << "==Start Test==" << orig << std::endl;




	try {

		::mg::core::Cstring srcdir = get_env_variable(_T("srcdir"));
		
				
		std::cout << srcdir << std::endl;

		uint64_t repeat = get_repeat();

		STARTCASE;

		for (uint64_t x = 0; x < repeat; x++)
		{

			if(0 < x)
				std::cout << "==>>> [" << x << "] <<<==" << std::endl;


			BEGINSECTION(BITSTREAM)
				TESTCASE(test_fixed_memory);
				TESTCASE(test_bitset);
			ENDSECTION

			BEGINSECTION(THREAD)
				TESTCASE(test_signal_event);
				TESTCASE(test_thread_locking);
				TESTCASE(test_thread_signaling);
				//TESTCASE(test_uvloop);
			ENDSECTION

			BEGINSECTION(RESOURCES)
				TESTCASE(test_resource);
				TESTCASE(test_buffer);
				TESTCASE(test_alxstring);
				TESTCASE(test_exception);
				TESTCASE(test_resource_create);
			ENDSECTION

			BEGINSECTION(UV)
				TESTCASE(test_uvloop);
				TESTCASE1(test_file, srcdir);
				TESTCASE(test_file_exception);
				TESTCASE1(test_file_delayed, srcdir);
				TESTCASE1(test_file_read, srcdir);
				TESTCASE(test_file_bitset);
				TESTCASE(test_fixed_file);
				TESTCASE(test_file_err);
				TESTCASE1(test_bitstream_read_write_sync, srcdir);
				TESTCASE1(test_bitstream_read_write, srcdir);
				TESTCASE1(test_randow_async_file, srcdir);
			ENDSECTION

			BEGINSECTION(MP4)
				TESTCASE1(test_mp4_read, srcdir);
				TESTCASE1(test_mp4_write, srcdir);
			ENDSECTION

		}


			std::cout << "==End Test==" << std::endl;

		ENDCASE;

		std::cout << orig << std::endl;

	}
	catch (::mg::core::mgexception & ex)
	{
		std::cout << "**mgexception**" << std::endl;
		std::cout << ex.what() << orig << std::endl;
		return 9;
	}
	catch (...)
	{
		std::cout << "***...***" << orig << std::endl;
		
		return 19;
	}


	

}

