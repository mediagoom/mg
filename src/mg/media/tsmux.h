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

#include "mp4edit.h"
#include "tswrite.h"
#include <vector>

#ifndef STREAM_TYPE_AUDIO_AAC
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_H264      0x1b
#endif

#define BASEPID  0x100

#define DBGCTSMUXINFO 1

class TSMuxStream:public CPESWrite
{

	int _pid;

public:
	virtual void add_sample(
		  const BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, CTSWrite & tswrite
		, CTSW & TSW
		) = 0;

	void set_pid(int pid){_pid = pid;}
	int  get_pid(){return _pid;}

	virtual ~TSMuxStream(){}

	virtual void set_pcr() { } 
	
};


class TSEmptyStream:public TSMuxStream
{
public:
	virtual void add_sample(
		  const BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, CTSWrite & tswrite
		, CTSW & TSW
		){}
};

class TSMuxH264Stream:public TSMuxStream
{
	CTSWriteMemory _header;
	CTSWriteMemory _body;
	CTSWriteMemory _pes;

	unsigned int _header_start_position;

	unsigned int _nal_lengthSize;

	void write_nal_9(CTSWriteMemory & m)
	{

		//AV_WB32(data, 0x00000001);
        //    data[4] = 0x09;
        //    data[5] = 0xf0;

		m.write_uint(0x00000001);
		m.write_byte(0x09);
		m.write_byte(0xf0);

		//6 byte header for all nal
	}

	void init(const BYTE*    sps_nal_source
            , const unsigned sps_nal_size
		    , const BYTE*	 pps_nal_source
            , const unsigned pps_nal_size
		)
	{

		

		_header.set_position(0);
		_body.set_position(0);

		write_nal_9(_header);
		write_nal_9(_body);

		_header.write_uint(0x00000001);
		_header.write_bytes(sps_nal_source, sps_nal_size);
		_header.write_uint(0x00000001);
		_header.write_bytes(pps_nal_source, pps_nal_size);

		_header.flush();
		_body.flush();

		_header_start_position = static_cast<uint32_t>(_header.get_position());

		_ASSERTE(6 == _body.get_position());

	}

	const BYTE * process_nal( 
		  const BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		)
	{
		unsigned int nal_size(0);

		uint32_t x(0);

		for(x = 0; x < _nal_lengthSize; x++)
		{
			nal_size = (nal_size << 8) + body[x];
		}

		_ASSERTE(nal_size <= body_size);
		
		CTSWriteMemory * m = &_header;

		if(!IFrame)
		{
			m = &_body;
		}
		
		m->write_uint(0x00000001);
		m->write_bytes(body + _nal_lengthSize, nal_size);

		return body + _nal_lengthSize + nal_size;
	}

public:
	virtual void add_sample(
		  const BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, CTSWrite & tswrite
		, CTSW & TSW
		)
	{
		const BYTE * start = body;
		const BYTE * end   = body + body_size;

		if(IFrame)
			_header.set_position(_header_start_position);
		else
			_body.set_position(6);

		while(start < end)
		{
			start =  process_nal(start, ST_U32(end - start), IFrame, composition_time, decoding_time);
		}

		_header.flush();
		_body.flush();

		_pes.set_position(0);

		uint64_t PCR = UINT64_MAX;

		if(IFrame)
		{
			WritePesHeader(224
			, composition_time, decoding_time, UINT64_MAX
				, U64_U32(_header.get_position())
				, _pes);

			PCR = decoding_time;

			_pes.write_bytes(_header.get_buffer()
				, U64_U32(_header.get_position())
			);
		}
		else
		{
			WritePesHeader(224
			, composition_time, decoding_time, UINT64_MAX, 
				U64_U32(_body.get_position())
				, _pes);

			_pes.write_bytes(_body.get_buffer()
				, U64_U32(_body.get_position())
			);
		}

		uint64_t size = _pes.get_position();

		_pes.flush();

		tswrite.output_ts(get_pid(), _pes.get_buffer()
			, U64_U32(size)
			, TSW, IFrame, PCR); 
	}
public:
	

