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
#include "stdafx.h"
#include <mgcore.h>
#include "mp4edit.h"


void CMP4EditBase::move_to_read(  uint64_t start
			, MP4Reader &reader
			)
{
	reader.move(start);
}

void CMP4EditBase::do_edit_header_mux( MP4Reader &reader
					  , IMP4Mux2 & mp4w
					  , edit_info & info)
{
	
	
	info.audio_streams_count = 0;
	info.video_streams_count = 0;

	for(size_t i = 0; i < reader.stream_count(); i++)
	{

        if(!reader.IsValidStream(i))
            continue;
        
		_ASSERTE(1 == reader.entry_count(i));

		if(reader.IsVisual(i))
		{
			//std::wcout << L"Add Visual Stream" << std::endl;

			_ASSERTE(!reader.IsSound(i));
			_ASSERTE(!reader.IsLTC(i));

			/*mp4w.add_visual_stream(reader.get_visual_entry(0, i)
				, reader.get_media_time_scale(i));*/

			info.video_streams_count++;
			info.video_info[info.video_streams_count - 1].stream_id =
			mp4w.add_visual_stream(
				  reader.get_visual_entry(0, i).get_avcc_header().get_nal_sequence(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_sequence_size(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_nal_picture(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_picture_size(0)
				, (0 == info.video_info[info.video_streams_count - 1].time_scale)?reader.get_media_time_scale(i):info.video_info[info.video_streams_count - 1].time_scale
				, reader.get_visual_entry(0, i).get_entry().width
				, reader.get_visual_entry(0, i).get_entry().height
				);

			mp4w.set_auto_decoding_time(info.video_info[info.video_streams_count - 1].stream_id, reader.has_composition_time(i));

			info.video_info[info.video_streams_count - 1].set_info(
				  reader.get_visual_entry(0, i).get_avcc_header().get_nal_sequence(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_sequence_size(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_nal_picture(0)
				, reader.get_visual_entry(0, i).get_avcc_header().get_picture_size(0)
				);

			info.map[i].internal_index = info.video_streams_count - 1;
			info.map[i].video = true;

			//mp4w.set_ctts_offset(i, 50000000ULL);
		}
		else
		{
			if(reader.IsLTC(i))
			{
				info.ltc = i;

				mp4w.add_extension_ltc_stream(
					static_cast<uint32_t>(reader.LTC_avg_time(i))
					);	
				info.audio_streams_count++;
			}
			else
			{

				_ASSERTE(!reader.IsVisual(i));
				_ASSERTE(!reader.IsLTC(i));

				//std::wcout << L"Add Audio Stream" << std::endl;

				FixedMemoryBitstream  mem(
					  reader.get_audio_entry(0, i).get_body()
					, static_cast<uint32_t>(reader.get_audio_entry(0, i).get_body_size()));

				unsigned int box_size = mem.getbits(32);
				unsigned int box_type = mem.getbits(32);
				
				_ASSERTE(box_esds == box_type);

				mem.skipbits(32); //version flags

				ES_Descriptor es;
				es.get(mem);

				if(!es.p_decoder_config_descriptor->p_ext->get_length())
					ALXTHROW_T(_T("no audio decoder configuration"));
				
				FixedMemoryBitstream  maac(es.p_decoder_config_descriptor->p_ext->get_descriptor()
									, es.p_decoder_config_descriptor->p_ext->get_length()
									  );

				aac_info_mp4 aac;
				aac.get(maac);

				info.audio_streams_count++;
				info.audio_info[info.audio_streams_count - 1].stream_id =
				mp4w.add_audio_stream(
					  aac.object_type
					, aac.sample_rate
					, aac.channels
					, static_cast<uint32_t>(reader.get_stream_bit_rate(i))
					, (0 == info.audio_info[info.audio_streams_count - 1].time_scale)?reader.get_media_time_scale(i):info.audio_info[info.audio_streams_count - 1].time_scale
					);

				mp4w.set_auto_decoding_time(info.audio_info[info.audio_streams_count - 1].stream_id, reader.has_composition_time(i));


				mp4w.set_stream_optional(i
					, reader.get_language(i)
					, static_cast<uint32_t>(reader.get_stream_bit_rate(i))
					, 20000000
					);

				info.audio_info[info.audio_streams_count - 1].object_type = aac.object_type;
				info.audio_info[info.audio_streams_count - 1].sample_rate = aac.sample_rate;
				info.audio_info[info.audio_streams_count - 1].channels    = aac.channels;
				info.audio_info[info.audio_streams_count - 1].bit_rate    = static_cast<uint32_t>(reader.get_stream_bit_rate(i));
				}
				
				info.map[i].internal_index = info.audio_streams_count - 1;
				info.map[i].video = false;
			
		}


			//mp4w.set_ctts_offset(i, 5000000ULL);
		
	}

	

	//return 0;
}



void CMP4EditBase::do_edit_mux(
			   int64_t start
			,  int64_t end
			, CMP4 &mp4
			, MP4Reader &reader
			, IMP4Mux2 & body     //<----IMP4Mux
			, edit_info & info
			, bool physical
			, bool discard_pre_start)
{
	sample_stream_info ms;

	ms.composition_time = 0;

	stream_edit_info * ps(0);
		
	//are the file compatibles?

	if(reader.parsable_stream_count() != (info.audio_streams_count + info.video_streams_count))
		ALXTHROW_T(_T("The files do not have an equal number of streams"));

	int video_stream(0);
	int audio_stream(0);

	if(reader.HasLTC() && info.ltc < 0)
		ALXTHROW_T(_T("The profile does not have LTC"));

	if(!reader.HasLTC() && info.ltc > -1)
		ALXTHROW_T(_T("The file does not have LTC"));

	for(int i = 0; i < ST_I32(reader.stream_count()); i++)
	{
        if(!reader.IsValidStream(i))
            continue;

		//if(!reader.IsLTC(i))
		//{
			if(info.map[i].video)
			{
				info.video_info[info.map[i].internal_index].next_composition = INT64_MAX;
				info.video_info[info.map[i].internal_index].next_duration    = UINT64_MAX;
				info.video_info[info.map[i].internal_index].next_decoding    = UINT64_MAX;
			}
			else
			{
				info.audio_info[info.map[i].internal_index].next_composition = INT64_MAX;
				info.audio_info[info.map[i].internal_index].next_duration    = UINT64_MAX;
				info.audio_info[info.map[i].internal_index].next_decoding    = UINT64_MAX;
			}


			if(reader.IsVisual(i))
			{
				if(i != info.video_info[video_stream].stream_id)
					ALXTHROW_T(_T("The files do not have the same streams mapping"));

				if(info.video_info[video_stream].sequence_size != 
					reader.get_visual_entry(0, i).get_avcc_header().get_sequence_size()
					)
				{
					ALXTHROW_T(_T("The files do not have the same sequence size"));
				}

				if(memcmp(
					   reader.get_visual_entry(0, i).get_avcc_header().get_nal_sequence()
					 , info.video_info[video_stream].p_sequence
					 , info.video_info[video_stream].sequence_size)
					 )
				{
					ALXTHROW_T(_T("The files do not have the same sequence"));
				}

				if(info.video_info[video_stream].sequence_size != 
					reader.get_visual_entry(0, i).get_avcc_header().get_sequence_size()
					)
				{
					ALXTHROW_T(_T("The files do not have the same sequence size"));
				}

				if(memcmp(
					   reader.get_visual_entry(0, i).get_avcc_header().get_nal_picture()
					 , info.video_info[video_stream].p_picture
					 , info.video_info[video_stream].picture_size)
					 )
				{
					ALXTHROW_T(_T("The files do not have the same picture"));
				}

				video_stream++;
			}
			else
			{
				if(reader.IsLTC(i))
				{
					if(info.ltc != i)
						ALXTHROW_T(_T("The files do not have the same LTC streams mapping"));
				}
				else
				{

					if(i != info.audio_info[audio_stream].stream_id)
						ALXTHROW_T(_T("The files do not have the same audio streams mapping"));

					if(info.audio_info[audio_stream].channels != 
						reader.get_audio_entry(0, i).get_aac_info().channels)
					{
						ALXTHROW_T(_T("The files do not have the same audio channels"));

					}

					if(info.audio_info[audio_stream].object_type != 
						reader.get_audio_entry(0, i).get_aac_info().object_type)
					{
						ALXTHROW_T(_T("The files do not have the same audio object_type"));

					}

					if(info.audio_info[audio_stream].sample_rate != 
						reader.get_audio_entry(0, i).get_aac_info().sample_rate)
					{
						ALXTHROW_T(_T("The files do not have the same audio sample_rate"));

					}

				}//LTC audio

				audio_stream++;

			}
		//}//LTC
	}

		bool work = true;
    
		int current_look_head = 0;

		reader.move(start);

		info.ltc_interpolated = false;

		while(work)
		{
			
			if(!reader.next_file_chunks(ms))
				ALXTHROW_T(_T("cannot check first sample info."));

            if ((ms.composition_time + static_cast<int64_t>(reader.get_stream_offset(ms.stream))) < start && discard_pre_start) //<-- loose gop position
            {
                if (reader.has_random_access_point(ms.stream))
                {
                    _ASSERTE(false);
                    ALXTHROW_T(_T("CANNOT DISCARD RANDOM ACCESS POINT STREAM. WOULD LOOSE STARTING GOP"));
                }

                continue;
            }
	        
			work = false;

			uint64_t stream_offset = reader.get_stream_offset(ms.stream);

			if(stream_offset)
			{
				ms.composition_time += stream_offset;
				/*ms.decoding_time    += stream_offset*/;
			}
			

				if(info.map[ms.stream].video)
				{
					if(info.video_info[info.map[ms.stream].internal_index].next_composition > ms.composition_time)
					{
						info.video_info[info.map[ms.stream].internal_index].next_composition = ms.composition_time;
						info.video_info[info.map[ms.stream].internal_index].next_duration    = ms.duration;
						
						info.video_info[info.map[ms.stream].internal_index].next_decoding    = ms.decoding_time;
						work = true;
					}
				}
				else
				{
					if(info.audio_info[info.map[ms.stream].internal_index].next_composition > ms.composition_time)
					{
						if(ms.stream != info.ltc || ms.composition_time >= start) //avoid ltc as firts stream
						{
							info.audio_info[info.map[ms.stream].internal_index].next_composition = ms.composition_time;
							info.audio_info[info.map[ms.stream].internal_index].next_duration    = ms.duration;
							info.audio_info[info.map[ms.stream].internal_index].next_decoding    = ms.decoding_time;
						}
						
						work = true;
					}
				}

			

			if(!work)
			{
				current_look_head++;
				
				for(size_t i = 0; i < reader.stream_count(); i++)
				{
                    if(!reader.IsValidStream(i))
                        continue;

					if(i != reader.LTCStream())
					{
						if(info.map[i].video)
						{
							if(info.video_info[info.map[i].internal_index].next_composition == INT64_MAX ||
								current_look_head < _look_head)
								work = true;
						}
						else
						{
							if(info.audio_info[info.map[i].internal_index].next_composition == INT64_MAX||
								current_look_head < _look_head)
								work = true;
						}
					}
				}
			}
			else
				current_look_head = 0;

		}//while work


		

		if(-1 < info.ltc 
			&&
			info.audio_info[info.map[info.ltc].internal_index].next_composition == UINT64_MAX)
		{
			info.audio_info[info.map[info.ltc].internal_index].next_composition = info.video_info[0].next_composition;
			info.audio_info[info.map[info.ltc].internal_index].next_decoding    = info.video_info[0].next_decoding;
			info.ltc_interpolated = true;
		}

#ifdef _DEBUG

		for(size_t i = 0; i < reader.stream_count(); i++)
		{
            if(!reader.IsValidStream(i))
                continue;

			if(i != reader.LTCStream())
			{
				if(info.map[i].video)
				{
					ps = &info.video_info[info.map[i].internal_index];
				}
				else
				{
					ps = &info.audio_info[info.map[i].internal_index];
				}
				
				_ASSERTE( 
						(ps->next_composition == (reader.get_start_time(i) + reader.get_stream_offset(i)))
						|| start > 0
					);
				_ASSERTE( 
						(ps->next_decoding    == reader.get_decoding_start_time(i))
						|| start >0					
					);

			}
		}

#endif


		CMediaJoin composition;
		CMediaJoin decoding;
		CMediaJoin decoding_composition;

		size_t stream_count = reader.parsable_stream_count() - ((-1 < info.ltc)?1:0);

				 composition.SetStreamCount(stream_count);
				    decoding.SetStreamCount(stream_count);
	    decoding_composition.SetStreamCount(stream_count * 2);

		for(size_t i = 0; i < reader.stream_count(); i++)
		{
            if(!reader.IsValidStream(i))
                continue;

			if(i != reader.LTCStream())
			{
				if(info.map[i].video)
				{
					ps = &info.video_info[info.map[i].internal_index];
				}
				else
				{
					ps = &info.audio_info[info.map[i].internal_index];
				}

				if(0 == info.file_count)
				{
					ps->stream_composition = 0;
					ps->stream_duration    = 0;
					ps->stream_decoding    = 0;

					//if(info.map[i].video)
					//	ps->stream_composition = ps->next_composition;
				}
				
				composition.AddStreamEnd(ps->stream_composition + ps->stream_duration);
				composition.AddStreamStart(ps->next_composition);
				decoding.AddStreamEnd(ps->stream_decoding + ps->stream_duration);
				decoding.AddStreamStart(ps->next_decoding);

				decoding_composition.AddStreamEnd(ps->stream_composition + ps->stream_duration);
				decoding_composition.AddStreamStart(ps->next_composition);
				decoding_composition.AddStreamEnd(ps->stream_decoding + ps->stream_duration);
				decoding_composition.AddStreamStart(ps->next_decoding);

			}
		}

		composition.Compute();
		decoding.Compute();
		decoding_composition.Compute();

		int64_t ltc_at_least(0);

		for(size_t i = 0; i < reader.stream_count(); i++)
		{
            if(!reader.IsValidStream(i))
                continue;

			stream_edit_info * ps(0);

			
				if(info.map[i].video)
				{
					ps = &info.video_info[info.map[i].internal_index];
				}
				else
				{
					ps = &info.audio_info[info.map[i].internal_index];
				}

				info_pre_composition_info(i, ps);

				if(i != reader.LTCStream())
			    {
					ps->stream_composition = composition.GetJoinTime(i);
					ps->stream_decoding    = decoding.GetJoinTime(i);
				}
				else
				{
					stream_edit_info * base_info(0);

					if(info.map[0].video)
					{
						base_info = &info.video_info[info.map[i].internal_index];
					}
					else
					{
						base_info = &info.audio_info[info.map[i].internal_index];
					}

					
					if(base_info->next_composition > ps->next_composition)
					{
						info.ltc_interpolated = true;
						ltc_at_least = base_info->next_composition;
					}

					int64_t offset = ps->next_composition - base_info->next_composition;

					_ASSERTE(composition.GetJoinTime(0) >= offset || reader.LTCStream());
					_ASSERTE(decoding.GetJoinTime(0)    >= offset || reader.LTCStream());

					if(composition.GetJoinTime(0) >= offset)
						ps->stream_composition = composition.GetJoinTime(0) - offset;
					else
						ps->stream_composition = 0;

					if(decoding.GetJoinTime(0)    >= offset)
						ps->stream_decoding    = decoding.GetJoinTime(0) - offset;
					else
						ps->stream_decoding    = 0;

					

					//ps->next_composition   = info.map[0].internal_index
				}



				info_composition_info(i, ps);
				if(i != reader.LTCStream())
				{
					_ASSERTE(ps->stream_decoding == decoding_composition.GetJoinTime((i * 2) + 1));
					info_composition_info2(i, ps
						, decoding_composition.GetJoinTime(i * 2)
						, decoding_composition.GetJoinTime((i * 2) + 1)
					);
				}
			

			//_ASSERTE(0 < info.file_count || 0 == ps->last_composition); 
		}
	

	std::vector<bool> streams(reader.stream_count());
	
	for(size_t i = 0; i < streams.size(); i++)
		streams[i] = true;

	ALX::wmt_timecode interpolated;

	if(info.ltc_interpolated)
	{
		interpolated = reader.get_interpolated_time_code(info.audio_info[info.map[info.ltc].internal_index].next_composition, mp4);
	}

	////////////////////////
	//reader.move(start);
	move_to_read(start, reader);
	////////////////////////

	work = true;

	CBuffer<BYTE> sample(1024);

	/*keep track of the monotone ending*/
    uint64_t last_composition[MAX_STREAMS], last_duration[MAX_STREAMS];//, last_decoding(0);

	for(int i = 0; i < MAX_STREAMS; i++)
	{
		last_composition[i] = 0;
		last_duration[i]    = 0;
	}

	while(work)
	{
		if(!reader.next_file_chunks(ms))
			break;

		if(0 < end)
		{
			streams[ms.stream] = (ms.decoding_time < end);
            work = false;
			for(uint32_t i = 0; i < streams.size(); i++)
				work |= streams[i];
		}

		
		if( (ms.composition_time + static_cast<int64_t>(reader.get_stream_offset(ms.stream))) < start && discard_pre_start) //<-- loose gop position
		{
			if(reader.has_random_access_point(ms.stream))
			{
				_ASSERTE(false);
				ALXTHROW_T(_T("CANNOT DISCARD RANDOM ACCESS POINT STREAM. WOULD LOOSE STARTING GOP"));
			}

			continue;
		}
		

		if(end > 0 && (ms.composition_time + static_cast<int64_t>(reader.get_stream_offset(ms.stream))) >= end)
			continue;

		sample.prepare(static_cast<uint32_t>(ms.size));
		
		if(physical)
		{
			mp4.set_position(ms.offset);
			mp4.read_bytes(sample.get(), ms.size);
		}
		/*else
		{
			_ASSERTE(false);
		}
		*/
		
		if(info.map[ms.stream].video)
		{
			ps = &info.video_info[info.map[ms.stream].internal_index];
		}
		else
		{
			ps = &info.audio_info[info.map[ms.stream].internal_index];
		}

		
		uint64_t stream_offset = reader.get_stream_offset(ms.stream);

        
        //check next sample is not less than one second away
        _ASSERTE(
            10000000ULL + ps->stream_composition + (ms.composition_time + stream_offset - ps->next_composition) >= ps->last_composition
        );
        

		ps->last_composition = ps->stream_composition + (ms.composition_time + stream_offset - ps->next_composition);
		ps->last_decoding    = ps->stream_decoding    + (ms.decoding_time    + stream_offset - ps->next_decoding);

		if(!reader.has_composition_time(ms.stream))
			ps->last_decoding = ps->last_composition;


		_ASSERTE(reader.get_sample_count(ms.stream) > ms.sample_number);

		info_process_sample(ms
			, ps->last_composition
			, ps->last_decoding
			, reader.get_stream_offset(ms.stream)
			, reader.get_sample_count(ms.stream));


		if( -1 < info.ltc &&
			info.ltc_interpolated && ms.stream == info.video_info[0].stream_id)
		{
			WriteMemoryBitstream mem(sizeof(wmt_timecode));

			interpolated.put(mem);

			body.add_sample(info.ltc
				, mem.get_buffer()
				, 14
				, true
				, ps->last_composition
				, ps->last_composition   //, ps->last_decoding
				, reader.LTC_avg_time(info.ltc)
				);			

			info.ltc_interpolated = false;
		}

		/*
		if(ms.stream != info.ltc 
			|| ( ms.composition_time >= start && ltc_at_least <= ms.composition_time)
		  ) //avoid ltc as firts stream or ltc stream offsetting
			body.add_sample(ms.stream
				, sample.get()
				, ms.size
				, ms.bIsSyncPoint
				, ps->last_composition  //ms.composition_time
				, ps->last_decoding     //ms.decoding_time
				, ms.duration
				);
				*/


		if(ms.stream != info.ltc 
			|| ( ms.composition_time >= start && ltc_at_least <= ms.composition_time)
		  ) //avoid ltc as firts stream or ltc stream offsetting
		body_add_sample(
				  body
				, ms.stream
				, sample.get()
				, static_cast<uint32_t>(ms.size)
				, ms.bIsSyncPoint
				, ps->last_composition  //ms.composition_time
				, ps->last_decoding     //ms.decoding_time
				, ms.duration
				);
		

		if(ms.duration)
			ps->last_duration = ms.duration;
		else
			ps->last_duration = reader.get_sample_duration(ms.stream);
			

		if(last_composition[ms.stream] < ps->last_composition)
		{
			last_composition[ms.stream] = ps->last_composition;
			last_duration[ms.stream]    = ps->last_duration;
		}
			
		//if(last_decoding < ps->last_decoding

		

	}//while(work)

		info.file_count++;

		for(int i = 0; i < ST_I32(reader.stream_count()); i++)
		{
            if(!reader.IsValidStream(i))
                continue;

			if(info.map[i].video)
			{
				ps = &info.video_info[info.map[i].internal_index];
			}
			else
			{
				ps = &info.audio_info[info.map[i].internal_index];
			}

			//THIS WILL BE USED IN NEXT JOIN
			/*
			ps->stream_duration    = ps->last_duration;
			ps->stream_composition = ps->last_composition;
			*/

			ps->stream_duration    = last_duration[i];
			ps->stream_composition = last_composition[i];
			
			ps->stream_decoding    = ps->last_decoding;
		
		}
}

