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

//#include "mp4Meta.h"

#include "mp4parse.h"
#include "mp4write.h"

#define ASYNC

#ifdef ASYNC 
#define CLOSEASYNC false
#else
#define CLOSEASYNC
#endif


__ALX_BEGIN_NAMESPACE

class IMP4Mux
{
public:
	virtual int add_visual_stream
	(
		  const BYTE*    sps_nal_source
        , const unsigned sps_nal_size
		, const BYTE*	 pps_nal_source
        , const unsigned pps_nal_size
		, const uint64_t time_scale = 0
		, const unsigned int width  = 0
        , const unsigned int height = 0
		) = 0;


	virtual int add_audio_stream(
	  const unsigned int object_type
	, const unsigned int sample_rate
    , const unsigned int channels
	, const unsigned int target_bit_rate
	, const uint64_t time_scale = 0) = 0;

	virtual int add_extension_ltc_stream( const unsigned int avg_frame_rate = 400000
		, const uint64_t time_scale = 0) = 0;	 
	
	virtual void start(const TCHAR* file_out) = 0;
	virtual void set_stream_optional(
		  const int stream_id
		, const TCHAR*   lang
		, const DWORD	 bitrate
		, const uint64_t idr
		) = 0;

	virtual void add_sample(
		  int stream_id
		, const BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, uint64_t duration
		) = 0;
	
	virtual void end() = 0;

	virtual ~IMP4Mux()
	{
		//_RPT0(_CRT_WARN, "~IMP4Mux()");
	}
			
};

class IMP4Mux2: public IMP4Mux
{
public:
	virtual void set_auto_decoding_time(const int stream_id, const bool rhs) = 0;
};

class IMP4Mux3: public IMP4Mux2
{
public:
	virtual void set_max_distance(uint64_t max_distance) = 0;
	virtual void set_use_composition_in_distance(bool rhs) = 0;
};

class MP4Mux: public IMP4Mux3
{
#ifdef _DEBUG
public:
#endif

	MP4Write        _write;
	CMP4WriteMemory _headers;
	Cstring         _file_out;
	Cstring         _tmp_file_out;
	CMP4WriteFile   _body;
	bool			_ctts_offset;

	::mg::uv::loopthread _loop;
	

public:
	virtual ~MP4Mux()
	{
		//_RPT0(_CRT_WARN, "~MP4Mux()");
	};

	MP4Mux(::mg::uv::loopthread & loop):_ctts_offset(false), _loop(loop), _body(loop)
	{

	}

	void set_ctts_offset(bool rhs)
	{
		_ctts_offset = rhs;
	}

	virtual void set_max_distance(uint64_t max_distance)
	{
		_write.set_max_distance(max_distance);
	}

	virtual void set_use_composition_in_distance(bool rhs)
	{
		_write.set_use_composition_in_distance(rhs);
	}

	virtual int add_visual_stream
	(
		  const BYTE*    sps_nal_source
        , const unsigned sps_nal_size
		, const BYTE*	 pps_nal_source
        , const unsigned pps_nal_size
		, const uint64_t time_scale = 0
		, const unsigned int width  = 0
        , const unsigned int height = 0
		)
	{
		return _write.add_visual_stream(sps_nal_source, sps_nal_size, pps_nal_source, pps_nal_size, time_scale, width, height);
	}

	virtual void set_stream_optional(
		  const int stream_id
		, const TCHAR*   lang
		, const DWORD	 bitrate
		, const uint64_t idr
		)
	{
		_write.set_lang(stream_id, lang);
	}

	virtual int add_audio_stream(
	  const unsigned int object_type
	, const unsigned int sample_rate
    , const unsigned int channels
	, const unsigned int target_bit_rate
	, const uint64_t time_scale = 0)
	{
		/*if(!time_scale)
			time_scale = 10000000.00 / info.samplerate;
		*/
		return
		_write.add_audio_stream(
			  object_type
			, sample_rate
			, channels
			, target_bit_rate
			, time_scale);
	}

