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

#include "mp4mux.h"
#include "MediaJoin.h"

#define MAX_STREAMS 16

__ALX_BEGIN_NAMESPACE

struct stream_edit_info
{
   uint64_t last_composition;
   uint64_t last_duration;
   uint64_t last_decoding;

    int64_t next_composition;
   uint64_t next_duration;
   uint64_t next_decoding;

   uint64_t stream_composition;
   uint64_t stream_duration;
   uint64_t stream_decoding;

   uint64_t time_scale;

   unsigned int     stream_id;       

   stream_edit_info():
	         last_composition(0)
           , last_duration(0)
           , last_decoding(0)
           , stream_id(0)
		   , time_scale(0)
	{}
};

struct audio_edit_info: stream_edit_info
{
	unsigned int object_type;
	unsigned int sample_rate;
        unsigned int channels;
        unsigned int bit_rate;
	

	audio_edit_info():
		  object_type(0)
                , sample_rate(0)
                , channels(0)
                , bit_rate(0)
	{}
};

struct video_edit_info: stream_edit_info
{
	unsigned char * p_sequence;
	unsigned int  sequence_size;
	unsigned char * p_picture;
	unsigned int  picture_size;

	video_edit_info():
                  p_sequence(NULL)
		, sequence_size(0)
		, p_picture(NULL)
		, picture_size(0)
	{}

	void set_info(
			const unsigned char * p_sequence_data
		      , const unsigned int  sequence_data_size
		      , const unsigned char * p_picture_data
		      , const unsigned int  picture_data_size
		     )
	{

		p_sequence = new unsigned char[sequence_data_size];
		p_picture  = new unsigned char[picture_data_size];

		memcpy(p_sequence, p_sequence_data, sequence_data_size);
		memcpy(p_picture,  p_picture_data,  picture_data_size);

		sequence_size = sequence_data_size;
		picture_size  = picture_data_size;
	}

	virtual ~video_edit_info()
	{
		if(p_sequence)
			delete[] p_sequence;
		if(p_picture)
			delete[] p_picture;


		p_sequence = NULL;
		p_picture  = NULL;
	

	}

};

struct stream_map
{
	unsigned int internal_index;
	bool video;
};

struct edit_info
{
    int video_streams_count;
    int audio_streams_count;

    video_edit_info video_info[24];
    audio_edit_info audio_info[24];

	stream_map map[24];
	
    int file_count;

	int ltc;

	bool ltc_interpolated;

	edit_info():file_count(0)
		, ltc(-1)
		, ltc_interpolated(false)
	{}
};


class CMP4EditBase
{

	  int _look_head;

protected:

	CMP4EditBase():_look_head(20)
	{}

	virtual ~CMP4EditBase(){}

	virtual void info_input_stream(const MP4Reader &reader) const
	{

	}

	virtual void info_process_sample(const sample_stream_info & ms
		, uint64_t composition
		, uint64_t decoding
		, uint64_t stream_offset
		, uint64_t total_stream_samples) const 
	{
		
	}

	virtual void info_pre_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		
	}

	virtual void info_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		
	}

	virtual void info_composition_info2(const int stream_id, stream_edit_info * ps
		, uint64_t composition, uint64_t decoding)
	{
		
	}

	virtual void move_to_read(  uint64_t start
			, MP4Reader &reader
			);

	void do_edit_header_mux(    MP4Reader &reader
							  , IMP4Mux2   & mp4w
							  , edit_info & info);

	void do_edit_mux(
				   int64_t start
				,  int64_t end
				, CMP4      & mp4
				, MP4Reader & reader
				, IMP4Mux2   & body
				, edit_info & info
				, bool physical = true
				, bool discard_pre_start = false
				);

	virtual void body_add_sample(
					IMP4Mux2 & muxer
					,   int stream_id
					, const BYTE * body
					, const unsigned int body_size
					, bool IFrame
					, uint64_t composition_time
					, uint64_t decoding_time
					, uint64_t duration
				)
	{
		muxer.add_sample(stream_id, body, body_size, IFrame, composition_time, decoding_time, duration);
	}
};





class CMP4Edit_: public CMP4EditBase
{

	
	edit_info _info;

	bool _physical;
	bool _discard_pre_start;

protected:

	
	virtual IMP4Mux2 & get_mux() = 0;
	virtual void set_ctts_offset_(bool rhs)
	{}
	

private:

	/*void do_edit_header_mux( MP4Reader &reader
						  , MP4Mux & mp4w
						  , edit_info & info);
	void do_edit_mux(
				  uint64_t start
				, uint64_t end
				, CMP4 &mp4
				, MP4Reader &reader
				, MP4Mux & body
				, edit_info & info);*/
	
public:

	CMP4Edit_():_physical(true)
		, _discard_pre_start(false)
	{}

	void set_ctts_offset(bool rhs){set_ctts_offset_(rhs);}
	void set_physical(bool rhs){_physical = rhs;}
	void set_discard_pre_start(bool rhs){_discard_pre_start = rhs;}

	void start(const TCHAR * psz_output_file)
	{
	   get_mux().start(psz_output_file);
	   _info.file_count = 0;
	}    
	