	TSMuxH264Stream(const BYTE*    sps_nal_source
            , const unsigned sps_nal_size
		    , const BYTE*	 pps_nal_source
            , const unsigned pps_nal_size
			):_nal_lengthSize(4)
	{

		//_pes.open(1024);
		//_header.open(1024);
		//_body.open(1024);

		init(sps_nal_source
            ,sps_nal_size
		    , pps_nal_source
            , pps_nal_size
		);
	}

	virtual ~TSMuxH264Stream()
	{
	}

};

class TSMuxAACStream:public TSMuxStream
{
	ADTS           _adts;
	CTSWriteMemory _body;
	CTSWriteMemory _pes;

	uint64_t        _body_presentation_time;

	int _pes_frames;
	int _pes_current_frame;

	bool     _pcr_output;
	int64_t  _pcr_distance;
	int      _pcr_count;

public:

	TSMuxAACStream(
	  const unsigned int object_type
	, const unsigned int sample_rate
    , const unsigned int channels
	, const unsigned int target_bit_rate
	):_pes_frames(1), _pes_current_frame(0), _pcr_output(false), _pcr_distance(20000000), _pcr_count(-1)
	{
		//_body.open(1024);
		//_pes.open(1024);
		
		_adts.set_mpeg_4_audio_object(object_type);
		_adts.mpeg_4_sampling_frequency = sample_rate;
		_adts.channel_configuration     = channels;

		_adts.mpeg_version = 0;
		_adts.buffer_fullnes = 0x7FF;
	
	}

	virtual void add_sample(
		  const BYTE * body
		, const unsigned int body_size
		, bool IFrame
		, uint64_t composition_time
		, uint64_t decoding_time
		, CTSWrite & tswrite
		, CTSW & TSW
		)
	{
		_adts.set_body_length(body_size);

		if(!_pes_current_frame)
			_body_presentation_time = composition_time;

		_body.put(_adts);

		_body.flush();

		_body.write(body, body_size);

		_body.flush();

		uint64_t PCR = UINT64_MAX;
		

		if(_pcr_output) 
		{
			//PCR = _body_presentation_time;
			PCR = decoding_time;
			_pcr_count++;
		}

		if(++_pes_current_frame >= _pes_frames)
		{
			_pes.set_position(0);

			WritePesHeader(192
			, _body_presentation_time
			, (_pcr_output)?decoding_time:UINT64_MAX
				, UINT64_MAX, U64_U32(_body.get_position()), _pes);

			_pes.write_bytes(_body.get_buffer()
				, static_cast<uint32_t>(_body.get_position())
			);

			uint64_t size = _pes.get_position();

			_pes.flush();

			tswrite.output_ts(get_pid(), _pes.get_buffer()
				, static_cast<uint32_t>(size)
				, TSW, IFrame, PCR);

			_body.set_position(0);
			_pes_current_frame = 0;
		}


	}

	virtual void set_pcr(){_pcr_output = true;}

	virtual ~TSMuxAACStream()
	{}
};

class TSMux: public IMP4Mux2
{
	CTSW * _pctsw;

	std::vector<TSMuxStream *> _streams;

	CTSWrite _tswrite;

	int _repeat_pid;

	bool _use_memory;
	bool _only_table_at_beginning;

	bool _is_first_sample;

	//bool _only_audio;
	
	uint64_t _presentation_offset;
	uint64_t _decoding_offset;

	void end_processesing()
	{

		if(_pctsw)
		{
			_pctsw->flush();
			_pctsw->close();
			delete _pctsw;
		}

		_pctsw = NULL;

		for(uint32_t i = 0; i < _streams.size(); i++)
			delete _streams[i];

		_streams.clear();

		
	}
public:

	TSMux():_pctsw(NULL), _repeat_pid(-1)
		, _presentation_offset(2000000)
		, _decoding_offset(0)
		, _use_memory(false)
		, _only_table_at_beginning(false)
		, _is_first_sample(true)
		//, _only_audio(false)
	{
	}

