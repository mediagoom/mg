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
//#include <intsafe.h>
#include "mp4edit.h"

class CMP4EmptyMux: public IMP4Mux2
{
	int _streams;
	int _ltcs;

	std::map<int, uint64_t> _stream_end;
	std::map<int, uint64_t> _stream_end_duration;
public:


	CMP4EmptyMux()
		: _streams(0)
		, _ltcs(199)
	{
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
		int idx = _streams++;

		_stream_end[idx] = 0;
		_stream_end_duration[idx] = 0;

		return idx;
	}


	virtual int add_audio_stream(
	  const unsigned int object_type
	, const unsigned int sample_rate
    , const unsigned int channels
	, const unsigned int target_bit_rate
	, const uint64_t time_scale = 0)
    {
		int idx = _streams++;

		_stream_end[idx] = 0;
		_stream_end_duration[idx] = 0;

		return idx;

	}

	virtual int add_extension_ltc_stream( const unsigned int avg_frame_rate = 400000
		, const uint64_t time_scale = 0)
	{
		return _ltcs++;
	}
	
	virtual void start(const TCHAR * file_out)
	{

	}

	virtual void set_stream_optional(
		  const int stream_id
		, const TCHAR*   lang
		, const DWORD	 bitrate
		, const uint64_t idr
		) 
	{

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
		
		if(_stream_end[stream_id] < composition_time)
		{
			_stream_end[stream_id] = composition_time;
			_stream_end_duration[stream_id] = duration;
		}
		
	}
	
	virtual void end()
	{
	}

	virtual void set_auto_decoding_time(const int stream_id, const bool rhs)
	{
	}

	uint64_t get_stream_end(const int stream_id) 
	{
		_ASSERTE(stream_id < _streams);
		return _stream_end[stream_id] + _stream_end_duration[stream_id];
	}

};

struct dynamic_item
{
	uint64_t bitrate;
	Cstring psz_path;
	//std::vector<uint64_t> audio_bitrate;
	uint64_t audio_bitrate;
	//TODO: add languages
	
		unsigned int   langs[16];
		unsigned short lang_count;
	

	//dynamic_item():audio_bitrate(96000){}
};

#define MAX_STREAM 64

