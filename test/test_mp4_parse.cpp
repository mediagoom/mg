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

#include "test_base.h"

#include <sstream>
#include <fstream>

#include "test_uv.h"
#include "test_bitstream.h"



using namespace MGUV;
using namespace MGCORE;


int pick_samples(
	  std::ostream & ost
	, uint64_t start
	, uint64_t end
	, CMP4 &mp4
	, MP4Reader &reader
	, int stream = -1)
{

	sample_stream_info ms;

	ms.composition_time = 0;

	//MP4Reader reader;
	//reader.parse(mp4);

	uint64_t max_distance = 0;
	uint64_t max_distance_time = 0;


	//if(0 == end)
	//	    end = reader.get_duration();

	std::vector<bool> streams(reader.stream_count());
	std::vector<uint64_t> samples(reader.stream_count());

	ost << _T(" duration: ")
		<< HNS(reader.get_duration())
		<< std::endl;

	for (unsigned int i = 0; i < streams.size(); i++)
	{
		ost << i
			<< _T(") offset: ")
			<< HNS(reader.get_stream_offset(i))
			<< _T(" duration: ")
			<< HNS(reader.get_duration(i))
			<< _T(" sample duration: ")
			<< HNS(reader.get_sample_duration(i))
			<< _T(" [")
			<< reader.get_sample_duration(i)
			<< _T("]")
			<< _T(" samples count: ")
			<< reader.get_sample_count(i)
			<< _T(" start time: ")
			<< HNS(reader.get_start_time(i))
			<< _T(" end time: ")
			<< HNS(reader.get_end_time_plus_duration(i))
			/*
			<< _T(" {"
			<< HNS(  (reader.get_end_time_plus_duration(i) - reader.get_start_time(i)) / reader.get_sample_count(i) )
			<< _T("}"
			*/
			<< std::endl;

		streams[i] = true;
	}

	if (0 < start)
		reader.move(start);

	ost
		<< _T("stream")
		<< _T("\t")
		<< _T("sample")
		<< _T("\t")
		<< _T("composition_time")
		<< _T("\t")
		<< _T("decoding_time")
		<< _T("\t")
		<< _T("duration")
		<< _T("\t\t")
		<< _T("bIsSyncPoint")
		<< _T("\t")
		<< _T("offset")
		<< _T("\t")
		<< _T("size")
		<< std::endl;

	bool work = true;

	while (work)
	{
		work = reader.next_file_chunks(ms);

		if (!work)
			continue;

		if (-1 < stream
			&& stream != ms.stream
			&& work)
			continue;

		ost << ms.stream
			<< _T("\t")
			<< ms.sample_number
			<< _T("\t")
			<< HNS(ms.composition_time + reader.get_stream_offset(ms.stream))
			<< _T("\t")
			<< HNS(ms.decoding_time)
			<< _T("\t")
			<< HNS(ms.duration)
			<< _T("\t")
			<< ms.bIsSyncPoint
			<< _T("\t")
			<< ms.offset
			<< _T("\t")
			<< ms.size
			<< std::endl;

		streams[ms.stream] = (ms.decoding_time < static_cast<int64_t>(end)) || !end;

		for (unsigned int i = 0; i < streams.size(); i++)
			work &= streams[i];

		uint64_t d = ms.composition_time - ms.decoding_time;

		if (max_distance < d)
		{
			max_distance = d;
			max_distance_time = ms.composition_time;
		}

	}

	ost << _T("MAX DISTANCE: ") << HNS(max_distance) << _T(" AT ") << HNS(max_distance_time) << std::endl;

	return 0;
}

int fileout(const Cstring & fn, std::string & so)
{
	std::ofstream f;
	f.open(fn);
	f << so;
	f.close();

	return 0;
}

uint64_t get_end_time()
{
	Cstring t = get_env_variable(_T("MGTEST_MP4_END_TIME"));

	if (0 == t.size())
		return 0;

	else
		return t;
}

uint64_t get_chunk_size()
{
	Cstring t = get_env_variable(_T("MGTEST_MP4_CHUNK_SIZE"));

	if (0 == t.size())
		return 64 * 2 * 1024;

	else
		return t;
}

int test_mp4_read(const Cstring & src)
{
	try {

		CResource<uvloopthread> t; t.Create(); t->start();
		
		USEF1;

		Cstring pick = f1.clone();

		pick += _T(".pick.txt");
		
		{

			std::cout << _T("pre buffer filled") << std::endl;

			CResource<CBuffer<unsigned char> > pick_info = file_fill_buffer(pick, t);


			std::cout << _T("buffer filled") << std::endl;

			uint32_t chunk = static_cast<uint32_t>(get_chunk_size());

			MP4File mp4(t, chunk);
			mp4.open(f1);


			std::cout << _T("file opened") << std::endl;

			MP4Reader reader;
			reader.parse(mp4);

			std::cout << _T("opened file") << std::endl;

			CHECK(2, reader.stream_count(), _T("INVALID STREAMS"));

			std::stringstream sout;



			int r = pick_samples(sout
				, 0
				, 10ULL * 1000ULL * 10000ULL
				, mp4
				, reader
			);

			std::string bo = sout.str();

			uint32_t size = bo.length();

			if (size > pick_info->size())
				size = pick_info->size();

			//std::cout << pick_output << std::endl;

			CHECK(0, r, _T("PICK_SAMPLES_FAILED"));

			int comp = compare_buffer(reinterpret_cast<const unsigned char*>(bo.c_str()), pick_info->get(), size);

			if (0 != comp)
			{
				Cstring t1 = f1.clone();
				t1 += _T("MP4PICK1.tmp");

				std::string pickcontent(reinterpret_cast<char *>(pick_info->get()));

				fileout(t1, pickcontent);

				Cstring t2 = f1.clone();
				t2 += _T("MP4PICK2.tmp");

				fileout(t2, bo);
			}

			CHECK(0, comp, _T("INVALID CONTENT FOUND"));

			
			t->stop();
			t->end();
			

		}
		
		

		TEST_OK;
	}
	catch (const std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 5;
	}
	
}


int test_mp4_write(const Cstring & src)
{
	try {

		CResource<uvloopthread> t; t.Create(); 
		t->set_wait_time(10);
		t->start();

		USEF1;
		
		/*
		Cstring out = f1.clone();

		out += _T(".out.tmp.mp4");
		*/

		Cstring out = _T("MEDIA_out.tmp.mp4");

		uint32_t chunk = static_cast<uint32_t>(get_chunk_size());

		STARTCLOCK;

		MP4File mp4(t, chunk);
		//sync_file_media_bitstream<CMP4> mp4(chunk);
		mp4.open(f1);

		MP4Reader reader;
		reader.parse(mp4);



		{
			CMP4EditConsole edit(t);

			edit.start(out);

			edit.Add(mp4, reader, 0, get_end_time());

			std::cout << HNS(TIMECLOCK) << std::endl;

			edit.End();

			std::cout << HNS(TIMECLOCK) << std::endl;
		}

		t->stop();
		t->end();

		std::cout << HNS(TIMECLOCK) << std::endl;

		TEST_OK;
	}
	catch (const std::exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 5;
	}

}

