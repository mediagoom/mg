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
//#include "StdAfx.h"
#include "mp4dynamic.h"

CMP4Dynamic::CMP4Dynamic(void)
{
	_info.file_count = 0;

	for(int i = 0; i < MAX_STREAM; i++)
		_stream_start_offset[0];
}

CMP4Dynamic::~CMP4Dynamic(void)
{
}

void CMP4Dynamic::move_to_read(  uint64_t start
			, MP4Reader &reader
			)
{
	bool only_audio = !reader.HasVisual();
	uint64_t last_composition = 0;

	uint64_t computed_start_time = UINT64_MAX;
    
    size_t stream_count =  reader.stream_count();

	for(size_t idx = 0; idx < stream_count; idx++)
	{
        if(!reader.IsValidStream(idx))
            continue;

		uint64_t t_composition    = UINT64_MAX;

		begin_stream(idx);

	    if(!reader.IsLTC(idx) && reader.has_random_access_point(idx))
		{
			//publish_stream
			//TODO: use decoding start instead of start
			uint64_t composition      = reader.get_composition_time(reader.get_IFrame_number(start, idx), idx);

			if(UINT64_MAX == computed_start_time)
				computed_start_time = composition;
			
			
			while(composition < _current_end
				&& (
						   composition > t_composition 
						|| UINT64_MAX == t_composition 
					)
				)
			{
				
				publish_composition(
				    composition
				  , computed_time(composition
				                 , idx
								 , reader.get_stream_offset(idx)
								 )
				  , idx
				);

				t_composition = composition;

				composition = reader.get_next_IFrame_time(idx);
			}

			///DONE? TODO: MUST PUBLISH FROM LAST COMPOSITION TO THE END ???
		}

		
		if(!reader.IsLTC(idx) && !reader.has_random_access_point(idx))
		{

			if(UINT64_MAX == computed_start_time) //we have not found an I frame stream yet look for it.
			{
				for(int idx = 0; idx < ST_I32(reader.stream_count()); idx++)
				{
					if(!reader.IsLTC(idx) && reader.has_random_access_point(idx))
					{
						computed_start_time = reader.get_composition_time(reader.get_IFrame_number(start, idx), idx);
						break;
					}
				}
			}

			if(UINT64_MAX == computed_start_time) //we do not have an I frame stream
			{
				computed_start_time = start;
			}


			publish_no_random_stream(idx, reader, computed_start_time, _current_end);
		}

		end_stream();

		if(reader.IsVisual(idx) || only_audio)
			last_composition = t_composition;
	}

	reader.move(last_composition);

}



int CMP4Dynamic::add_visual_stream
	(
		  const BYTE*    sps_nal_source
        , const unsigned sps_nal_size
		, const BYTE*	 pps_nal_source
        , const unsigned pps_nal_size
		, const uint64_t time_scale
		, const unsigned int width
        , const unsigned int height
		)
{

	int stream = -1;
	
	{

	DWORD extrasize = sps_nal_size
			+ pps_nal_size
			+ 4;
	

	ALX::CBuffer<BYTE> extra(extrasize);
	

	//{

	BYTE *out = extra.get();

	/*

	out[0] = sps_nal_size >> 8 & 0xFF;
	out[1] = sps_nal_size & 0xFF;

	memcpy(out+ 2, sps_nal_source, sps_nal_size);

	out[sps_nal_size + 2] = pps_nal_size >> 8 & 0xFF;
	out[sps_nal_size + 3] = pps_nal_size & 0xFF;

	memcpy(out + sps_nal_size + 4, pps_nal_source, pps_nal_size);

	*/

	compute_video_private_data(sps_nal_source
        , sps_nal_size
		, pps_nal_source
        , pps_nal_size
		, out);


	stream =  CMP4EmptyMux::add_visual_stream(
		  sps_nal_source
        , sps_nal_size
		, pps_nal_source
        , pps_nal_size
		, time_scale
		, width
        , height
		);

	video_private_data(out, extrasize, stream, width, height);

	}

	return stream;
}
	


int CMP4Dynamic::add_audio_stream(
	  const unsigned int object_type
	, const unsigned int sample_rate
    , const unsigned int channels
	, const unsigned int target_bit_rate
	, const uint64_t time_scale)
{

	aac_info_mp4 aac;

		aac.object_type = object_type;
		aac.sample_rate = sample_rate;
		aac.channels	= channels;

		const int aac_size(2);


	   write_memory_bitstream  mem(aac_size);

	   aac.put(mem);

	   mem.flush();



	int stream = CMP4EmptyMux::add_audio_stream(
	  object_type
	, sample_rate
    , channels
	, target_bit_rate
	, time_scale
	);

	   audio_private_data(mem.get_buffer(), aac_size, stream, sample_rate
			, channels
			, target_bit_rate);

	   return stream;
}