	virtual int add_extension_ltc_stream(
		  const unsigned int avg_frame_rate = 400000
		, const uint64_t time_scale = 0)
	{
		return
		_write.add_extension_ltc_stream(avg_frame_rate, time_scale);
	}
	
	
	void start(const TCHAR * file_out
		, unsigned int init_mem_buffer_size)
	{
		_file_out = file_out;
		_headers.open(init_mem_buffer_size);
		_write.write_ftyp(_headers);

		_tmp_file_out = _file_out.clone();
		_tmp_file_out += _T(".body.tmp");

		_body.open(_tmp_file_out);
	}

	virtual void start(const TCHAR * file_out){start(file_out, 3000000);}

	virtual void set_auto_decoding_time(const int stream_id, const bool rhs)
	{
		_write.set_auto_decoding_time(stream_id, rhs);
	}

	virtual void add_sample(
		  int stream_id
		, const BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, uint64_t duration
		)
	{
		_write.add_sample(stream_id
			, body
			, body_size
			, IFrame
			, composition_time
			, decoding_time
			, _body);

		
	}

	void write_meta(   const TCHAR* asset
                     , Ctime & date
				    )
	{
		/*
		XmlMetaReader<> w;
		w.set_asset(asset);
		w.set_date(date.ToXml()); //this should be UTC

		Cstring offset;
		        offset += Ctime::GetLocalOffset();

		GUID guid;
		WCHAR pszGuid[40];
		
		HRESULT hr = ::CoCreateGuid(&guid);
		ALXCHECK_HR(hr);

		int ret = StringFromGUID2(guid, pszGuid, sizeof(pszGuid));
		
		if(0 == ret)
			ALXTHROW_T(_T("ALX: CANNOT GENERATE MP4 HEADER FILEID"));

		w.set_id(pszGuid);
		w.set_offset(offset);

		_write.write_xml_meta(_headers
			, w.write()
			, META_BOX
			, META_SIZE);

		*/
	}

	virtual void end()
	{
		_body.close();

		_write.compute_auto_decoding_time();
		_write.level_start_time(!_ctts_offset);

		_write.write_moov(_headers);

		//SHStream body_read(_tmp_file_out);

#ifdef ASYNC
		file_bitstream body_read(_loop);
		               body_read.open_sync(_tmp_file_out);
#else

		sync_file_bitstream body_read;
		                   body_read.open(_tmp_file_out);
#endif
           
		_write.open_mdat(_headers, body_read.size());
		
		_headers.flush();
			
		uint64_t headers_size = _headers.get_size();

		_write.rebase_streams(_headers, headers_size);

		_headers.flush();
            
		_write.end();

		//SHStreamWrite full(_file_out);
#ifdef ASYNC		
		file_bitstream full(_loop);
		               full.open_sync(_file_out, O_CREAT | O_WRONLY | O_TRUNC);

#else
		sync_file_bitstream full;
		                   full.open(_file_out, false);
#endif

			uint32_t uls(0);

			//full.Write(_headers.get_buffer(), _headers.get_size(), &uls);
			
			//_ASSERTE(uls == _headers.get_size());

			unsigned int chunk_size = 102400;
			
			copy_stream(_headers.storage(), full, chunk_size);

			DBGC2(_T("HEADERS COPIED %llu %llu"), headers_size, full.get_position());

			copy_stream(body_read, full, chunk_size);

			DBGC2(_T("BODY COPIED %llu %llu"), body_read.size(), full.get_position());

			_loop->loop_now();
			
			//::Sleep(1000);

			body_read.close(CLOSEASYNC);
			full.close(CLOSEASYNC);

			/*

			CBuffer<BYTE> chunk(chunk_size);

			for(uint64_t x = 0; x < body_read.size();x)
			{
				ULONG ucopy(0);
				ULONG uwrite(0);

				body_read.Read(chunk.get(), chunk_size, &ucopy);
				if(ucopy)
				{
					full.Write(chunk.get(), ucopy, &uwrite);
					_ASSERTE(uwrite == ucopy);
				}

				if(ucopy < chunk_size)
					break;

				x += ucopy;
			}

			body_read.Release();

			*/



			//if(!::DeleteFile(_tmp_file_out))
			//	ALXTHROW_LASTERR;

			delete_file(_tmp_file_out);
	}

	Cstring & get_file_out(){return _file_out;}
};
__ALX_END_NAMESPACE

