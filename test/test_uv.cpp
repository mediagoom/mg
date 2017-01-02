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

using namespace MGUV;
using namespace MGCORE;

#include "test_bitstream.h"
#include "test_base.h"
#include "test_uv.h"
#include "test_util.h"

template class MGCORE::CResource<::mg::uv::uvloop>; 

template class MGCORE::CResource<::mg::uv::uvloopthread>;

template class MGCORE::CResource<MGCORE::CBuffer<unsigned char> >;

template class MGCORE::bitstream<file_bitstream>;

int test_uvloop()
{
	try {
		
		uvloop l;

		l.run();

		uvloopthread t;

		t.start();

		t.stop();

		bool ended = t.is_done(1000);

		CHECK(ended, true, _T("uvloopthread timeout. May Crash!."));

		{
			uvloopthread k;
			k.start();

			ended = k.is_done(10);

			CHECK(ended, false, _T("uvloopthread unexpected exited."));

			//here we check the descrutor.

		}

		l.stop();
		l.close();
	}
	catch (MGCORE::mgexceptionbase & ex)
	{
		std::cout << ex.get_error_number() << std::endl;
		return 5;
	}



	TEST_OK;
}

/*
void stop(uvloopthread * pt, CResource<uv_fs_cb> & fs)
{
	pt->stop();
}
*/


int do_test_file(const Cstring & src, bool delayed, bool read = false)
{
	try {
		
		CResource<uvloopthread> t;
		
		//signal_event w;

		t.Create();

		Cstring f1 = src.clone();
		f1 += _T("/test_assets/");
		f1 += _T("MEDIA1.MP4");

		std::cout << _T("TEST FILE: ") << f1 << std::endl;

		t->start();

		uvloopthread * pt = t;

		cfile file(t);
		CResource<CPromise<uv_fs_t> > promise = file.open(f1);

		if (delayed)
		{
			t->is_done(1000);

			CHECK(promise->has_value(), true, _T("promise failed to set value"));
		}

		promise->set_er().set_cb(

			[pt, read, & file](CResource<uv_fs_t> & fs) mutable
			{ 
				if (!read)
				{
					pt->stop();
				}
				else
				{
					CResource<CPromise<CBuffer<unsigned char> > > prom2 = file.read(8 * 1024);

					prom2->set_cb(
						[pt](CResource<CBuffer<unsigned char> > & buf) mutable
						{
							/*
							DECLARE_CH_HEX;

							for (size_t x = 0; x < buf->size(); x++)
							{
								unsigned char ch = buf->getAt(x);
								char out[4] = { 0x0, 0x0, 0x0, 0x0 };

								out[0] = s_chHexChars[(ch >> 4) & 0x0F];
								std::cout << out ;

								out[0] = s_chHexChars[ch & 0x0F];
								std::cout << out;

							}
							*/

							/*
							Cstring output = hexformat(buf->get(), buf->size());

							std::cout << output << std::endl;
							*/
							char b[200];
							memset(b, 0, 200);
							int s = (100 < buf->size())?100:buf->size();
							std::cout << "=================" << s << " ====================" << std::endl;
							for(int k = 0; k < s; k++){
							   unsigned char c = buf->getAt(k);
								if(0 == c)
								   c = '0';
								//std::cout << " ++ " << c << " == " ;

							   	printf("%c", c);
								sprintf(b + k, "%c" , c);
							   }
							
							std::cout << "============================" << std::endl;
							std::cout << b << std::endl;
							pt->stop();
						}

						
					);
				}
			}
		
		);
		//promise->set_er([](CResource<uv_fs_cb> fs) { pt->stop(); });

		//std::function<void(CResource<uv_fs_cb> & pres)> fc = std::bind(pt, &stop, std::placeholders::_1);

		//promise->set_er().set_cb(fc);

		t->is_done(1000);

		std::cout << "SIZE(1): " << file.size() << std::endl;

		t->loop_now();

		t->is_done(100);

		std::cout << "SIZE(2): " << file.size() << std::endl;

		//t->stop();

		//w.wait(60000);

		//t->start();

		bool done = t->is_done(10000);

		std::cout << "END" << std::endl;

		CHECK(done, true, _T("uvloopthread timeout"));
		
		t->join();

		t->end();
		
		CHECK(file.size(), 17477683, _T("wrong size"));

		//CHECK(tt->get_err(), 0, _T("loop has error"));

	}
	//catch (MGCORE::mgexceptionbase & ex)
	catch (const std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 5;
	}

	std::cout << "BEFORE EXIT" << std::endl;

	TEST_OK;
}

int test_file(const Cstring & src)
{
	return do_test_file(src, false);
}

int test_file_exception()
{
	try {

		int r = test_file(_T("/"));

		CHECK(5, r, _T("uvloopthread exception handling ko"));

		TEST_OK;

	}
	catch (const std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 5;
	}
	/*
	catch (MGCORE::mgexceptionbase & ex)
	{
		std::cout << "mgexceptionbase" << std::endl;
		std::cout << ex.what() << std::endl;
		return 5;
	}*/
}