void CMP4Dynamic::Add(/*MP4File*/CMP4 & mp4, MP4Reader & reader, uint64_t start, uint64_t end
	, dynamic_item * p_dynamic_items, int dynamic_items_size)
{
	_current_end = end;


	_ASSERTE(0 < dynamic_items_size);

	if (0 == _current_end)
	{
		_current_end = reader.get_duration();
	}

	if (0 == _info.file_count)
	{
		do_edit_header_mux(
			reader
			, *this
			, _info);
	}

	//info_input_stream(reader);

	begin_processing(
		p_dynamic_items
		, dynamic_items_size
		, reader.stream_count()
		, reader);

	do_edit_mux(
		start
		, end
		, mp4
		, reader
		, *this
		, _info
		, false //we do not use samples
		, false
		);

	end_processing();

}

void CMP4Dynamic::Add(uint64_t start, uint64_t end
					  , dynamic_item * p_dynamic_items, int dynamic_items_size)
{
		


	SYNCMP4File mp4;
	mp4.open(p_dynamic_items[0].psz_path);

	MP4Reader reader;
	reader.parse(mp4);

	
	Add(mp4, reader, start, end, p_dynamic_items, dynamic_items_size);

}



void CMP4DynamicDiscreate::publish_no_random_stream(
		int stream, const MP4Reader &reader, uint64_t start, uint64_t end)
{
	
	uint64_t off = reader.get_stream_offset(stream);

	uint64_t start_time = start;

	if(start_time > off)
		start_time -= off;
	else
		start_time = 0;

	uint64_t p = reader.get_composition_sample_number(start_time, stream);
	uint64_t d = reader.get_sample_duration(stream);

	

	_ASSERTE(_fragment_length > d);

	//TODO: allow to use a predefined duration 
	//for old file with invalid audio duration
	uint64_t jump = _fragment_length / d;
	
	uint64_t composition(reader.get_decoding_time(p, stream));

	bool has_composition = reader.has_composition_time(stream);
	
	if(has_composition)
		composition =  reader.get_composition_time(p, stream);

	composition += off;

	while(composition < start)
	{
		if(has_composition)
			composition =  reader.get_composition_time(p, stream);
		else
		    composition = reader.get_decoding_time(p, stream);

		composition += off;

	}


	if(UINT64_MAX != _reporting_start_time 
			&& _generate_only_full_fragment)
	{
		_ASSERTE(jump > _reporting_samples);
		
		uint64_t fill_jump = jump - _reporting_samples;

		uint64_t t1 = computed_time(
			                          (composition - off) - (_reporting_samples * d)
									 , stream
									 , off
									 );

		//_ASSERTE(t1 >= _reporting_computed_start_time);

		t1 = _reporting_computed_start_time;


		publish_segmented_composition_cross( _reporting_start_time
			, t1
			, composition
			, computed_time(
			                 (composition - off)
							 , stream
							 , off
							 )
		, stream);
				
		
		
		p += fill_jump;

		if(has_composition)
			composition =  reader.get_composition_time(p, stream);
		else
		    composition = reader.get_decoding_time(p, stream);

		composition += off;


		_reporting_start_time = UINT64_MAX;
	}
		

	while(composition < end && p < reader.get_sample_count(stream))
	{
		publish_segmented_composition(
							  composition
							, computed_time(composition - off
									 , stream
									 , off
									 )
						    , stream);

		p += jump;

		if(p >= reader.get_sample_count(stream))
			break;

		if( (reader.get_sample_count(stream) - p) < jump  ) //last sample 
		{
			if(_generate_only_full_fragment)
			{
				_ASSERTE(UINT64_MAX == _reporting_start_time);
				 
				_reporting_samples    = reader.get_sample_count(stream) - p;

				uint64_t sn   = reader.get_sample_count(stream) - _reporting_samples;

				_reporting_start_time = reader.get_decoding_time(sn, stream);

				if(has_composition)
				{
					_reporting_start_time = reader.get_composition_time(sn, stream);
				}
		    				
				_reporting_computed_start_time = computed_time(_reporting_start_time
									 , stream
									 , off
									 );

				_reporting_start_time += off;
				
				//p = reader.get_sample_count(stream); //get off
				return;

				
			}
			else
			{
				p = reader.get_sample_count(stream) - 1;
			}
		}


		if(has_composition)
			composition =  reader.get_composition_time(p, stream);
		else
		    composition = reader.get_decoding_time(p, stream);

		composition += off;


	}

}



void CMP4DynamicDiscreate::publish_composition(uint64_t composition_time,
		uint64_t computed_time,
		int stream)
{
	if(UINT64_MAX == _last_fragment[stream] ||
		(_last_fragment[stream] - _fragment_tolerance) <= computed_time
	)
	{
		publish_segmented_composition(
							  composition_time
							, computed_time
						    , stream);

		_last_fragment[stream] = computed_time += _fragment_length;
	}
}