class CMP4Dynamic :
	public CMP4EditBase
	, public CMP4EmptyMux
{
public:
	CMP4Dynamic(void);
	virtual ~CMP4Dynamic(void);


	edit_info     _info;
	uint64_t _current_end;

	uint64_t _stream_start_offset[MAX_STREAM];

	std::map<int, uint64_t> _stream_composition_end;

protected:

	virtual void info_composition_info2(const int stream_id, stream_edit_info * ps
		, uint64_t composition, uint64_t decoding)
	{
		
		if(0 == _info.file_count)
		{
			_stream_start_offset[stream_id] = composition - ps->stream_composition;
		}

		_stream_composition_end[stream_id] = 0;
		
	}
/*
	virtual void info_composition_info(const int stream_id, stream_edit_info * ps) const 
	{
		
		if(0 == _info.file_count)
		{
			const_cast<CMP4Dynamic*>(this)->_stream_start_offset[stream_id] = ps->next_composition - ps->next_decoding;
		}

		const_cast<CMP4Dynamic*>(this)->_stream_composition_end[stream_id] = 0;
		
	}
*/

	virtual void info_process_sample(const sample_stream_info & ms
		, uint64_t composition
		, uint64_t decoding
		, uint64_t stream_offset
		, uint64_t total_stream_samples) const 
	{
		std::map<int, uint64_t> & stream_composition_end = *(const_cast<std::map<int, uint64_t> *>(&_stream_composition_end));
		if(stream_composition_end[ms.stream] < static_cast<uint64_t>(ms.composition_time))
		{
			stream_composition_end[ms.stream] = ms.composition_time + ms.duration;
		}
	}

	virtual void compute_video_private_data( 
		  const BYTE*    sps_nal_source
        , const unsigned sps_nal_size
		, const BYTE*	 pps_nal_source
        , const unsigned pps_nal_size
		, BYTE *out //must be at least extra-size
		)
	{
		DWORD extrasize = sps_nal_size
			+ pps_nal_size
			+ 4;
	

			//ALX::CBuffer<BYTE> extra(extrasize);
			

			//{

			//BYTE *out = extra.get();

			out[0] = sps_nal_size >> 8 & 0xFF;
			out[1] = sps_nal_size & 0xFF;

			memcpy(out+ 2, sps_nal_source, sps_nal_size);

			out[sps_nal_size + 2] = pps_nal_size >> 8 & 0xFF;
			out[sps_nal_size + 3] = pps_nal_size & 0xFF;

			memcpy(out + sps_nal_size + 4, pps_nal_source, pps_nal_size);
	}

	virtual uint64_t computed_time(uint64_t composition_time, int stream, uint64_t stream_offset)
	{
		stream_edit_info * ps(0);

		if(_info.map[stream].video)
		{
			ps = &_info.video_info[_info.map[stream].internal_index];
		}
		else
		{
			ps = &_info.audio_info[_info.map[stream].internal_index];
		}
		
		//uint64_t stream_offset = reader.get_stream_offset(stream);

		return ps->stream_composition + (composition_time + stream_offset - ps->next_composition) + _stream_start_offset[stream];
		
	}

    
	virtual void publish_composition(uint64_t composition_time,
		uint64_t computed_time,
		int stream)
	{
	}

	virtual void publish_no_random_stream(
		int stream, const MP4Reader &reader, uint64_t start, uint64_t end)
	{

	}

	virtual void video_private_data(const unsigned char* p_private_data, long size, int stream
		, const unsigned int width
        , const unsigned int height)
	{

	}

	virtual void audio_private_data(const unsigned char* p_private_data, long size, int stream
		, const unsigned int sample_rate
		, const unsigned int channels
		, const unsigned int target_bit_rate)
	{

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
		);
	


	virtual int add_audio_stream(
	  const unsigned int object_type
	, const unsigned int sample_rate
    , const unsigned int channels
	, const unsigned int target_bit_rate
	, const uint64_t time_scale = 0);
    

	virtual void move_to_read(  uint64_t start
			, MP4Reader &reader
			);

	virtual void begin_processing(dynamic_item * p_dynamic_items, int dynamic_items_size
		, int streams, MP4Reader &reader)
	{
	}

	virtual void end_processing()
	{
	}

	virtual void begin_stream(int stream){}
	virtual void end_stream(){}
public:

    

    void Add(uint64_t start, uint64_t end 
		, dynamic_item * p_dynamic_items, int dynamic_items_size
		);

	void Add(/*MP4File*/CMP4 & mp4, MP4Reader & reader, uint64_t start, uint64_t end
		, dynamic_item * p_dynamic_items, int dynamic_items_size);
	

	virtual void End()
	{
	}

	uint64_t get_stream_composition_end(int stream_id)
	{
		return _stream_composition_end[stream_id];
	}


};


class CMP4DynamicDiscreate : public CMP4Dynamic
{
	uint64_t _fragment_length;
	uint64_t _last_fragment[64];
	uint64_t _fragment_tolerance;

	bool			 _generate_only_full_fragment; //if true we join an half fragment from one file to the following one
    uint64_t _reporting_start_time;
	uint64_t _reporting_computed_start_time;
	uint64_t				 _reporting_samples;
protected:
	
	virtual void begin_stream(int stream){_last_fragment[stream] = UINT64_MAX;}
	
    virtual void publish_composition(uint64_t composition_time,
		uint64_t computed_time,
		int stream);

	virtual void publish_no_random_stream(
		int stream, const MP4Reader &reader, uint64_t start, uint64_t end);

	/*
	virtual void video_private_data(const unsigned char* p_private_data, long size, int stream)
	{

	}

	virtual void audio_private_data(const unsigned char* p_private_data, long size, int stream)
	{

	}
	*/

	virtual void publish_segmented_composition(uint64_t composition_time,
		uint64_t computed_time,
		int stream)
	{
	}

	virtual void publish_segmented_composition_cross(uint64_t composition_time,
		uint64_t computed_time,
		uint64_t composition_time_2,
		uint64_t computed_time_2,
		int stream)
	{
	}

public:

	CMP4DynamicDiscreate()
		:_fragment_length(20000000)
		, _fragment_tolerance(260000) //less than a frame
		, _generate_only_full_fragment(true)
		, _reporting_start_time(UINT64_MAX)
	{}

	CMP4DynamicDiscreate(uint64_t fragment_length)
		:_fragment_length(fragment_length)
	{}

	void set_fragment_length(uint64_t fragment_length)
	{_fragment_length = fragment_length;}

	void set_fragment_tolerance(uint64_t fragment_tolerance)
	{_fragment_tolerance = fragment_tolerance;}


};