int test_file_delayed(const Cstring & src)
{
	return do_test_file(src, true);
}

int test_file_read(const Cstring & src)
{
	
	return do_test_file(src, false, true);

	TEST_OK;
}

int test_bitstream(
	unsigned char * pbuffer
	, uint32_t size
	, const std::function<int(IBitstream & bs)> & check_func)
{
	try {
		

		CResource<uvloopthread> t; 
			t.Create(); 
			t->start();

		Cstring ff(_T("bitset.tmp"));

		cfile output(t);
		output.open_sync(ff, O_CREAT | O_WRONLY | O_TRUNC);

		int ret(0);

		signal_event written;

		output.write(pbuffer, size)->set_er().set_cb(
			[&written](CResource<uv_fs_t> & req)
		{
			written.signal();
		}
		);

		bool w = written.wait(10000);

		CHECK(w, true, _T("write time out"));

		output.close();

		CResource<file_bitstream> fm(new file_bitstream(t));
		fm->open_sync(ff);

		bitstream<file_bitstream> bs(fm);

		ret = check_func(bs);

		t->stop();



		t->join();
		t->end();

		return ret;

	}
	catch (const std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 5;
	}
}

int test_file_bitset()
{

	unsigned char mem[4][4];

	//std::wcout << sizeof(mem) << std::endl;

	::memset(mem, 0, (sizeof(mem)));

	fill_bitset(mem);

	CHECK(sizeof(mem), 4 * 4, _T("INVALID MEMORY SIZE"));
	
	return test_bitstream(reinterpret_cast<unsigned char *>(mem), 4 * 4, check_bitset_bitream);
}


int test_fixed_file()
{
	unsigned char mem[] = { CHAR4 };

	return test_bitstream(mem, 4, check_char4_bitstream);

}

CResource<CBuffer<unsigned char> > file_fill_buffer(const Cstring & f, CResource<uvloopthread> & t, uint64_t size)
{
	cfile file(t);
		  file.open_sync(f);

		  if (0 == size)
			  size = file.size();

		  CResource<CBuffer<unsigned char> > tmp;

		  signal_event read;

		  file.read(static_cast<uint32_t>(size))->set_er().set_cb([&tmp, &read](CResource<CBuffer<unsigned char> > & buf)
		  {
			  tmp = buf;
			  read.signal();

		  });

		  bool w = read.wait(10000);
		  if (!w)
			  MGCHECK(-1000);

		  return tmp;

}

int test_bitstream_read_write_sync(const Cstring & src)
{
	try {

		CResource<uvloopthread> t; t.Create(); t->start();

		/*Cstring f1 = src.clone();
		f1 += _T("/test_assets/");
		f1 += _T("MEDIA1.MP4");
		*/

		USEF1;

		Cstring ff(_T("bitset.tmp"));

		int tot = 0;
		int size = 1024 * 64;

		CResource<CBuffer<unsigned char> > fin = file_fill_buffer(f1, t, size);
		
		t->stop();

		t->join();
		t->end();
		
		CResource<fixed_memory> fm; fm.Create();
		fm->_buf = fin->get();
		fm->_buf_len = fin->size();
		
		CBuffer<unsigned char> buf2(size);

		CResource<fixed_memory> fm2(new fixed_memory(true));
		fm2->_buf = buf2.get();
		fm2->_buf_len = size;

		bitstream<fixed_memory> bs(fm);
		bitstream<fixed_memory> bs2(fm2, false);

		CHECK(size, fm->_buf_len, _T("INVALID READ"));

		tot = transfer_bits(bs, bs2);

		int ret = compare_buffer(fin->get(), buf2.get(), tot);
		
		CHECK(0, ret, _T("invalid check buffer"));

		TEST_OK;

	}
	catch (const std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 5;
	}

}

int test_bitstream_read_write(const Cstring & src)
{
	try {

		CResource<uvloopthread> t; t.Create(); t->start();
		
		Cstring f1 = src.clone();
				f1 += _T("/test_assets/");
				f1 += _T("MEDIA1.MP4");

			Cstring ff(_T("bitset.tmp"));

			int tot = 0;

		{
			
			std::cout << _T("TEST FILE: ") << f1 << std::endl;
			
			CResource<file_bitstream> fm(new file_bitstream(t));
			fm->open_sync(ff, O_CREAT | O_WRONLY | O_TRUNC);

			bitstream<file_bitstream> outs;
			                          outs.create(fm, 1024, false);

			CResource<file_bitstream> fm2(new file_bitstream(t));
			fm2->open_sync(f1);

			bitstream<file_bitstream> ins(fm2);
			
			tot = transfer_bits(ins, outs);

			
			fm->close(false);
			fm2->close(false);

			

			
		}

		std::cout << "** " << tot << " **" << std::endl;

		CResource<CBuffer<unsigned char> > fin = file_fill_buffer(f1, t, tot);
		CResource<CBuffer<unsigned char> > fout = file_fill_buffer(ff, t, tot);

		CHECK(tot, fin->size(), _T("invalid file 1"));
		CHECK(tot, fout->size(), _T("invalid file 2"));
		
		t->stop();

		int ret = compare_buffer(fin->get(), fout->get(), tot);
			
			
			
			t->join();
			t->end();

			CHECK(0, ret, _T("invalid check buffer"));

			TEST_OK;
		}
		catch (const std::exception & ex)
		{
			std::cout << ex.what() << std::endl;
			return 5;
		}
	
}

