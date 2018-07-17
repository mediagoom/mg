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

#include <iostream>
#include <map>
//#include <mgmedia.h>


#define CHECK(expected, real, msg) if(expected != real){std::cout << RED << "\t\tko\t" << msg << " expected: " << expected << " found: " << real << " [" << __FILE__ << "] " << __LINE__ << std::endl; return 1;}
#define CHECKNOT(expected, real, msg) if(expected == real){std::cout << RED << "\t\tko\t" << msg << " not expected: " << expected << " found: " << real << " [" << __FILE__ << "] " << __LINE__ << std::endl; return 1;}

#define TEST_OK return 0;

struct TEST_TIMING
{
	uint64_t count;
	uint64_t sum;
	uint64_t max;
	uint64_t min;
	uint32_t failed;
};

inline void push_time(const Cstring & func
	, uint64_t time
	, std::map<Cstring
	, TEST_TIMING> & hash
	, int failed
	, int &lastfailed
)
{
	std::cout << "\tTIME\t" << func << "\t" << HNS(time) << std::endl;

	if (hash.find(func) == hash.end())
	{
		hash[func] = TEST_TIMING{ 0, 0, 0, UINT64_MAX, 0 };
	}

	TEST_TIMING & t = hash[func];

	t.sum += time;
		t.count++;
	if (t.min > time)
		t.min = time;
	if (t.max < time)
		t.max = time;
	if (failed > lastfailed)
		t.failed++;

	lastfailed = failed;

}

inline void output_time(std::map<Cstring, TEST_TIMING> & hash)
{
	std::cout << _T("TEST")
		<< _T("\t")
		<< _T("count")
		<< _T("\t")
		<< _T("mean")
		<< _T("\t")
		<< _T("max")
		<< _T("\t")
		<< _T("min")
		<< _T("\t")
		<< _T("total")
		<< _T("\t")
		<< _T("success")
		<< _T("\t")
		<< _T("failed")
		<< std::endl;

	std::map<Cstring, TEST_TIMING>::const_iterator it = hash.begin();

	while (it != hash.end())
	{
		//it->second;

		std::cout << it->first
			<< _T("\t")
			<< it->second.count
			<< _T("\t")
			<< HNS(static_cast<uint64_t>(it->second.sum / it->second.count))
			<< _T("\t")
			<< HNS(it->second.max)
			<< _T("\t")
			<< HNS(it->second.min)
			<< _T("\t")
			<< HNS(it->second.sum)
			<< _T("\t")
			<< it->second.count - it->second.failed
			<< _T("\t")
			<< it->second.failed
			<< std::endl;

		it++;

	}
}

//define FUNCTIME(FUNC)	std::cout << "\tTIME\t" #FUNC "\t" <<  HNS(TIMECLOCK) << std::endl;
#define FUNCTIME(FUNC) push_time(FUNC, TIMECLOCK, timehash, failed, lastfailed);


#define STARTCASE int res = 0; \
				  MGCOLOR original = get_current_color(); \
                  ALX::Cstring test_section = _T("*"); if(argc > 1){test_section = argv[1];} \
		  ALX::Cstring test_name   = _T("*");  if(argc > 2){test_name = argv[2];}  \
		  std::cout << "\tSTART TESTING: " << test_section << _T("\t") << test_name << std::endl; \
		  int tests(0), successed(0), failed(0), lastfailed(0);\
		  STARTCLOCK; std::map<Cstring, TEST_TIMING> timehash; 
		  


#define TESTCASE(FUNC) if(test_name == _T( #FUNC )  || test_name == _T("*")){\
				std::cout << GREEN << "\t----------------------\t" #FUNC "\t----------------------\t" << original << std::endl; \
				try{  tests++; \
						res = FUNC(); if(res){std::cout << RED << "\tTEST-FAILED\t" #FUNC "\t" << std::endl; failed++;}else{std::cout << "\tOK\t" #FUNC "\t" << std::endl; successed++;} \
					 }catch(mgexception & mgex){std::cout << RED << "\tTEST-FAILED\t" #FUNC "\t" << mgex.toString() << std::endl; failed++; } \
					  catch(mgexceptionbase & mgex){std::cout << RED << "\tTEST-FAILED\t" #FUNC "\t" << mgex.get_error_number() << std::endl; failed++;} \
				FUNCTIME( _T( #FUNC ) )} 


#define TESTCASE1(FUNC, P) if(test_name == _T( #FUNC ) || test_name == _T("*")){\
				std::cout << GREEN << "\t----------------------\t" #FUNC "\t----------------------\t" << original << std::endl; \
				try{  tests++; \
						res = FUNC(P); if(res){std::cout << RED << "\tTEST-FAILED\t" #FUNC "\t" << std::endl; failed++;}else{std::cout << "\tOK\t" #FUNC "\t" << std::endl; successed++;} \
					 }catch(mgexception & mgex){std::cout << RED << "\tTEST-FAILED\t" #FUNC "\t" << mgex.toString() << std::endl; failed++; } \
					  catch(mgexceptionbase & mgex){std::cout << RED << "\tTEST-FAILED\t" #FUNC "\t" << mgex.get_error_number() << std::endl; failed++;} \
				FUNCTIME( _T( #FUNC ) )} 



#define BEGINSECTION(SECNAME) if(test_section == _T( #SECNAME ) || test_section == _T("./" #SECNAME "")   || test_section == _T("*")){  \
			        std::cout << "======" << #SECNAME << "======" << std::endl;
#define ENDSECTION } 

#define ENDCASE  if(failed){std::cout << RED;}else{std::cout << GREEN;} \
                 std::cout << "SUMMARY:\t" << tests << "\tSUCCEEDED:\t" << successed << "\tFAILED:\t" << failed << original << std::endl \
			     ; output_time(timehash) \
                 ; std::cout << orig << std::endl; \
                ;return failed;

#define F1SRC(SRC) Cstring f1 = SRC.clone(); f1 += _T("/test_assets/"); f1 += _T("MEDIA1.MP4");
#define USEF1 F1SRC(src)