	virtual int add_visual_stream
	(	  const BYTE*    sps_nal_source
        , const unsigned sps_nal_size
		, const BYTE*	 pps_nal_source
        , const unsigned pps_nal_size
		, const uint64_t time_scale = 0
		, const unsigned int width  = 0
        , const unsigned int height = 0
		)
	{
		TSMuxH264Stream * p = new TSMuxH264Stream(sps_nal_source
            , sps_nal_size
		    , pps_nal_source
            , pps_nal_size);

		_streams.push_back(p);

		int stream_idx = _streams.size() -1;

		p->set_pid(BASEPID + stream_idx);
		

		_tswrite.add_pam(p->get_pid(), STREAM_TYPE_VIDEO_H264, stream_idx);

		if(-1 == _repeat_pid)
		{
			_repeat_pid = stream_idx;
			_tswrite.set_pcr_pid(p->get_pid());
		}
		
		return stream_idx;
	}


	virtual int add_audio_stream(
	  const unsigned int object_type
	, const unsigned int sample_rate
    , const unsigned int channels
	, const unsigned int target_bit_rate
	, const uint64_t time_scale = 0)
	{
		TSMuxAACStream * p = new TSMuxAACStream(
				  object_type
				, sample_rate
				, channels
				, target_bit_rate);


			_streams.push_back(p);

		int stream_idx = _streams.size() -1;

		p->set_pid(BASEPID + stream_idx);

		_tswrite.add_pam(p->get_pid(), STREAM_TYPE_AUDIO_AAC, stream_idx);

	
		return stream_idx;
	}

	virtual int add_extension_ltc_stream( const unsigned int avg_frame_rate = 400000
		, const uint64_t time_scale = 0)
	{	
			TSEmptyStream * p = new TSEmptyStream();

			_streams.push_back(p);

			int stream_idx = _streams.size() -1;

			p->set_pid(BASEPID + stream_idx);

			return stream_idx;
	}
	

	void set_section_continuity(unsigned char continuity)
	{
		_tswrite.set_sections_tables_continuity(continuity);
	}

	void set_only_table_at_beginning(bool only_table_at_beginning)
	{
		_only_table_at_beginning = only_table_at_beginning;
	}

	void stuff_streams()
	{
		for(uint32_t i = 0; i < _streams.size(); i++)
			_tswrite.fill_pid(_streams[i]->get_pid(), *_pctsw);
	}

	void flush()
	{
		_pctsw->flush();
	}

	//pass null to use a memory storage ??
	virtual void start(const TCHAR * file_out)
	{
		_ASSERTE(NULL == _pctsw);

		if(NULL != file_out)
		{
			SYNCTSWriteFile * p  = new SYNCTSWriteFile;
						   p->open(file_out, false);
			_pctsw = p;
		}
		else
		{
			_use_memory = true;

			CTSWriteMemory * p = new CTSWriteMemory;
			                 p->open(1024);

			 _pctsw = p;
		}

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

		//_ASSERTE(composition_time >= decoding_time);



		if( _is_first_sample 
		   || ((!_only_table_at_beginning) && _repeat_pid == stream_id && IFrame)
		   )
		{
            FDBGC3(DBGCTSMUXINFO
                , "OUTPUT-TABLES %d %d %d\r\n"
				, _is_first_sample
                , _only_table_at_beginning
                , ((!_only_table_at_beginning) && _repeat_pid == stream_id && IFrame)
				);

			_tswrite.output_sections_tables(*_pctsw);
		}

		TSMuxStream * ps = _streams[stream_id];

		ps->add_sample(body, body_size, IFrame, composition_time + _presentation_offset, decoding_time + _decoding_offset, _tswrite, *_pctsw);

		_is_first_sample = false;

	}


	
	virtual void end()
	{
		end_processesing();
	}

	virtual ~TSMux()
	{
		end_processesing();
	}

	virtual void set_auto_decoding_time(const int stream_id, const bool rhs){}

	void set_presentation_offset(const uint64_t presentation_offset)
	{_presentation_offset = presentation_offset;}

	uint64_t get_presentation_offset() const {return _presentation_offset;}

	void set_decoding_offset(const uint64_t decoding_offset)
	{_decoding_offset = decoding_offset;}

	virtual uint64_t get_memory_size()
	{
		_ASSERTE(_use_memory == true);

		return static_cast<CTSWriteMemory *>(_pctsw)->get_size();
	}

	virtual const BYTE * get_memory()
	{
		_ASSERTE(_use_memory == true);

		return static_cast<CTSWriteMemory *>(_pctsw)->get_buffer();
	}