int test_file_err()
{
	try {

		CResource<uvloopthread> t; 
		                        t.Create(); 
					t->start();

		
		Cstring ff(_T("bitset.tmp"));

					
			cfile fm(t);
			fm.open_sync(ff, O_CREAT | O_WRONLY | O_TRUNC);

			CHECK(0, fm.get_position(), "CHECK POSITION");
			CHECK(0, fm.size(), "CHECK SIZE");
			CHECK(true, fm.eof(), "CHECK EOF");

			signal_event read;
			             read.reset();

			fm.read(50)->set_er([&read](std::exception_ptr eptr) -> bool
			{
				read.signal();
			        std::cout << "READ EXCEPTION" << std::endl;
				
				std::rethrow_exception(eptr);

				return true;
			}
			).set_cb([&read](CResource<CBuffer<unsigned char> > & buf)
			{
				
				read.signal();

			});

			std::cout << "WAIT" << std::endl;

			bool w = read.wait(10000);
			if(!w)
			{
				t->end(); //this will re-throw the exception. //if we let the destructro throw we coredump.
			}
			CHECK(true, w, "WAIT TIME OUT");

			fm.close(false);

		t->stop();

		t->join();
		t->end();

		

		return 9;

	}
	catch (const std::exception & ex)
	{
		std::cout << "EXCEPTION" << std::endl;
		std::cout << ex.what() << std::endl;
		return 0;
	}catch(...)
		{
			std::cout << "EX999999999" << std::endl;
			return 12;
		}


}

int test_randow_async_file(const Cstring & src)
{
	try {

		CResource<uvloopthread> t; t.Create(); t->start();

		USEF1;

		Cstring ff(_T("output.tmp"));

		int tot = 0;
		uint32_t size = 1024 * 64;

		uint64_t chunks = 250;

		Cstring v = get_env_variable(_T("MGTEST_FILE_COPY_CHUNK"));

		if (0 < v.size())
			chunks = v;


		int out(0);
		int done(0);
		int read(0);

		STARTCLOCK;

		cfile fr(t);
		      fr.open_sync(f1);

		cfile fw(t);
			  fw.open_write_sync(ff);

	    std::cout << HNS(TIMECLOCK) << std::endl;

		for (uint32_t i = 0; i <= chunks; i += 2 /*++*/)
		{
			uint32_t p = i * size;
			fr.set_position(p);
			read++;
			fr.read(size)->set_cb(
			
				[&fw, &out, &done, p](CResource<CBuffer<unsigned char> > & buf)
				{
					out++;
					fw.set_position(p);
					fw.write(buf->get(), buf->size())->set_cb(
						[&done](CResource<uv_fs_t> & r)
						{
							done++;
						}
						);
				}
			);
		}

		std::cout << HNS(TIMECLOCK) << std::endl;

		
		for (uint32_t i = 1; i < chunks; i += 2)
		{
			uint32_t p = i * size;
			fr.set_position(p);
			read++;
			fr.read(size)->set_cb(

				[&fw, &out, &done, p](CResource<CBuffer<unsigned char> > & buf)
			{
				out++;
				fw.set_position(p);
				fw.write(buf->get(), buf->size())->set_cb(
					[&done](CResource<uv_fs_t> & r)
				{
					done++;
				}
				);
			}
			);
		}
		

		std::cout << _T("READ\t") << HNS(TIMECLOCK) << std::endl;

		while (fw.outstanding() || fr.outstanding() || (read > done))
		{

			std::cout << _T("W:\t") << fw.outstanding() << _T("\tR:\t") << fr.outstanding() 
				<< _T("\tread:\t") << read
				<< _T("\tout:\t") << out
				<< _T("\tdone:\t") << done
				<< std::endl;

			t->is_done(25);
		}

		std::cout << _T("WRITTEN\t") << HNS(TIMECLOCK) << std::endl;


		t->stop();

		t->join();
		t->end();


		std::cout << _T("W:\t") << fw.outstanding() << _T("\tR:\t") << fr.outstanding()
			<< _T("\tread:\t") << read
			<< _T("\tout:\t") << out
			<< _T("\tdone:\t") << done
			<< std::endl;

		fw.close(false);
		fr.close(false);


		int64_t fsize = chunks * size;

		std::cout << _T("COMPLETED\t") << HNS(TIMECLOCK) << _T(" size: " ) << fsize << std::endl;
		

		CHECK(0, compare_file(ff, f1, fsize), _T("invalid check file"));

		std::cout << _T("CHECKED\t") << HNS(TIMECLOCK) << std::endl;

		TEST_OK;

	}
	catch (const std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 5;
	}
	catch (...)
	{
		return 9;
	}
}