	void Add(/*MP4File*/CMP4 & mp4, MP4Reader & reader, uint64_t start, uint64_t end)
	{
		
		if(0 == _info.file_count)
		{
			do_edit_header_mux(
				reader
				, get_mux()
				, _info);
		}

		info_input_stream(reader);

		do_edit_mux(
			  start
			, end
			, mp4
			, reader
			, get_mux()
			, _info
			, _physical
			, _discard_pre_start);

	}

	
	void Add(const TCHAR * pszfile, uint64_t start, uint64_t end, bool start_is_composition = false)
	{
		SYNCMP4File mp4;
		mp4.open(pszfile);

		MP4Reader reader;
		reader.parse(mp4);

		if(start_is_composition && (0 < start) && reader.HasVisual()) //adjust start to use decoding time since positioning is in decoding time
		{

			int streamid = reader.VisualStream();

			uint64_t sample_number = reader.get_composition_sample_number(start, streamid);

			start = reader.get_decoding_time(sample_number, streamid);
		}

		Add(mp4, reader, start, end);
	}
	

	void End()
	{
		get_mux().end();
	}

	virtual ~CMP4Edit_()
	{
	}

	void set_stream_time_scale(uint64_t time_scale, bool audio = true, int stream = 0)
	{
		if(audio)
			_info.audio_info[stream].time_scale = time_scale;
		else
			_info.video_info[stream].time_scale = time_scale;
	}
};


class CMP4Edit: public CMP4Edit_
{
	MP4Mux  _mp4mux;
	

protected:
	
	virtual IMP4Mux4 & get_mux(){ return _mp4mux; }
	virtual void set_ctts_offset_(bool rhs){_mp4mux.set_ctts_offset(rhs);}

public:

	CMP4Edit(::mg::uv::loopthread & loop):_mp4mux(loop)
	{}
	CMP4Edit()
	{}

	virtual void set_max_distance(uint64_t max_distance){get_mux().set_max_distance(max_distance);}
	virtual void set_use_composition_in_distance(bool rhs){get_mux().set_use_composition_in_distance(rhs);}
	virtual void set_aac_audio_fix(bool rhs){get_mux().set_aac_audio_fix(rhs);}
};


class CMP4EditConsole: public CMP4Edit
{
	void output_composition(const TCHAR * pmsg, const int stream_id, stream_edit_info * ps) const
	{
		std::wcout 
				
				   << pmsg
			       << _T("\t")
			       << stream_id

				   << _T(" start comp\t")
				   << HNS(ps->next_composition)
				   << _T(" start dur\t")
				   << HNS(ps->next_duration)
				   << _T(" start dec\t")
				   << HNS(ps->next_decoding)
				   << _T(" prev comp\t")
				   << HNS(ps->stream_composition)
				   << _T(" prev dur \t")
				   << HNS(ps->stream_duration)
				   << _T(" prev dec\t")
				   << HNS(ps->stream_decoding)

				   << std::endl;

				  //<< L"last_composition\t"
				  //<< HNS(ps->last_composition)
				  //<< L"last_duration\t"
				  //<< HNS(ps->last_duration)
				  //<< L"last_decoding\t"
				  //<< HNS(ps->last_decoding)
	}

protected:
	virtual void info_input_stream(const MP4Reader &reader) const
	{
		for(int i = 0; i < ST_I32(reader.stream_count()); i++)
		{		
				std::wcout << i
				<< L") offset: "
				<< HNS(reader.get_stream_offset(i))
				<< L" duration: " 
				<< HNS(reader.get_duration(i))
				<< L" sample duration: "
				<< HNS(reader.get_sample_duration(i))
				<< L" ["
				<< reader.get_sample_duration(i)
				<< L"]"
				<< L" samples count: "
				<< reader.get_sample_count(i)
				<< std::endl;
		}
	}

	virtual void info_process_sample(const sample_stream_info & ms
		, uint64_t composition
		, uint64_t decoding
		, uint64_t stream_offset
		, uint64_t total_stream_samples) const 
	{
		if(ms.sample_number < 5 
				|| (total_stream_samples - ms.sample_number) < 5)
			{
				std::wcout << ms.stream
				<< _T("\t")
				<< ms.sample_number
				<< _T("\t")
				<< HNS(ms.composition_time + stream_offset)
				<< _T("\t")
				<< HNS(composition)
				<< _T("\t")
				<< HNS(ms.decoding_time)
				<< _T("\t")
				<< HNS(decoding)
				<< _T("\t")
				<< HNS(ms.duration)
				<< _T("\t")
				<< ms.bIsSyncPoint
				<< _T("\t")
				<< ms.offset
				<< _T("\t")
				<< ms.size
				<< std::endl;
			}
	}

	
	
	virtual void info_pre_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		output_composition(_T("PRE "), stream_id, ps);
	}

	virtual void info_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		output_composition(_T("POST "), stream_id, ps);
	}
public:
	virtual ~CMP4EditConsole(){}

	CMP4EditConsole(::mg::uv::loopthread & loop) :CMP4Edit(loop)
	{}

	CMP4EditConsole() 
	{}
};


__ALX_END_NAMESPACE

