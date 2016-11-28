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
#include <mgmedia.h>

using namespace MGCORE;

#include <sstream>
#include "test_base.h"

//coverage declarations
template class CBuffer<unsigned char>;
template class ResetBuffer<unsigned char>;
template class CstringT<TCHAR>;

int test_buffer(){

	
	CBuffer<unsigned char> b;

	CHECK(0, b.size(), _T("Size Check"));

	unsigned char a[] = {0x01, 0x02, 0x03, 0x04};

	b.add(a, sizeof(a));

	CHECK(4, b.size(), _T("Check size 4"));
	CHECK(sizeof(a), b.size(), _T("Check Size"));

	
	CBuffer<unsigned char> b2(2);

	b2.add(a, sizeof(a));

	CHECK(4, b2.size(), _T("Check size b2"));

	TEST_OK;

}

int test_alxstring(){

	Cstring s = _T("pippo");

	CHECK(5, s.size(), _T("string size"));

	const TCHAR* pp = s;
	
	std::stringstream ss;
	
	//std::cout << pp << std::endl;

	ss << _T("-->") << std::endl
           << _T("\t") << pp << std::endl
           ; //<< "\t\t[" << s << "]" << std::endl;

       std::cout << ss.str() ;

	CHECK(true , (_T("pippo") == s ), _T("Check1 == "));
	CHECK(false, (_T("lillo") == s ), _T("Check2 == "));
	
	CHECK(true, (s == _T("pippo") ), _T("Check3 == "));
	CHECK(false, (s == _T("lillo")), _T("Check4 == "));	
	TEST_OK;
}

int test_exception(){

	try{
		MGCHECK(-1);

	}catch(mgexception & mgex)
	{
		CHECK(-1, mgex.get_error_number(), _T("wrong error number"));

		Cstring e = mgex.toString();

		std::cout << e << std::endl;
				
		TEST_OK;
	}


	CHECK(false, true, _T("TEST EXCEPTION FAILURE"));
}