	virtual void set_stream_pcr(int stream_id)
	{
		_streams[stream_id]->set_pcr();
		_tswrite.set_pcr_pid(
			_streams[stream_id]->get_pid()
			);
	}

	
};

class MP42TS: public CMP4Edit_
{
	TSMux  _mp4mux;
	

protected:
	
	virtual IMP4Mux2 & get_mux(){ return _mp4mux; }
	virtual void set_ctts_offset_(bool rhs){}
};


class HLSMux: public TSMux
{
	struct HLSSTREAM
	{
		uint64_t base_time;
		         uint64_t computed_offset;
	};




	std::map<int, HLSSTREAM> _composition_start_time;
	std::map<int, uint64_t> _composition_end_time;



public:


#if _DEBUG
	uint64_t _last[64];
	HLSMux()
	{
		for(int i = 0; i < 64; i++)
			_last[i] = 0;

		set_only_table_at_beginning(true);
	}
#else
    HLSMux()
	{
		set_only_table_at_beginning(true);
	}
#endif



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

		int t_stream_id = stream_id;
		//ONLY USE ONE STREAM
		t_stream_id = 0;

		HLSSTREAM s = _composition_start_time[t_stream_id];

		//_ASSERTE(composition_time >= decoding_time);
		uint64_t decoding_offset = composition_time - decoding_time;

        
		if(INT64_MAX == s.computed_offset)
		{
			s.computed_offset = s.base_time - composition_time;
		    _composition_start_time[t_stream_id] = s;

			_ASSERTE( (composition_time  + s.computed_offset) == s.base_time );

			/*
			DBGC4(L"FIRST SAMPLE %s %s %s %d\r\n"
				, HNS(composition_time)
				, HNS(composition_time  + s.computed_offset)
				, HNS(s.base_time)
				, stream_id
				);
			*/
		}

		_ASSERTE(
			(composition_time  + s.computed_offset) >= s.base_time
			);

		
		TSMux::add_sample(
				  stream_id
				, body
				, body_size
				, IFrame
				, composition_time  + s.computed_offset
				, composition_time + s.computed_offset - decoding_offset
				, duration
				);
		

#if _DEBUG
		
			if(_last[stream_id] < (composition_time  + s.computed_offset))
			{
				_last[stream_id] = (composition_time  + s.computed_offset);
			}
		
#endif

		
	}

	void set_composition_start_time(int stream_id, uint64_t composition_time)
	{
		int t_stream_id = stream_id;
		//ONLY USE ONE STREAM
		t_stream_id = 0;

		HLSSTREAM s = {composition_time, INT64_MAX};
		_composition_start_time[t_stream_id] = s;

		//set_presentation_offset(0);
	}

	void set_composition_end_time(int stream_id, uint64_t composition_time)
	{
		_composition_end_time[stream_id] = composition_time;
	}

	virtual void end()
	{
		//stuff_streams();
		TSMux::end();
	}
};


class MP42HLS: public CMP4Edit_
{
	HLSMux  _mp4mux;
	

protected:
	
	virtual IMP4Mux2 & get_mux(){ return _mp4mux; }
	virtual void set_ctts_offset_(bool rhs){}
public:
	MP42HLS()
	{
		 set_discard_pre_start(true);
	}
	void set_composition_start_time(int stream_id, uint64_t composition_time)
	{
		_mp4mux.set_composition_start_time(stream_id, composition_time);
	}

	void set_composition_end_time(int stream_id, uint64_t composition_time)
	{
		_mp4mux.set_composition_end_time(stream_id, composition_time);
	}

	virtual uint64_t get_memory_size()
	{
		return _mp4mux.get_memory_size();
	}

	virtual const BYTE * get_memory()
	{
		return _mp4mux.get_memory();
	}

	void set_section_continuity(unsigned char continuity)
	{
		_mp4mux.set_section_continuity(continuity);
	}

	void stuff_streams()
	{
		_mp4mux.stuff_streams();
	}

	void flush()
	{
		_mp4mux.flush();
	}

#if _DEBUG
	uint64_t get_last(int idx){return _mp4mux._last[idx];}
#endif
};
